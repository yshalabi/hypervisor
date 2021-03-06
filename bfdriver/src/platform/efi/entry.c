/*
 * Bareflank Hypervisor
 * Copyright (C) 2015 Assured Information Security, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <efi.h>
#include <efilib.h>

#include <vmm.h>
#include <common.h>

#include <bfdebug.h>
#include <bftypes.h>
#include <bfconstants.h>
#include <bfdriverinterface.h>

/* -------------------------------------------------------------------------- */
/* Global                                                                     */
/* -------------------------------------------------------------------------- */

uint64_t g_vcpuid = 0;

struct pmodule_t {
    char *data;
    uint64_t size;
};

uint64_t g_num_pmodules = 0;
struct pmodule_t pmodules[MAX_NUM_MODULES] = {{0}};

/* -------------------------------------------------------------------------- */
/* Misc Device                                                                */
/* -------------------------------------------------------------------------- */

static int64_t
ioctl_add_module(const char *file, uint64_t len)
{
    char *buf;
    int64_t ret;

    if (g_num_pmodules >= MAX_NUM_MODULES) {
        BFALERT("IOCTL_ADD_MODULE: too many modules have been loaded\n");
        return BF_IOCTL_FAILURE;
    }

    buf = platform_alloc_rw(len);
    if (buf == NULL) {
        BFALERT("IOCTL_ADD_MODULE: failed to allocate memory for the module\n");
        return BF_IOCTL_FAILURE;
    }

    gBS->CopyMem(buf, (void *)file, len);

    ret = common_add_module(buf, len);
    if (ret != BF_SUCCESS) {
        BFALERT("IOCTL_ADD_MODULE: common_add_module failed: %p - %s\n", (void *)ret, ec_to_str(ret));
        goto failed;
    }

    pmodules[g_num_pmodules].data = buf;
    pmodules[g_num_pmodules].size = len;

    g_num_pmodules++;

    return BF_IOCTL_SUCCESS;

failed:

    platform_free_rw(buf, len);

    BFALERT("IOCTL_ADD_MODULE: failed\n");
    return BF_IOCTL_FAILURE;
}

static long
ioctl_load_vmm(void)
{
    int64_t ret;

    ret = common_load_vmm();
    if (ret != BF_SUCCESS) {
        BFALERT("IOCTL_LOAD_VMM: common_load_vmm failed: %p - %s\n", (void *)ret, ec_to_str(ret));
        goto failure;
    }

    return BF_IOCTL_SUCCESS;

failure:

    BFDEBUG("IOCTL_LOAD_VMM: failed\n");
    return BF_IOCTL_FAILURE;
}

static long
ioctl_start_vmm(void)
{
    int64_t ret;

    ret = common_start_vmm();
    if (ret != BF_SUCCESS) {
        BFALERT("IOCTL_START_VMM: common_start_vmm failed: %p - %s\n", (void *)ret, ec_to_str(ret));
        goto failure;
    }

    return BF_IOCTL_SUCCESS;

failure:

    BFDEBUG("IOCTL_START_VMM: failed\n");
    return BF_IOCTL_FAILURE;
}

/* -------------------------------------------------------------------------- */
/* Load / Image                                                               */
/* -------------------------------------------------------------------------- */

/**
 * TODO
 *
 * Instead of loading the OS, we need to actually load a EFI/BOOT/chain.efi
 * file which is the previous EFI/BOOT/boot64.efi. This will allow us to
 * support the different types of loaders that are instealled regardless of
 * which one is actually installed.
 */

static long
load_start_vm(EFI_HANDLE ParentImage)
{
    /**
     * TODO
     *
     * Need to check to see if there are deallocate functions for a lot of
     * these functions as they are returning pointers.
     */

    EFI_STATUS status;

    UINTN i;
    UINTN NumberFileSystemHandles = 0;
    EFI_HANDLE *FileSystemHandles = NULL;

    status =
        gBS->LocateHandleBuffer(
            ByProtocol,
            &gEfiBlockIoProtocolGuid,
            NULL,
            &NumberFileSystemHandles,
            &FileSystemHandles
        );

    if (EFI_ERROR(status)) {
        BFALERT("LocateHandleBuffer failed\n");
        return EFI_ABORTED;
    }

    for(i = 0; i < NumberFileSystemHandles; ++i) {

        EFI_DEVICE_PATH_PROTOCOL *FilePath = NULL;
        EFI_BLOCK_IO *BlkIo = NULL;
        EFI_HANDLE ImageHandle = NULL;
        EFI_LOADED_IMAGE_PROTOCOL *ImageInfo = NULL;

        status =
            gBS->HandleProtocol(
                FileSystemHandles[i],
                &gEfiBlockIoProtocolGuid,
                (VOID**) &BlkIo
            );

        if (EFI_ERROR(status)) {
            continue;
        }

        FilePath = FileDevicePath(FileSystemHandles[i], L"\\EFI\\BOOT\\bootx64.efi");

        status =
            gBS->LoadImage(
                FALSE,
                ParentImage,
                FilePath,
                NULL,
                0,
                &ImageHandle
            );

        gBS->FreePool(FilePath);

        if (EFI_ERROR(status)) {
            continue;
        }

        status =
            gBS->HandleProtocol(
                ImageHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID **) &ImageInfo
        );

        if (EFI_ERROR(status)) {
            continue;
        }

        if(ImageInfo->ImageCodeType != EfiLoaderCode) {
            continue;
        }

        gBS->StartImage(ImageHandle, NULL, NULL);

        break;
    }

    BFALERT("Unable to locate EFI bootloader\n");
    return EFI_ABORTED;
}

/* -------------------------------------------------------------------------- */
/* Entry / Exit                                                               */
/* -------------------------------------------------------------------------- */

EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
    InitializeLib(image, systab);

    Print(L"\n");
    Print(L"  ___                __ _           _   \n");
    Print(L" | _ ) __ _ _ _ ___ / _| |__ _ _ _ | |__\n");
    Print(L" | _ \\/ _` | '_/ -_)  _| / _` | ' \\| / /\n");
    Print(L" |___/\\__,_|_| \\___|_| |_\\__,_|_||_|_\\_\\\n");
    Print(L"\n");
    Print(L" Please give us a star on: https://github.com/Bareflank/hypervisor\n");
    Print(L"\n");

    if (common_init() != BF_SUCCESS) {
        return EFI_ABORTED;
    }

    ioctl_add_module((char *)vmm, vmm_len);
    ioctl_load_vmm();
    ioctl_start_vmm();

    load_start_vm(image);

    return EFI_SUCCESS;
}
