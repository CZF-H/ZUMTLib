//     ZUMTLib - A C++ library for Unix Memory Operations
//     Copyright (C) 2026  CZF-H
//
//     This library is free software; you can redistribute it and/or
//     modify it under the terms of the GNU Lesser General Public
//     License as published by the Free Software Foundation; either
//     version 2.1 of the License, or (at your option) any later version.
//
//     This library is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//     Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public
//     License along with this library; if not, write to the Free Software
//     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
//     USA
//
//     江芷酱紫, 24 March 2026, Authorized Contributor of CZF-H
//     Contact: wanjiangzhi@outlook.com

//
// Created by wanjiangzhi on 2026/3/24.
//

#ifndef ZUMTLIB_ENV_H
#define ZUMTLIB_ENV_H

#include <cstdint>

#if __cplusplus >= 201703L
    #define ZUMTLib_NODISCARD [[nodiscard]]
#else
    #define ZUMTLib_NODISCARD
#endif

#define ZUMTLib_PTR_BYTE (sizeof(void*))
#define ZUMTLib_BIT      (ZUMTLib_PTR_BYTE*8)
#if INTPTR_MAX == 0x7fff
#define ZUMTLib_16_BIT

    #define ZUMTLib_CAN_16_BIT
#elif INTPTR_MAX == 0x7fffffff
#define ZUMTLib_32_BIT

    #define ZUMTLib_CAN_16_BIT
    #define ZUMTLib_CAN_32_BIT
#elif INTPTR_MAX == 0x7fffffffffffffff
    #define ZUMTLib_64_BIT

    #define ZUMTLib_CAN_16_BIT
    #define ZUMTLib_CAN_32_BIT
    #define ZUMTLib_CAN_64_BIT
#else
    #error "Unknown Size of INTPTR"
#endif

#if defined(__x86_64__)
    #define ZUMTLib_ARCH_X64
    #define ZUMTLib_X86
#elif defined(__i386__)
    #define ZUMTLib_ARCH_X86
    #define ZUMTLib_X86
#elif defined(__aarch64__)
    #define ZUMTLib_ARCH_ARM64
    #define ZUMTLib_ARM
#elif defined(__arm__)
    #define ZUMTLib_ARCH_ARM32
    #define ZUMTLib_ARM
#elif defined(__riscv)
    #define ZUMTLib_ARCH_RISCV
    #define ZUMTLib_RISCV
#elif defined(__mips__)
    #define ZUMTLib_ARCH_MIPS
    #define ZUMTLib_MIPS
#elif defined(__powerpc__)
    #define ZUMTLib_ARCH_PPC
    #define ZUMTLib_PPC
#elif defined(__loongarch__)
    #define ZUMTLib_ARCH_LOONGARCH
    #define ZUMTLib_LOONGARCH
#else
    #define ZUMTLib_ARCH_UNKNOWN
    #define ZUMTLib_ARCH_NONE
#endif

#endif //ZUMTLIB_ENV_H