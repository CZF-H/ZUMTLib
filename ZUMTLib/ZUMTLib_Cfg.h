// Copyright (C) 2026 CZF-H
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

//
// Created by wanjiangzhi on 2026/3/24.
//

/** ZUMTLib_Config.h
 *
 *
 */

#ifndef ZUMTLIB_CFG_H
#define ZUMTLIB_CFG_H

// 开启测试中功能
#define ZUMTLib_CFG_ENABLE_INTEST_FUNCTION false

// 开启此项将在没有注明的情况默认使用汇编syscall
#define ZUMTLib_CFG_DEFAULT_USE_ASM_SYSCALL false

// 开启此项将在没有注明的情况默认使用汇编memcpy
#define ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY false

// 开启此项将在没有注明的情况默认使用汇编mprotect
#define ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT false

// 开启语法 using ZUMTLib_Alias;
#define ZUMTLib_CFG_USING_ZUMTLib_Alias true

// 开启语法 using ZUMTLib_Literals;
#define ZUMTLib_CFG_USING_ZUMTLib_Literals true

// 默认自身maps
#define ZUMTLib_CFG_DEFAULT_SELF_MAPS "/proc/self/maps"

// 默认自身cmdline
#define ZUMTLib_CFG_DEFAULT_SELF_CMDLINE "/proc/self/cmdline"

// 默认模块bss段标志
#define ZUMTLib_CFG_DEFAULT_MODULE_BSS_SIGN ":bss"

// 默认地址类型
#define ZUMTLib_CFG_ADDRESS_TYPE std::uintptr_t

// 默认地偏移类型
#define ZUMTLib_CFG_OFFSET_TYPE std::uintptr_t

// 使用GameGuardian拓展类型名
#define ZUMTLib_CFG_USE_GG_TYPE true

// 使用IDA拓展类型名
#define ZUMTLib_CFG_USE_IDA_TYPE false

// 使用MSVC拓展类型名
#define ZUMTLib_CFG_USE_MSVC_TYPE false

// 使用GCC拓展类型名
#define ZUMTLib_CFG_USE_GCC_TYPE false

// 关闭高于环境位的PtrLow检测
#define ZUMTLib_CFG_DISABLE_PtrLow_CHECK_BITS false

// 特化Unreal Engine对象访问的UEPtrL(typedef PtrLow<40>)
#define ZUMTLib_CFG_ENABLE_UEPtrL true

#endif //ZUMTLIB_CFG_H
