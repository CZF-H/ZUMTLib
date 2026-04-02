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
// Created by wanjiangzhi on 2026/1/26 16:28.
//

#ifndef ZUMTLib_HPP
#define ZUMTLib_HPP
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "OCUnusedTypeAliasInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <cstddef>
#include <cstring>
#include <utility>
#include <initializer_list>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <elf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "ZUMTLib_Cfg.h"
#include "ZUMTLib_Env.h"

#if ZUMTLib_CFG_USING_ZUMTType_Alias
// For using ZUMTType_Alias;
#define ZUMTType_Alias namespace ::ZUMTType::alias
#endif
#if ZUMTLib_CFG_USING_ZUMTConfig
#define ZUMTConfig namespace ::ZUMTCfg
#endif
#if ZUMTLib_CFG_USING_ZUMTLib_Literals
// For using ZUMTLib_Literals;
#define ZUMTLib_Literals namespace ::ZUMTLib::literals
#endif

namespace ZUMTAsm {
    #ifdef ZUMTLib_ARCH_ARM64
    extern "C" inline void* raw_memcpy(void* dst, const void* src, std::size_t n) {
        auto d = static_cast<char*>(dst);
        auto s = static_cast<const char*>(src);

        asm volatile (
            "1:\n"
            "cbz %w2, 2f\n"
            "ldrb w3, [%1], #1\n"
            "strb w3, [%0], #1\n"
            "subs %w2, %w2, #1\n"
            "b.ne 1b\n"
            "2:\n"
            : "+r"(d),
            "+r"(s),
            "+r"(n)
            :
            : "w3", "memory"
        );

        return dst;
    }
    #define ZUMTLib_memcpy ::ZUMTAsm::raw_memcpy
    extern "C" inline void* multi_memcpy(void* dst, const void* src, std::size_t n) {
        auto d = static_cast<char*>(dst);
        auto s = static_cast<const char*>(src);
        asm volatile (
            "1:\n"
            "cmp %2, #16\n"
            "blt 2f\n"
            "ld1 {v0.16b}, [%1], #16\n"
            "st1 {v0.16b}, [%0], #16\n"
            "sub %2, %2, #16\n"
            "b 1b\n"
            "2:\n"
            : "+r"(d), "+r"(s), "+r"(n)
            :
            : "v0", "memory"
        );
        while (n--) *d++ = *s++;
        return dst;
    }
    #define ZUMTLib_memcpy_m ::ZUMTAsm::multi_memcpy
    extern "C" inline long raw_syscall(
        long number,
        long a1,
        long a2,
        long a3,
        long a4,
        long a5,
        long a6
    ) noexcept {
        long ret;
        __asm__ volatile (
            "mov x8, %1\n"
            "mov x0, %2\n"
            "mov x1, %3\n"
            "mov x2, %4\n"
            "mov x3, %5\n"
            "mov x4, %6\n"
            "mov x5, %7\n"
            "svc #0\n"
            "mov %0, x0\n"
            : "=r"(ret)
            : "r"(number), "r"(a1), "r"(a2), "r"(a3),
            "r"(a4), "r"(a5), "r"(a6)
            : "x0","x1","x2","x3","x4","x5","x8","memory"
        );
        return ret;
    }
    #define ZUMTLib_syscall ::ZUMTAsm::raw_syscall
    #endif
    #ifdef ZUMTLib_ARCH_ARM32
    extern "C" inline void* raw_memcpy(void* dst, const void* src, std::size_t n) {
        auto d = static_cast<char*>(dst);
        auto s = static_cast<const char*>(src);

        asm volatile (
            "1:\n"
            "cmp %0, #0\n"
            "beq 2f\n"
            "ldrb r3, [%1], #1\n"
            "strb r3, [%2], #1\n"
            "subs %0, %0, #1\n"
            "bne 1b\n"
            "2:\n"
            : "+r"(n),
            "+r"(s),
            "+r"(d)
            :
            : "r3", "memory"
        );

        return dst;
    }
    #define ZUMTLib_memcpy_m ::ZUMTAsm::raw_memcpy
    extern "C" inline void* multi_memcpy(void* dst, const void* src, std::size_t n) {
        auto d = static_cast<char*>(dst);
        auto s = static_cast<const char*>(src);
        std::size_t words = n / 4;
        std::size_t tail = n % 4;

        asm volatile (
            "1:\n"
            "cmp %0, #0\n"
            "beq 2f\n"
            "ldr r3, [%1], #4\n"
            "str r3, [%2], #4\n"
            "subs %0, %0, #1\n"
            "bne 1b\n"
            "2:\n"
            : "+r"(words),
            "+r"(s),
            "+r"(d)
            :
            : "r3", "memory"
        );

        while (tail--) *d++ = *s++;

        return dst;
    }
    #define ZUMTLib_memcpy_m ::ZUMTAsm::multi_memcpy
    extern "C" inline long raw_syscall(
        long number,
        long a1,
        long a2,
        long a3,
        long a4,
        long a5,
        long a6
    ) noexcept {
        register long r0 __asm__("r0") = a1;
        register long r1 __asm__("r1") = a2;
        register long r2 __asm__("r2") = a3;
        register long r3 __asm__("r3") = a4;
        register long r4 __asm__("r4") = a5;
        register long r5 __asm__("r5") = a6;
        register long r7 __asm__("r7") = number;

        __asm__ volatile (
            "svc 0"
            : "+r"(r0)
            : "r"(r7),
            "r"(r1),
            "r"(r2),
            "r"(r3),
            "r"(r4),
            "r"(r5)
            : "memory"
        );

        return r0;
    }
    #define ZUMTLib_syscall ::ZUMTAsm::raw_syscall
    #endif
    #ifdef ZUMTLib_X86
    extern "C" inline void* raw_memcpy(void* dst, const void* src, std::size_t n) {
        void* ret = dst;

        asm volatile (
            "rep movsb"
            : "+D"(dst),
            "+S"(src),
            "+c"(n)
            :
            : "memory"
        );

        return ret;
    }

    #define ZUMTLib_memcpy ::ZUMTAsm::raw_memcpy

    extern "C" inline void* multi_memcpy(void* dst, const void* src, std::size_t n) {
        auto d = static_cast<char*>(dst);
        auto s = static_cast<const char*>(src);

        std::size_t blocks = n / 16;
        std::size_t tail = n % 16;

        asm volatile (
            "1:\n"
            "cmp $0, %0\n"
            "je 2f\n"
            "movdqu (%1), %%xmm0\n"
            "movdqu %%xmm0, (%2)\n"
            "add $16, %1\n"
            "add $16, %2\n"
            "dec %0\n"
            "jmp 1b\n"
            "2:\n"
            : "+r"(blocks),
            "+r"(s),
            "+r"(d)
            :
            : "xmm0", "memory"
        );

        while (tail--) *d++ = *s++;

        return dst;
    }

    #define ZUMTLib_memcpy_m ::ZUMTAsm::multi_memcpy
    #ifdef ZUMTLib_ARCH_X64
    inline long raw_syscall(
        long number,
        long a1,
        long a2,
        long a3,
        long a4,
        long a5,
        long a6
    ) {
        register long r10 asm("r10") = a4;
        register long r8 asm("r8") = a5;
        register long r9 asm("r9") = a6;

        long ret;

        __asm__ volatile(
            "syscall"
            : "=a"(ret)
            : "a"(number),
            "D"(a1),
            "S"(a2),
            "d"(a3),
            "r"(r10),
            "r"(r8),
            "r"(r9)
            : "rcx", "r11", "memory"
        );

        return ret;
    }

    #define ZUMTLib_syscall ::ZUMTAsm::raw_syscall
    #endif
    #ifdef ZUMTLib_ARCH_X86
    extern "C" inline long raw_syscall(
        long number,
        long a1,
        long a2,
        long a3,
        long a4,
        long a5,
        long a6 = 0 // x86 unused
    ) noexcept {
        long ret;
        (void)a6;

        __asm__ volatile (
            "int $0x80"
            : "=a"(ret)
            : "a"(number),
            "b"(a1),
            "c"(a2),
            "d"(a3),
            "S"(a4),
            "D"(a5)
            : "memory"
        );

        return ret;
    }
    #define ZUMTLib_syscall ::ZUMTAsm::raw_syscall
    #endif
    #endif

    #ifdef ZUMTLib_syscall
    extern "C" inline long raw_mprotect(void* addr, const std::size_t len, const int prot) {
        return ZUMTLib_syscall(10, reinterpret_cast<long>(addr), static_cast<long>(len), prot, 0, 0, 0);
    }

    #define ZUMTLib_mprotect ::ZUMTAsm::raw_mprotect
    #endif
}

namespace ZUMTTool {
    #define STD_memcpy std::memcpy
    #define UNISTD_syscall syscall
    #define UNISTD_mprotect mprotect

    // ===================================== //

    #ifndef ZUMTLib_memcpy
    #define ZUMTLib_memcpy STD_memcpy
    #endif
    #ifndef ZUMTLib_memcpy_m
    #define ZUMTLib_memcpy_m STD_memcpy
    #endif
    #ifndef ZUMTLib_syscall
    #define ZUMTLib_syscall UNISTD_syscall
    #endif
    #ifndef ZUMTLib_mprotect
    #define ZUMTLib_mprotect UNISTD_mprotect
    #endif
}

namespace ZUMTCfg {
    using asm_flag_t = uint8_t;

    enum asm_flag_ : asm_flag_t {
        asm_none_func = 0,
        asm_memcpy    = 1 << 0, // 0000 0001
        asm_syscall   = 1 << 1, // 0000 0010
        asm_mprotect  = 1 << 2, // 0000 0100
    };

    inline asm_flag_ asm_flag_default() {
        static constexpr asm_flag_ default_flag = asm_none_func
            #if ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY
        | asm_memcpy
            #endif
            #if ZUMTLib_CFG_DEFAULT_USE_ASM_SYSCALL
        | asm_syscall
            #endif
            #if ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT
        | asm_mprotect
            #endif
            ;
        return default_flag;
    }

    struct asm_cfg_t {
        bool memcpy   : 1;
        bool syscall  : 1;
        bool mprotect : 1;

        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr asm_cfg_t(const asm_flag_t flag = asm_flag_default()) :
            memcpy((flag & asm_memcpy) != 0),
            syscall((flag & asm_syscall) != 0),
            mprotect((flag & asm_mprotect) != 0) {}

        constexpr asm_cfg_t(
            const bool _memcpy,
            const bool _syscall,
            const bool _mprotect
        ) :
            memcpy(_memcpy),
            syscall(_syscall),
            mprotect(_mprotect) {}

        asm_cfg_t(const std::initializer_list<std::pair<asm_flag_, bool>>& list) : asm_cfg_t() {
            for (const auto& pair : list) {
                const auto& flag = pair.first;
                const auto& val = pair.second;

                switch (flag) {
                    case asm_none_func: break;

                    case asm_memcpy: memcpy = val;
                        break;
                    case asm_syscall: syscall = val;
                        break;
                    case asm_mprotect: mprotect = val;
                        break;

                    default: break;
                }
            }
        }
    };

    using fn_asm_cfg = const asm_cfg_t;

    enum byte_endian_ : uint8_t {
        little_endian,
        big_endian
    };

    using fn_byte_endian = const byte_endian_;
    constexpr byte_endian_ default_endian = ZUMTLib_ENDIAN;
}

namespace ZUMTType {
    using Address_t = ZUMTLib_CFG_ADDRESS_TYPE;
    using Offset_t = ZUMTLib_CFG_OFFSET_TYPE;
    using String_t = std::string;
    using Prot_t = signed int;
    using Inode_t = ino_t;

    using Byte_t = std::uint8_t;
    using Bytes_t = std::vector<Byte_t>;

    namespace alias {
        using BYTE = std::int8_t;
        using WORD = std::int16_t;
        using DWORD = std::int32_t;
        using QWORD = std::int64_t;
        using FLOAT = float;
        using DOUBLE = double;

        #if ZUMTLib_CFG_USE_IDA_TYPE

        #endif

        #if ZUMTLib_CFG_USE_MSVC_TYPE

        #endif

        #if ZUMTLib_CFG_USE_GCC_TYPE

        #endif

        #if ZUMTLib_CFG_USE_GG_TYPE
        using B = BYTE;
        using W = WORD;
        using D = DWORD;
        using Q = QWORD;
        using F = FLOAT;
        using E = DOUBLE;
        #endif
    }}

namespace ZUMTLib {
    namespace Asm = ZUMTAsm;
    namespace Tool = ZUMTTool;

    auto self_maps = ZUMTLib_CFG_DEFAULT_SELF_MAPS;
    auto self_cmdline = ZUMTLib_CFG_DEFAULT_SELF_CMDLINE;
    auto bss_sign = ZUMTLib_CFG_DEFAULT_MODULE_BSS_SIGN;


    using namespace ZUMTCfg;
    using namespace ZUMTType;

    namespace details {
        inline String_t remove_spaces(const String_t& str) {
            String_t result = str;
            result.erase(
                std::remove_if(
                    result.begin(),
                    result.end(),
                    [](const unsigned char c) { return std::isspace(c); }
                ),
                result.end()
            );
            return result;
        }

        inline Address_t align_down(const Address_t x, const Address_t align) {
            return x & ~(align - 1); // NOLINT
        }

        inline Address_t align_up(const Address_t x, const Address_t align) {
            return (x + align - 1) & ~(align - 1); // NOLINT
        }

        // ReSharper disable CppParameterMayBeConst
        inline void* configured_memcpy(
            void* dst,
            const void* src,
            const std::size_t len,
            fn_asm_cfg asm_cfg = {}
        ) noexcept {
            void* result{};
            if (asm_cfg.memcpy) {
                result = len >= 128 ? ZUMTLib_memcpy_m(dst, src, len) : ZUMTLib_memcpy(dst, src, len);;
            } else {
                result = STD_memcpy(dst, src, len);
            }
            return result;
        }

        inline int configured_mprotect(
            void* addr,
            std::size_t len,
            int prot,
            fn_asm_cfg asm_cfg = {}
        ) noexcept {
            return asm_cfg.mprotect
                       ? static_cast<int>(ZUMTLib_mprotect(addr, len, prot))
                       : UNISTD_mprotect(addr, len, prot);
        }

        inline long configured_syscall(
            long number,
            long a1,
            long a2,
            long a3,
            long a4,
            long a5,
            long a6,
            fn_asm_cfg asm_cfg = {}
        ) noexcept {
            return asm_cfg.syscall
                       ? ZUMTLib_syscall(number, a1, a2, a3, a4, a5, a6)
                       : UNISTD_syscall(number, a1, a2, a3, a4, a5, a6);
        }
        // ReSharper enable CppParameterMayBeConst
    }

    struct BaseRange {
        Address_t start;
        Address_t end;
    };

    struct PageInfo : BaseRange {
        std::size_t size;
    };

    struct ProtRange : BaseRange {
        String_t perm;
    };

    inline long PageSize() noexcept {
        static long ps = sysconf(_SC_PAGESIZE);
        return ps > 0 ? ps : 4096; // fallback
    }

    inline PageInfo PageAlign(
        const Address_t addr,
        const std::size_t len
    ) {
        PageInfo result{};
        const auto ps = static_cast<Address_t>(PageSize());

        result.start = details::align_down(addr, ps);
        result.end = details::align_up(addr + len, ps);
        result.size = result.end - result.start;

        return result;
    }

    template <typename Type>
    Bytes_t ToBytes(
        const Type& value,
        fn_byte_endian endian = default_endian,
        fn_asm_cfg asm_cfg = {}
    ) {
        #if ZUMTLib_CFG_BYTES_CONVERT_MEMCPY_COPYABLE_CHECK
        static_assert(
            std::is_trivially_copyable<Type>::value,
            "Type must be trivially copyable"
        );
        #endif

        const std::size_t type_size = sizeof(Type);
        Bytes_t bytes(type_size);

        details::configured_memcpy(bytes.data(), &value, type_size, asm_cfg);

        if (endian == big_endian) {
            std::reverse(bytes.begin(), bytes.end());
        }

        return bytes;
    }

    template <typename Type>
    Type BytesTo(
        const Bytes_t& bytes,
        fn_byte_endian endian = default_endian,
        fn_asm_cfg asm_cfg = {}
    ) {
        #if ZUMTLib_CFG_BYTES_CONVERT_MEMCPY_COPYABLE_CHECK
        static_assert(
            std::is_trivially_copyable<Type>::value,
            "Type must be trivially copyable"
        );
        #endif

        const std::size_t type_size = sizeof(Type);
        if (bytes.size() != type_size) {
            throw std::runtime_error("size mismatch");
        }

        Bytes_t temp = bytes;

        if (endian == big_endian) {
            std::reverse(temp.begin(), temp.end());
        }

        Type value{};
        details::configured_memcpy(&value, temp.data(), type_size, asm_cfg);

        return value;
    }

    inline String_t Bytes2Hex(const Bytes_t& bytes) {
        std::ostringstream oss;
        for (const uint8_t b : bytes) {
            oss << std::hex
                << std::uppercase
                << std::setw(2)
                << std::setfill('0')
                << static_cast<short>(b)
                << " ";
        }
        return {oss.str()};
    }

    class BytesHEX {
        Bytes_t bytes;

    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        BytesHEX(const char* hex) {
            while (*hex) {
                while (*hex == ' ') hex++;
                if (!*hex) break;

                char byte_str[3] = {0};
                byte_str[0] = *hex++;
                if (!*hex) break;
                byte_str[1] = *hex++;

                auto val = static_cast<uint8_t>(std::strtoul(byte_str, nullptr, 16));
                bytes.push_back(val);
            }
        }

        // ReSharper disable once CppNonExplicitConvertingConstructor
        BytesHEX(const String_t& hex) : BytesHEX(hex.c_str()) {}

        std::size_t size() const noexcept {
            return bytes.size();
        }

        Byte_t* data() noexcept {
            return bytes.data();
        }

        const Byte_t* data() const noexcept {
            return bytes.data();
        }

        String_t hex() const {
            return Bytes2Hex(bytes);
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator const Bytes_t&() const noexcept {
            return bytes;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator const Bytes_t*() const noexcept {
            return &bytes;
        }

        Bytes_t& details_inside_data() noexcept {
            return bytes;
        }
    };

    inline Bytes_t Hex2Bytes(const String_t& hex) {
        return std::move(BytesHEX(hex).details_inside_data());
    }

    inline pid_t GetPid() noexcept {
        return getpid();
    }

    struct Proc_t {
        pid_t pid{};
        std::string maps;
        std::string cmdline;

        Proc_t() :
            pid(GetPid()),
            maps(self_maps),
            cmdline(self_maps) {}

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Proc_t(const pid_t& _pid) {
            static const std::string _proc = "/proc/";
            static const std::string _maps = "/maps";
            static const std::string _cmdline = "/cmdline";
            if (pid != _pid) {
                pid = _pid;
                const std::string _pid_str = std::to_string(pid);
                if (!_pid_str.empty()) {
                    ((maps = _proc) += _pid_str) += _maps;
                    ((cmdline = _proc) += _pid_str) += _cmdline;
                }
            }
        }
    };

    struct ProcBasedClass {
    protected:
        const Proc_t* m_proc{};

    public:
        void ChangeProc(const Proc_t* proc = nullptr) noexcept {
            m_proc = proc;
        }

        ZUMTLib_NODISCARD const Proc_t* Proc() const noexcept {
            return m_proc;
        }
    };

    static bool ReadPtr(
        const Address_t addr,
        void* buffer,
        // NOLINTNEXTLINE
        const std::size_t size,
        fn_asm_cfg asm_cfg = {}
    ) noexcept {
        iovec local{};
        iovec remote{};

        local.iov_base = buffer;
        local.iov_len = size;
        remote.iov_base = reinterpret_cast<void*>(addr);
        remote.iov_len = size;

        const ssize_t ret = details::configured_syscall(
            SYS_process_vm_readv,
            getpid(),
            reinterpret_cast<long>(&local),
            1,
            reinterpret_cast<long>(&remote),
            1,
            0,
            asm_cfg
        );

        return ret == static_cast<ssize_t>(size);
    }

    inline Address_t GetModuleBase(String_t name, const Proc_t* proc = {}) {
        std::ifstream maps(proc ? proc->maps : self_maps);
        if (!maps.is_open()) {
            return 0;
        }

        String_t line;
        bool is_bss = false;

        if (name.size() > 4 && name.rfind(bss_sign) == name.size() - 4) {
            is_bss = true;
            name = name.substr(0, name.size() - 4);
        }

        Address_t result = 0;
        Offset_t max_offset = 0;

        while (std::getline(maps, line)) {
            if (line.find(name) == String_t::npos)
                continue;

            std::istringstream iss(line);
            String_t addressRange, perms, dev;
            Inode_t inode;
            Offset_t offset;

            if (!(iss >> addressRange >> perms)) // string
                continue;

            if (!(iss >> std::hex >> offset)) // hex
                continue;

            if (!(iss >> dev)) // string
                continue;

            if (!(iss >> std::dec >> inode)) // dec
                continue;

            auto dashPos = addressRange.find('-');
            if (dashPos == String_t::npos)
                continue;

            Address_t address{};
            try {
                address = std::stoull(addressRange.substr(0, dashPos), nullptr, 16);
            } catch (...) { continue; }

            if (is_bss) {
                if (perms.find("rw-p") == 0) {
                    if (offset >= max_offset) {
                        max_offset = offset;
                        result = address;
                    }
                }
            } else {
                return address;
            }
        }

        return result;
    }

    inline bool IsPtrValid(const void* ptr, const std::size_t size = 1) noexcept {
        if (!ptr) return false;
        const long ps = PageSize();

        const auto info = PageAlign(reinterpret_cast<Address_t>(ptr), size);
        for (Address_t page = info.start; page < info.end; page += ps) {
            unsigned char vec;
            if (mincore(reinterpret_cast<void*>(page), ps, &vec) != 0 || (vec & 1) == 0)
                return false;
        }
        return true;
    }

    inline bool IsSafeAddress(const Address_t addr, const std::size_t size) noexcept {
        if (addr <= 0x10000000 || addr >= 0x10000000000) return false;
        return IsPtrValid(reinterpret_cast<void*>(addr), size);
    }

    inline bool IsLibraryLoaded(const String_t& name, const Proc_t* proc = {}) {
        std::ifstream mapsFile(proc ? proc->maps : self_maps);
        if (!mapsFile.is_open()) return false;
        String_t line;
        while (std::getline(mapsFile, line))
            if (line.find(name) != String_t::npos) return true;
        return false;
    }

    inline Address_t GetModuleEnd(String_t name, const Proc_t* proc = {}) {
        std::ifstream maps(proc ? proc->maps : self_maps);
        if (!maps.is_open()) {
            return 0;
        }

        bool is_bss = false;
        if (name.size() > 4 && name.rfind(bss_sign) == name.size() - 4) {
            is_bss = true;
            name = name.substr(0, name.size() - 4);
        }

        Address_t result = 0;
        Offset_t max_offset = 0;
        String_t line;

        while (std::getline(maps, line)) {
            if (line.find(name) == String_t::npos)
                continue;

            std::istringstream iss(line);
            String_t addressRange, perms, dev;
            Inode_t inode;
            Offset_t offset;

            if (!(iss >> addressRange >> perms)) // string
                continue;

            if (!(iss >> std::hex >> offset)) // hex
                continue;

            if (!(iss >> dev)) // string
                continue;

            if (!(iss >> std::dec >> inode)) // dec
                continue;

            auto dashPos = addressRange.find('-');
            if (dashPos == String_t::npos)
                continue;

            Address_t endAddr = 0;
            try {
                endAddr = std::stoull(addressRange.substr(dashPos + 1), nullptr, 16);
            } catch (...) {
                continue;
            }

            if (endAddr == 0) continue;

            if (is_bss) {
                if (perms.find("rw-p") == 0) {
                    if (offset >= max_offset) {
                        max_offset = offset;
                        result = endAddr;
                    }
                }
            } else {
                result = endAddr;
            }
        }

        return result;
    }

    using name_addr_pairs = std::initializer_list<std::pair<const char*, Address_t*>>;

    template <typename Rep, typename Period>
    bool AskLibBase(
        const char* name,
        Address_t* out,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        if (!out) return false;
        while (!*out) {
            *out = GetModuleBase(name);
            if (*out) return true;
            if (sleep_time.contains())
                std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        }
        return true;
    }

    template <typename Rep, typename Period>
    bool AskLibBase(
        const name_addr_pairs& name_out_pair,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        for (auto& p : name_out_pair)
            AskLibBase(p.first, p.second, sleep_time);
        return true;
    }

    template <typename Rep, typename Period>
    bool AskLibEnd(
        const char* name,
        Address_t* out,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        if (!out) return false;
        while (!*out) {
            *out = GetModuleEnd(name);
            if (*out) return true;
            if (sleep_time.contains())
                std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        }
        return true;
    }

    template <typename Rep, typename Period>
    bool AskLibEnd(
        const name_addr_pairs& name_out_pair,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        for (auto& p : name_out_pair)
            AskLibEnd(p.first, p.second, sleep_time);
        return true;
    }

    inline String_t ReadCmdlineName(const bool stop_at_colon = false, const Proc_t* proc = {}) {
        std::ifstream f(proc ? proc->cmdline : self_cmdline, std::ios::in | std::ios::binary);
        if (!f.is_open()) {
            return {};
        }

        String_t name;
        char ch;
        while (f.get(ch)) {
            if (ch == '\0') break;
            if (stop_at_colon && ch == ':') break;
            name += ch;
        }

        return name;
    }

    template <std::size_t bit>
    class PtrLow {
        #if !ZUMTLib_CFG_DISABLE_PtrLow_CHECK_BITS
        static_assert(bit <= ZUMTLib_BIT, "You truncated more bits than the platform limit, are you sure?");
        #endif
        Address_t local;

    public:
        PtrLow() noexcept : local(0) {}

        explicit PtrLow(const Address_t address) noexcept {
            Address_t value = 0;
            ReadPtr(address, &value, ZUMTLib_PTR_BYTE);
            local = value & ((1ULL << bit) - 1);
        }

        explicit PtrLow(void* ptr) noexcept : PtrLow(reinterpret_cast<Address_t>(ptr)) {}

        PtrLow(const Address_t start, const std::initializer_list<Offset_t>& il) {
            if (il.size()) {
                *this = PtrLow(start);
            } else if (il.size() == 1) {
                *this = PtrLow(start + *il.begin());
            } else {
                std::size_t idx{};
                local = 0;
                // aco = address_chain_offset
                for (auto& aco : il) {
                    if (idx == 0) {
                        *this = PtrLow(start + aco);
                    } else if (idx == il.size() - 1) {
                        ToOffset(aco);
                    } else {
                        ToNext(aco);
                    }
                    idx++;
                }
            }
        }

        PtrLow(
            const String_t& name,
            const std::initializer_list<Offset_t>& il
        )
            : PtrLow(GetModuleBase(name), il) {}

        PtrLow(const Address_t address, const Offset_t offset) : PtrLow(address) {
            ToNext(offset);
        }

    private:
        PtrLow(const Address_t value, const bool is_value) noexcept {
            if (is_value) local = value & ((1ULL << bit) - 1);
        }

    public:
        ZUMTLib_NODISCARD PtrLow Next(const Offset_t offset = 0) const noexcept {
            const Address_t addr = (local & ((1ULL << bit) - 1)) + offset;
            Address_t value = 0;
            ReadPtr(addr, &value, ZUMTLib_PTR_BYTE);
            return PtrLow(value, true);
        }

        PtrLow& ToNext(const Offset_t offset = 0) noexcept {
            local = Next(offset).value();
            return *this;
        }

        ZUMTLib_NODISCARD PtrLow Offset(const Offset_t offset) const noexcept {
            PtrLow result = *this;
            result.local += offset;
            return result;
        }

        PtrLow& ToOffset(const Offset_t offset = 0) noexcept {
            local = Offset(offset).value();
            return *this;
        }

        PtrLow operator+(const Offset_t offset) const noexcept { return Offset(offset); }
        PtrLow& operator+=(const Offset_t offset) noexcept { return ToOffset(offset); }
        PtrLow operator-(const Offset_t offset) const noexcept { return Offset(-offset); }

        PtrLow& operator-=(const Offset_t offset) {
            this->local -= offset;
            return *this;
        }

        PtrLow operator>>(const Offset_t offset) const noexcept { return Next(offset); }
        PtrLow& operator>>=(const Offset_t offset) { return ToNext(offset); }
        PtrLow operator()(const Offset_t offset) const noexcept { return Next(offset); }

        ZUMTLib_NODISCARD Address_t value() const noexcept { return local; }
        ZUMTLib_NODISCARD bool empty() const noexcept { return local == 0; }
        ZUMTLib_NODISCARD Address_t operator*() const noexcept { return value(); }
        explicit operator Address_t() const noexcept { return value(); }
    };

    #ifdef ZUMTLib_CAN_64_BIT
    /**
     * @brief PtrL32 For:
     * Old 32-bit program
     * Special compressed data
     */
    using PtrL32 = PtrLow<32>;
    /**
     * @brief PtrL40 For:
     * Unreal Engine、Game
     * Pointer compression、Memory optimization
     * Offset Linked List、Memory mapping
     */
    using PtrL40 = PtrLow<40>;
    /**
     * @brief PtrL48 For:
     * Some Windows x64 kernel space addresses
     * Some Windows x64 user space addresses
     */
    using PtrL48 = PtrLow<48>;

    #if ZUMTLib_CFG_ENABLE_UEPtrL
    /**
     * @brief UEPtrL For:
     * Unreal Engine Object Pointer
     */
    using UEPtrL = PtrLow<40>;
    #endif
    #endif

    inline Prot_t ParsePerm(const String_t& in) noexcept {
        Prot_t res = 0;
        if (!in.empty()) {
            if (/*in.size() >= 1 &&*/ in[0] == 'r') res |= PROT_READ;
            if (in.size() >= 2 && in[1] == 'w') res |= PROT_WRITE;
            if (in.size() >= 3 && in[2] == 'x') res |= PROT_EXEC;
        }
        return res;
    }

    class ProtGuard : ProcBasedClass {
        void* m_ptr{};
        std::size_t m_size{};
        Prot_t m_orig{};
        bool state{true};
        asm_cfg_t asm_cfg = {};

    protected:
        std::vector<ProtRange> m_tbl{};
        bool m_tblValid{false};

        void RefreshTbl() {
            m_tbl.clear();
            std::ifstream maps(m_proc ? m_proc->maps : self_maps);
            if (!maps.is_open()) return;

            String_t line;
            while (std::getline(maps, line)) {
                std::istringstream iss(line);
                String_t addrRange, perm;
                if (!(iss >> addrRange >> perm)) continue;

                std::size_t dash = addrRange.find('-');
                if (dash == String_t::npos) continue;

                Address_t start = std::stoull(addrRange.substr(0, dash), nullptr, 16);
                Address_t end = std::stoull(addrRange.substr(dash + 1), nullptr, 16);

                ProtRange localRange;
                localRange.start = start;
                localRange.end = end;
                localRange.perm = perm;

                m_tbl.push_back(std::move(localRange));
            }
            m_tblValid = true;
        }

        Prot_t LookupPerm(void* addr) {
            if (!m_tblValid) RefreshTbl();
            const auto target = reinterpret_cast<Address_t>(addr);
            for (const auto& r : m_tbl) {
                if (target >= r.start && target < r.end) {
                    return ParsePerm(r.perm);
                }
            }
            return PROT_READ;
        }

    public:
        ProtGuard() noexcept = default;

        void operator()(void* addr, const std::size_t sz, const Prot_t prot, const Proc_t* proc = nullptr) noexcept {
            m_ptr = addr;
            m_size = sz;
            m_orig = prot;
            m_proc = proc;
        }

        ProtGuard(void* addr, const std::size_t sz, const Prot_t prot, const Proc_t* proc = nullptr) noexcept {
            (*this)(addr, sz, prot, proc);
        }

        void operator()(void* addr, const std::size_t sz, const Proc_t* proc = nullptr) noexcept {
            if (!addr || sz == 0) return;

            m_proc = proc;

            const auto page = PageAlign(reinterpret_cast<Address_t>(addr), sz);
            m_ptr = reinterpret_cast<void*>(page.start);
            m_size = page.size;

            m_orig = LookupPerm(addr);
        }

        ProtGuard(void* addr, const std::size_t sz, const Proc_t* proc = nullptr) noexcept {
            (*this)(addr, sz, proc);
        }

        void Set_asm_cfg(fn_asm_cfg to) {
            asm_cfg = to;
        }

        void Restore() noexcept {
            if (!state) return;
            if (m_ptr && m_size > 0) {
                details::configured_mprotect(m_ptr, m_size, m_orig, asm_cfg);
            }
            state = false;
        }

        ~ProtGuard() noexcept {
            Restore();
        }

        ZUMTLib_NODISCARD bool make(const Prot_t prot) const noexcept {
            if (!m_ptr || m_size == 0) return false;
            return details::configured_mprotect(m_ptr, m_size, prot, asm_cfg) == 0;
        }

        void RefreshTblManual() {
            RefreshTbl();
        }

        ProtGuard(const ProtGuard&) = delete;
        ProtGuard& operator=(const ProtGuard&) = delete;

        ProtGuard(ProtGuard&& other) noexcept
            : m_ptr(other.m_ptr),
              m_size(other.m_size),
              m_orig(other.m_orig),
              m_tbl(std::move(other.m_tbl)),
              m_tblValid(other.m_tblValid) {
            other.m_ptr = nullptr;
            other.m_size = 0;
            other.m_orig = 0;
            other.m_tblValid = false;
        }

        ProtGuard& operator=(ProtGuard&& other) noexcept {
            if (this != &other) {
                m_ptr = other.m_ptr;
                m_size = other.m_size;
                m_orig = other.m_orig;
                m_tbl = std::move(other.m_tbl);
                m_tblValid = other.m_tblValid;

                other.m_ptr = nullptr;
                other.m_size = 0;
                other.m_orig = 0;
                other.m_tblValid = false;
            }
            return *this;
        }
    };

    class AddrBase {
    public:
        using base_type = Address_t;

    protected:
        base_type m_addr{};

    public:
        // ReSharper disable CppNonExplicitConvertingConstructor
        constexpr AddrBase() noexcept = default;
        explicit constexpr AddrBase(const base_type addr) noexcept : m_addr(addr) {}
        template <std::size_t bit> AddrBase(const PtrLow<bit>& obj) noexcept : AddrBase(obj.value()) {}
        // ReSharper enable CppNonExplicitConvertingConstructor

        bool writeGuardFrom(
            const void* buffer,
            const std::size_t length,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) const {
            if (!m_addr || !buffer || length == 0) return false;

            ProtGuard guard(ptr(), length, proc);
            guard.Set_asm_cfg(asm_cfg);
            if (!guard.make(PROT_READ | PROT_WRITE | PROT_EXEC))
                return false;
            details::configured_memcpy(ptr(), buffer, length, asm_cfg);
            return true;
        }

        bool writeGuardFrom(
            const Bytes_t* bytes,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) const {
            return writeGuardFrom(bytes->data(), bytes->size(), asm_cfg, proc);
        }

        bool writeGuardHex(
            const char* hex_bytes,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) const {
            const BytesHEX bytes{hex_bytes};
            return writeGuardFrom(bytes, asm_cfg, proc);
        }

        template <typename Type, std::size_t size = sizeof(Type)>
        bool writeGuard(
            const Type& value,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) const {
            return writeGuardFrom(&value, size, asm_cfg, proc);
        }

        bool writeFrom(
            const void* buffer,
            const std::size_t length,
            fn_asm_cfg asm_cfg = {}
        ) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;

            const auto page = PageAlign(m_addr, length);
            static Prot_t prot = PROT_READ | PROT_WRITE | PROT_EXEC;
            if (details::configured_mprotect(reinterpret_cast<void*>(page.start), page.size, prot, asm_cfg) != 0) return false;
            details::configured_memcpy(ptr(), buffer, length, asm_cfg);
            return true;
        }

        bool writeFrom(
            const Bytes_t& bytes,
            fn_asm_cfg asm_cfg = {}
        ) const {
            return writeFrom(bytes.data(), bytes.size(), asm_cfg);
        }

        bool writeHex(
            const char* hex_bytes,
            fn_asm_cfg asm_cfg = {}
        ) const {
            const BytesHEX hex_arr{hex_bytes};
            return writeFrom(hex_arr, asm_cfg);
        }

        template <typename Type, std::size_t size = sizeof(Type)>
        bool write(
            const Type& value,
            fn_asm_cfg asm_cfg = {}
        ) const noexcept {
            return writeFrom(&value, size, asm_cfg);
        }

        bool readTo(
            void* buffer,
            const std::size_t length,
            fn_asm_cfg asm_cfg = {}
        ) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;
            details::configured_memcpy(buffer, ptr(), length, asm_cfg);
            return true;
        }

        template <typename Type, std::size_t size = sizeof(Type)>
        bool readTo(
            Type* buf,
            fn_asm_cfg asm_cfg = {}
        ) const noexcept {
            return readTo(reinterpret_cast<void*>(buf), size, asm_cfg);
        }

        bool readTo(
            Bytes_t& data,
            const std::size_t size,
            fn_asm_cfg asm_cfg = {}
        ) const {
            data.resize(size);
            return readTo(data.data(), size, asm_cfg);
        }

        bool readTo(
            Bytes_t& data,
            fn_asm_cfg asm_cfg = {}
        ) const {
            ;
            return readTo(data.data(), data.size(), asm_cfg);
        }

        template <std::size_t size>
        String_t readHex(
            fn_asm_cfg asm_cfg = {}
        ) const {
            Bytes_t data(size);
            if (readTo(data.data(), size, asm_cfg)) {
                return Bytes2Hex(data);
            }
            return {};
        }

        template <typename Type, std::size_t size = sizeof(Type)>
        Type read(
            fn_asm_cfg asm_cfg = {}
        ) const noexcept {
            Type result{};
            readTo(reinterpret_cast<void*>(&result), size, asm_cfg);
            return result;
        }

        template <std::size_t size>
        Bytes_t read(
            fn_asm_cfg asm_cfg = {}
        ) const {
            Bytes_t bytes{};
            readTo(bytes, size, asm_cfg);
            return bytes;
        }

        ZUMTLib_NODISCARD base_type get() const noexcept {
            return m_addr;
        }

        ZUMTLib_NODISCARD void* ptr() const noexcept {
            return reinterpret_cast<void*>(m_addr);
        }

        /* 此处不表示 按位取反，如需要地址按位取反清使用"~~"或者".NOT成员" */
        ZUMTLib_NODISCARD base_type operator~() const noexcept {
            return m_addr;
        }

        ZUMTLib_NODISCARD base_type NOT() const noexcept {
            return ~m_addr;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator base_type() const noexcept {
            return m_addr;
        }

        base_type operator()() const noexcept {
            return m_addr;
        }

        ZUMTLib_NODISCARD AddrBase alignUp(const base_type alignment) const noexcept {
            return AddrBase((m_addr + alignment - 1) & ~(alignment - 1));
        }

        ZUMTLib_NODISCARD AddrBase alignDown(const base_type alignment) const noexcept {
            return AddrBase(m_addr & ~(alignment - 1));
        }

        AddrBase operator+(const base_type offset) const noexcept {
            return AddrBase(m_addr + offset);
        }

        AddrBase operator-(const base_type offset) const noexcept {
            return AddrBase(m_addr - offset);
        }

        AddrBase& operator+=(const base_type offset) noexcept {
            m_addr += offset;
            return *this;
        }

        AddrBase& operator-=(const base_type offset) noexcept {
            m_addr -= offset;
            return *this;
        }

        AddrBase operator&(const base_type mask) const noexcept { return AddrBase(m_addr & mask); }
        AddrBase operator|(const base_type mask) const noexcept { return AddrBase(m_addr | mask); }
        AddrBase operator^(const base_type mask) const noexcept { return AddrBase(m_addr ^ mask); }

        AddrBase& operator&=(const base_type mask) noexcept {
            m_addr &= mask;
            return *this;
        }

        AddrBase& operator|=(const base_type mask) noexcept {
            m_addr |= mask;
            return *this;
        }

        AddrBase& operator^=(const base_type mask) noexcept {
            m_addr ^= mask;
            return *this;
        }

        AddrBase operator<<(const unsigned shift) const noexcept {
            return AddrBase(m_addr << shift);
        }

        AddrBase operator>>(const unsigned shift) const noexcept {
            return AddrBase(m_addr >> shift);
        }

        AddrBase& operator<<=(const unsigned shift) noexcept {
            m_addr <<= shift;
            return *this;
        }

        AddrBase& operator>>=(const unsigned shift) noexcept {
            m_addr >>= shift;
            return *this;
        }

        bool operator==(const AddrBase& other) const noexcept { return m_addr == other.m_addr; }
        bool operator!=(const AddrBase& other) const noexcept { return m_addr != other.m_addr; }
        bool operator<(const AddrBase& other) const noexcept { return m_addr < other.m_addr; }
        bool operator>(const AddrBase& other) const noexcept { return m_addr > other.m_addr; }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator bool() const noexcept { return m_addr != 0; }
    };

    class VFunction {
    protected:
        void** m_vfunc_ptr;

        VFunction() = default;
    public:
        VFunction(const Address_t vfunc_addr) {
            if (vfunc_addr == 0)
                throw std::invalid_argument("vtable_addr is null");
            const auto vtable_ptr = reinterpret_cast<void**>(vfunc_addr);
            if (*vtable_ptr) {
                m_vfunc_ptr = vtable_ptr;
            }
        }

        VFunction(void** vfunc_ptr) {
            if (vfunc_ptr && *vfunc_ptr) m_vfunc_ptr = vfunc_ptr;
            else throw std::invalid_argument("func_ptr or func is null");
        }

        VFunction(void** vtable_ptr, const std::size_t idx) {
            if (vtable_ptr && vtable_ptr[idx]) m_vfunc_ptr = &vtable_ptr[idx];
            else throw std::invalid_argument("vtable_ptr or vtable[idx] is null");
        }

        void* func() const noexcept {
            return *m_vfunc_ptr;
        }

        void** ptr() const noexcept {
            return m_vfunc_ptr;
        }

        std::size_t index(void** vtable) const {
            return ptr() - vtable;
        }

        Address_t addr() const noexcept {
            return reinterpret_cast<Address_t>(ptr());
        }

        bool write(
            void* new_func,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) const {
            if (!ptr() || !new_func) return false;

            ProtGuard guard(ptr(), ZUMTLib_PTR_BYTE, proc);
            guard.Set_asm_cfg(asm_cfg);
            if (!guard.make(PROT_READ | PROT_WRITE | PROT_EXEC))
                return false;
            *ptr() = new_func;
            return true;
        }

        bool writeNoGuard(
            void* new_func,
            fn_asm_cfg asm_cfg = {}
        ) const {
            if (!ptr() || !new_func) return false;

            const auto page = PageAlign(addr(), ZUMTLib_PTR_BYTE);
            static Prot_t prot = PROT_READ | PROT_WRITE | PROT_EXEC;
            if (details::configured_mprotect(reinterpret_cast<void*>(page.start), page.size, prot, asm_cfg) != 0) return false;
            *ptr() = new_func;
            return true;
        }

        bool read(void** buffer) const {
            if (!ptr() || !buffer) return false;
            *buffer = func();
            return true;
        }

        void* read() const {
            return func();
        }

        bool valid() const noexcept {
            return m_vfunc_ptr && *m_vfunc_ptr;
        }

        bool hook(
            void* hsub,
            void** osub = nullptr,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) const {
            if (!ptr() || !hsub) return false;
            if (func() == hsub) return false;
            void* orig = func();
            if (write(hsub, asm_cfg, proc)) {
                if (osub) *osub = orig;
                return true;
            }
            return false;
        }

        bool hookNoGuard(
            void* hsub,
            void** osub = nullptr,
            fn_asm_cfg asm_cfg = {}
        ) const {
            if (!ptr() || !hsub) return false;
            if (func() == hsub) return false;
            void* orig = func();
            if (writeNoGuard(hsub, asm_cfg)) {
                if (osub) *osub = orig;
                return true;
            }
            return false;
        }
    };

    inline bool VFuncHook(
        const VFunction& vf,
        void* hsub,
        void** osub = nullptr,
        fn_asm_cfg asm_cfg = {},
        const Proc_t* proc = nullptr
    ) {
        return vf.hook(hsub, osub, asm_cfg, proc);
    }

    inline bool VFuncHookNoGuard(
        const VFunction& vf,
        void* hsub,
        void** osub = nullptr,
        fn_asm_cfg asm_cfg = {}
    ) {
        return vf.hookNoGuard(hsub, osub, asm_cfg);
    }

    class VFuncHookRAII {
        const Proc_t* m_proc;
        VFunction m_vf;
        void* m_osub{};
        bool m_hooked{};
        asm_cfg_t m_asm_cfg;
    public:
        void* osub() const noexcept {
            return m_osub;
        }

        bool Hooked() const noexcept {
            return m_hooked;
        }

        bool Hook(void* hsub) {
            if (!m_hooked) {
                m_hooked = m_vf.hook(hsub, &m_osub, m_asm_cfg, m_proc);
            }
            return m_hooked;
        }

        bool DeHook() {
            if (m_hooked) {
                if (m_vf.write(m_osub, m_asm_cfg, m_proc)) {
                    m_osub = nullptr;
                    m_hooked = false;
                    return true;
                }
                return false;
            }
            return true;
        }

        bool ReHook(void* hsub) {
            return DeHook() ? Hook(hsub) : false;
        }

        VFuncHookRAII(
            const VFunction& vf,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) : m_proc(proc), m_vf(vf), m_asm_cfg(asm_cfg) {}

        VFuncHookRAII(
            const VFunction& vf,
            void* hsub,
            fn_asm_cfg asm_cfg = {},
            const Proc_t* proc = nullptr
        ) : m_proc(proc), m_vf(vf), m_asm_cfg(asm_cfg) {
            Hook(hsub);
        }

        ~VFuncHookRAII() {
            DeHook();
        }
    };

    class VFuncHookNoGuardRAII {
        VFunction m_vf;
        void* m_osub{};
        bool m_hooked{};
        asm_cfg_t m_asm_cfg;
    public:
        void* osub() const noexcept {
            return m_osub;
        }

        bool Hooked() const noexcept {
            return m_hooked;
        }

        bool Hook(void* hsub) {
            if (!m_hooked) {
                m_hooked = m_vf.hookNoGuard(hsub, &m_osub, m_asm_cfg);
            }
            return m_hooked;
        }

        bool DeHook() {
            if (m_hooked) {
                if (m_vf.writeNoGuard(m_osub, m_asm_cfg)) {
                    m_osub = nullptr;
                    m_hooked = false;
                    return true;
                }
                return false;
            }
            return true;
        }

        bool ReHook(void* hsub) {
            return DeHook() ? Hook(hsub) : false;
        }

        VFuncHookNoGuardRAII(
            const VFunction& vf,
            fn_asm_cfg asm_cfg = {}
        ) : m_vf(vf), m_asm_cfg(asm_cfg) {}

        VFuncHookNoGuardRAII(
            const VFunction& vf,
            void* hsub,
            fn_asm_cfg asm_cfg = {}
        ) : m_vf(vf), m_asm_cfg(asm_cfg) {
            Hook(hsub);
        }

        ~VFuncHookNoGuardRAII() {
            DeHook();
        }
    };

    class VTable {
        void** m_vtable;
    public:
        VTable(const Address_t vtable_addr) {
            if (vtable_addr == 0)
                throw std::invalid_argument("vtable_addr is null");
            const auto vtable_ptr = reinterpret_cast<void**>(vtable_addr);
            if (*vtable_ptr) {
                m_vtable = vtable_ptr;
            }
        }

        VTable(void** vtable_ptr) {
            if (vtable_ptr && *vtable_ptr) m_vtable = vtable_ptr;
            else throw std::invalid_argument("vtable is null");
        }

        template <typename Class,
                  typename = typename std::enable_if<
                      std::is_polymorphic<typename std::remove_reference<Class>::type>::value
                  >::type> VTable(Class* const obj) {
            if (!obj) {
                throw std::invalid_argument("object is null");
            }

            m_vtable = *reinterpret_cast<void***>(obj);
            if (!m_vtable) {
                throw std::invalid_argument("vtable is null");
            }
        }

        void** vtable() const noexcept {
            return m_vtable;
        }

        VFunction at(const std::size_t idx) const noexcept {
            return {vtable(), idx};
        }

        VFunction operator[](const std::size_t idx) const noexcept {
            return at(idx);
        }
    };

    template <typename Class,
              typename = typename std::enable_if<
                  std::is_polymorphic<typename std::remove_reference<Class>::type>::value
              >::type> Class* PolyCall(Class& clazz) {
        return &clazz;
    }

    class AddrPtr : public AddrBase {
    public:
        using BasedClass = AddrBase;
        using BasedClass::AddrBase;

        AddrPtr(void* ptr) noexcept : AddrBase(reinterpret_cast<base_type>(ptr)) {}
    };

    class AddrString : public AddrBase {
    public:
        using BasedClass = AddrBase;
        using BasedClass::AddrBase;

        AddrString(const String_t& str_addr) : AddrBase(std::stoull(str_addr, nullptr, 16)) {}
    };

    class Addr : public AddrBase {
    public:
        using BasedClass = AddrBase;
        using BasedClass::AddrBase;

        explicit Addr(void* ptr) noexcept : AddrBase(reinterpret_cast<base_type>(ptr)) {}
        explicit Addr(const String_t& str_addr) : AddrBase(std::stoull(str_addr, nullptr, 16)) {}
    };

    struct AddrRange {
        Addr start;
        Addr end;
    };

    class Module : ProcBasedClass {
        String_t m_name;
        AddrRange m_range;
        String_t m_perms;
        Offset_t m_offset{};
        String_t m_dev;
        Inode_t m_inode{};
        String_t m_path;

    public:
        Module* Self() noexcept {
            return this;
        }

        Module() = default;

        const Module& Scan(
            const String_t& targetName,
            bool merge = false
        ) noexcept {
            try {
                std::ifstream maps(m_proc ? m_proc->maps : self_maps);
                String_t line;
                Addr firstStart;
                bool firstFound = false;

                while (std::getline(maps, line)) {
                    std::istringstream iss(line);
                    String_t addr, perm, dev;
                    Inode_t inode;
                    Offset_t off;
                    if (!(iss >> addr >> perm >> std::hex >> off >> dev >> inode))
                        continue;

                    String_t path;
                    std::getline(iss, path);
                    if (!path.empty() && path.find(targetName) != String_t::npos) {
                        std::size_t dash = addr.find('-');
                        Addr start(std::stoull(addr.substr(0, dash), nullptr, 16));
                        Addr end(std::stoull(addr.substr(dash + 1), nullptr, 16));

                        if (!firstFound) {
                            firstStart = start;
                            m_name = targetName;
                            m_perms = perm;
                            m_offset = off;
                            m_dev = dev;
                            m_inode = inode;
                            m_path = path.substr(path.find_first_not_of(' '));

                            firstFound = true;
                        }

                        if (merge) {
                            m_range.start = firstStart;
                            m_range.end = end;
                        } else {
                            m_range.start = start;
                            m_range.end = end;
                            break;
                        }
                    }
                }
            } catch (...) {}
            return *this;
        }

        explicit Module(
            const String_t& targetName,
            const bool merge = false,
            const Proc_t* proc = nullptr
        ) noexcept {
            m_proc = proc;
            Scan(targetName, merge);
        }

        void operator()(
            const String_t& targetName,
            const bool merge = false,
            const Proc_t* proc = nullptr
        ) noexcept {
            m_proc = proc;
            Scan(targetName, merge);
        }

        Module(
            String_t name,
            const AddrRange range,
            String_t perms,
            const Offset_t offset,
            String_t dev,
            const Inode_t inode,
            String_t path,
            const Proc_t* proc = nullptr
        ) noexcept : m_name(std::move(name)),
                     m_range(range),
                     m_perms(std::move(perms)),
                     m_offset(offset),
                     m_dev(std::move(dev)),
                     m_inode(inode),
                     m_path(std::move(path)) {
            m_proc = proc;
        }

        void operator()(
            String_t name = {},
            const AddrRange range = {},
            String_t perms = {},
            const Offset_t offset = {},
            String_t dev = {},
            String_t path = {},
            const Proc_t* proc = {}
        ) noexcept {
            m_name = std::move(name);
            m_range = range;
            m_perms = std::move(perms);
            m_offset = offset;
            m_dev = std::move(dev);
            m_path = std::move(path);
            m_proc = proc;
        }

        String_t& name() noexcept {
            return m_name;
        }

        AddrRange& range() noexcept {
            return m_range;
        }

        String_t& perms() noexcept {
            return m_perms;
        }

        Offset_t& offset() noexcept {
            return m_offset;
        }

        String_t& dev() noexcept {
            return m_dev;
        }

        Inode_t& inode() noexcept {
            return m_inode;
        }

        String_t& path() noexcept {
            return m_path;
        }
    };

    using ModuleList = std::vector<Module>;

    inline ModuleList GetModuleList(
        const std::vector<std::string>& filters = {},
        bool merge = false,
        const Proc_t* proc = {}
    ) {
        std::ifstream maps(proc ? proc->maps : self_maps);
        String_t line;
        ModuleList modules;

        std::unordered_map<String_t, Module> moduleMap;

        while (std::getline(maps, line)) {
            std::istringstream iss(line);
            String_t addr, perms, dev;
            Inode_t inode;
            Offset_t offset;

            if (!(iss >> addr >> perms >> std::hex >> offset >> dev >> inode))
                continue;

            String_t path;
            std::getline(iss, path);

            if (!path.empty()) {
                bool found = false;
                if (!filters.empty()) {
                    for (auto& filter : filters) {
                        if (path.find(filter) != String_t::npos) {
                            found = true;
                        }
                        if (found) break;
                    }
                } else found = true;
                if (found) {
                    std::size_t pos = path.find_first_not_of(' ');
                    if (pos == String_t::npos) continue;
                    path = path.substr(pos);
                    if (path.empty()) continue;

                    std::size_t dash = addr.find('-');
                    if (dash == String_t::npos) continue;

                    Addr start(std::stoull(addr.substr(0, dash), nullptr, 16));
                    Addr end(std::stoull(addr.substr(dash + 1), nullptr, 16));

                    if (!merge) {
                        modules.emplace_back(
                            path.substr(path.find_last_of('/') + 1),
                            AddrRange{start, end},
                            perms,
                            offset,
                            dev,
                            inode,
                            path
                        );
                        continue;
                    }

                    auto it = moduleMap.find(path);
                    if (it == moduleMap.end()) {
                        Module mod(
                            path.substr(path.find_last_of('/') + 1),
                            {start, end},
                            perms,
                            offset,
                            dev,
                            inode,
                            path
                        );
                        moduleMap[path] = mod;
                    } else {
                        it->second.range().end = end;
                    }
                }
            }
        }

        if (merge) {
            for (auto& kv : moduleMap)
                modules.push_back(kv.second);
        }

        return modules;
    }

    struct MapRegion : AddrRange {
        String_t perms;
    };

    using MapRegions = std::vector<MapRegion>;

    inline MapRegions ParseMaps(const Proc_t* proc = {}) {
        MapRegions regions;
        std::ifstream ifs(proc ? proc->maps : self_maps);
        String_t line;

        while (std::getline(ifs, line)) {
            const char* str = line.c_str();
            char* end_ptr;
            Address_t start = strtoul(str, &end_ptr, 16);
            if (*end_ptr != '-') continue;
            Address_t end = strtoul(end_ptr + 1, &end_ptr, 16);
            if (*end_ptr != ' ') continue;
            while (*end_ptr == ' ') end_ptr++;
            String_t perms(end_ptr, 4);

            MapRegion localRegion;
            localRegion.start = Addr(start);
            localRegion.end = Addr(end);
            localRegion.perms = perms;

            regions.push_back(std::move(localRegion));
        }

        return regions;
    }

    inline bool IsReadable(
        const Address_t addr,
        const MapRegions& maps
    ) {
        for (const auto& r : maps) {
            if (addr >= ~r.start && addr < ~r.end) {
                return !r.perms.empty() && r.perms[0] == 'r';
            }
        }
        return false;
    }

    #if ZUMTLib_CFG_ENABLE_INTEST_FUNCTION

    inline bool Dump(
        const AddrRange& range,
        const String_t& outPath,
        Addr::base_type chunkSize = 0,
        asm_cfg asm_cfg = {}
    ) {
        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) return false;

        MapRegions maps = ParseMaps();

        if (chunkSize == 0)
            chunkSize = PageSize();

        std::vector<char> buffer(chunkSize);
        std::vector<char> zeros(chunkSize, 0);

        Addr current = range.start;

        const MapRegion* currentRegion = nullptr;

        while (current < range.end) {
            std::size_t remaining = range.end.address() - current.address();
            std::size_t size = std::min(chunkSize, remaining);

            Address_t addr = current.address();
            void* ptr = current.ptr();

            if (!ptr) return false;

            if (!currentRegion ||
                addr < currentRegion->start.address() ||
                addr >= currentRegion->end.address()) {
                currentRegion = nullptr;

                for (const auto& r : maps) {
                    if (addr >= r.start.address() && addr < r.end.address()) {
                        currentRegion = &r;
                        break;
                    }
                }
            }

            bool readable = currentRegion && !currentRegion->perms.empty() && currentRegion->perms[0] == 'r';

            if (readable) {
                details::configured_memcpy(buffer.data(), ptr, size, asm_cfg);
                ofs.write(buffer.data(), static_cast<std::streamsize>(size));
            } else {
                ofs.write(zeros.data(), static_cast<std::streamsize>(size));
            }

            current += size;
        }

        return true;
    }

    #endif

    namespace literals {
        inline Module operator""_Module(const char* name) {
            return Module{name};
        }

        inline AddrBase operator""_Addr(const unsigned long long address) {
            return AddrBase{static_cast<Address_t>(address)};
        }

        inline Offset_t operator""_Offset(const unsigned long long address) {
            return address;
        }
    }

    constexpr auto AUTHOR_NAME = "江芷酱紫";
    constexpr auto PROJECT_NAME = "ZUMTLib";
    constexpr auto PROJECT_VERSION = "1.1.1";
    constexpr auto PROJECT_VERSION_CODE = 10101;
}
#pragma clang diagnostic pop
#endif //ZUMTLib_HPP
