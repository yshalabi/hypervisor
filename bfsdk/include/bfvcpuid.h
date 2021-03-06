//
// Bareflank Hypervisor
// Copyright (C) 2015 Assured Information Security, Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef BFVCPUID_H
#define BFVCPUID_H

#include <cstdint>

// *INDENT-OFF*

namespace vcpuid
{
    using type = uint64_t;

    constexpr const auto reserved = 0x8000000000000000UL;

    constexpr const auto invalid = 0xFFFFFFFFFFFFFFFFUL;
    constexpr const auto current = 0xFFFFFFFFFFFFFFF0UL;

    constexpr const auto guest_mask = 0xFFFFFFFFFFFF0000UL;
    constexpr const auto guest_from = 16;

    /// Is Bootstrap vCPU
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param id the id to check
    /// @return true if this vCPU is the bootstrap vCPU, false otherwise
    ///
    constexpr inline bool is_bootstrap_vcpu(type id)
    { return id == 0; }

    /// Is Host VM vCPU
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param id the id to check
    /// @return true if this vCPU belongs to the host VM, false otherwise
    ///
    constexpr inline bool is_host_vm_vcpu(type id)
    { return (id & (vcpuid::guest_mask & ~vcpuid::reserved)) == 0; }

    /// Is Guest VM vCPU
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param id the id to check
    /// @return true if this vCPU belongs to a guest VM, false otherwise
    ///
    constexpr inline bool is_guest_vm_vcpu(type id)
    { return !is_host_vm_vcpu(id); }
}

// *INDENT-ON*

#endif
