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
// Created by wanjiangzhi on 2026/1/26 16:28.
//

#ifndef ZUMTLib_HPP
#define ZUMTLib_HPP
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "OCUnusedTypeAliasInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <initializer_list>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "ZUMTLib_Cfg.h"
#include "ZUMTLib_Env.h"

#if ZUMTLib_CFG_USING_ZUMTLib_Alias
// For using ZUMTLib_Alias;
#define ZUMTLib_Alias namespace ::ZUMTLib::alias
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
        long nr, long a1, long a2,
        long a3, long a4, long a5,
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
            : "r"(nr), "r"(a1), "r"(a2), "r"(a3),
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
        long nr, long a1, long a2,
        long a3, long a4, long a5,
        long a6
    ) noexcept {
        register long r0 __asm__("r0") = a1;
        register long r1 __asm__("r1") = a2;
        register long r2 __asm__("r2") = a3;
        register long r3 __asm__("r3") = a4;
        register long r4 __asm__("r4") = a5;
        register long r5 __asm__("r5") = a6;
        register long r7 __asm__("r7") = nr;

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
    extern "C" inline void* raw_memcpy(void* dst, const void* src, size_t n) {
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
    extern "C" inline void* multi_memcpy(void* dst, const void* src, size_t n) {
        auto d = static_cast<char*>(dst);
        auto s = static_cast<const char*>(src);

        size_t blocks = n / 16;
        size_t tail = n % 16;

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
        long nr, long a1, long a2,
        long a3, long a4, long a5,
        long a6
    ) {
        register long r10 asm("r10") = a4;
        register long r8  asm("r8")  = a5;
        register long r9  asm("r9")  = a6;

        long ret;

        __asm__ volatile(
            "syscall"
            : "=a"(ret)
            : "a"(nr),
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
        long nr, long a1, long a2,
        long a3, long a4, long a5,
        long a6 = 0 // x86 unused
    ) noexcept {
        long ret;
        (void) a6;

        __asm__ volatile (
            "int $0x80"
            : "=a"(ret)
            : "a"(nr),
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
    extern "C" inline long raw_mprotect(void* addr, const size_t len, const int prot) {
        return ZUMTLib_syscall(10, reinterpret_cast<long>(addr), static_cast<long>(len), prot, 0, 0, 0);
    }
    #define ZUMTLib_mprotect ::ZUMTAsm::raw_mprotect
    #endif
}

namespace ZUMTTool {
    #define STD_memcpy std::memcpy
    #define UNIXSTD_syscall syscall
    #define UNIXSTD_mprotect mprotect

    // ===================================== //

    #ifndef ZUMTLib_memcpy
    #define ZUMTLib_memcpy STD_memcpy
    #endif
    #ifndef ZUMTLib_memcpy_m
    #define ZUMTLib_memcpy_m STD_memcpy
    #endif
    #ifndef ZUMTLib_syscall
    #define ZUMTLib_syscall UNIXSTD_syscall
    #endif
    #ifndef ZUMTLib_mprotect
    #define ZUMTLib_mprotect UNIXSTD_mprotect
    #endif
}

namespace ZUMTLib {
    namespace Asm = ZUMTAsm;
    namespace Tool = ZUMTTool;

    auto self_maps = ZUMTLib_CFG_DEFAULT_SELF_MAPS;
    auto self_cmdline = ZUMTLib_CFG_DEFAULT_SELF_CMDLINE;
    auto bss_sign = ZUMTLib_CFG_DEFAULT_MODULE_BSS_SIGN;

    using Address_t = ZUMTLib_CFG_ADDRESS_TYPE;
    using Offset_t = ZUMTLib_CFG_OFFSET_TYPE;
    using String_t = std::string;
    using Prot_t = signed int;
    using Inode_t = ino_t;

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
    }

    namespace details {
        inline String_t remove_spaces(const String_t& str) {
            String_t result = str;
            result.erase(
                std::remove_if(
                    result.begin(), result.end(),
                    [](const unsigned char c) { return std::isspace(c); }
                ),
                result.end());
            return result;
        }
    }

    inline long PageSize() noexcept {
        static long ps = sysconf(_SC_PAGESIZE);
        return ps > 0 ? ps : 4096; // fallback
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

    static bool ReadBuffer(const Address_t addr, void* buffer, const std::size_t size, const bool asm_syscall = ZUMTLib_CFG_DEFAULT_USE_ASM_SYSCALL) noexcept { // NOLINT
        iovec local{};
        iovec remote{};

        local.iov_base = buffer;
        local.iov_len = size;
        remote.iov_base = reinterpret_cast<void*>(addr);
        remote.iov_len = size;

        const ssize_t ret =
            asm_syscall ? // NOLINT
            ZUMTLib_syscall(
                SYS_process_vm_readv,
                getpid(), // NOLINT
                reinterpret_cast<long>(&local),
                1,
                reinterpret_cast<long>(&remote),
                1,
                0
            ) :
            UNIXSTD_syscall(
                SYS_process_vm_readv,
                getpid(), // NOLINT
                &local,
                1,
                &remote,
                1,
                0
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
            }
            else {
                return address;
            }
        }

        return result;
    }

    inline bool IsPtrValid(const void* ptr, const std::size_t size = 1) noexcept {
        if (!ptr) return false;

        const auto start = reinterpret_cast<Address_t>(ptr);
        const auto end = start + size;

        const long ps = PageSize();

        for (Address_t page = start & ~(ps - 1); page < end; page += ps) {
            unsigned char vec;
            const int ret = mincore(reinterpret_cast<void*>(page), ps, &vec);
            if (ret != 0 || (vec & 1) == 0) {
                return false;
            }
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
            }
            else {
                result = endAddr;
            }
        }

        return result;
    }

    using name_addr_pairs = std::initializer_list<std::pair<const char*, Address_t*>>;

    template<typename Rep, typename Period>
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

    template<typename Rep, typename Period>
    bool AskLibBase(
        const name_addr_pairs& name_out_pair,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        for (auto& p : name_out_pair)
            AskLibBase(p.first, p.second, sleep_time);
        return true;
    }

    template<typename Rep, typename Period>
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

    template<typename Rep, typename Period>
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

    template<std::size_t bit>
    class PtrLow {
        #if !ZUMTLib_CFG_DISABLE_PtrLow_CHECK_BITS
        static_assert(bit <= ZUMTLib_BIT, "You truncated more bits than the platform limit, are you sure?");
        #endif
        Address_t local;
    public:
        PtrLow() noexcept: local(0) {}
        explicit PtrLow(const Address_t address) noexcept {
            Address_t value = 0;
            ReadBuffer(address, &value, ZUMTLib_PTR_BYTE);
            local = value & ((1ULL << bit) - 1);
        }
        explicit PtrLow(void* ptr) noexcept: PtrLow(reinterpret_cast<Address_t>(ptr)) {}
        PtrLow(const Address_t start, const std::initializer_list<Offset_t>& il) {
            if (il.size()) {
                *this = PtrLow(start);
            }
            else if (il.size() == 1) {
                *this = PtrLow(start + *il.begin());
            }
            else {
                std::size_t idx{};
                local = 0;
                // aco = address_chain_offset
                for (auto& aco : il) {
                    if (idx == 0) {
                        *this = PtrLow(start + aco);
                    }
                    else if (idx == il.size() - 1) {
                        ToOffset(aco);
                    }
                    else {
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
            ReadBuffer(addr, &value, ZUMTLib_PTR_BYTE);
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
            if (  in.size() >= 2 &&   in[1] == 'w') res |= PROT_WRITE;
            if (  in.size() >= 3 &&   in[2] == 'x') res |= PROT_EXEC;
        }
        return res;
    }

    struct BaseRange {
        Address_t start;
        Address_t end;
    };

    struct ProtRange : BaseRange {
        String_t perm;
    };

    class ProtGuard : ProcBasedClass {
        void* m_addr{};
        std::size_t m_size{};
        Prot_t m_orig{};
        bool state{true};
        bool asm_mprotect{ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT};
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
            m_addr = addr;
            m_size = sz;
            m_orig = prot;
            m_proc = proc;
        }

        ProtGuard(void* addr, const std::size_t sz, const Prot_t prot, const Proc_t* proc = nullptr) noexcept {
            (*this)(addr, sz, prot, proc);
        }

        void operator()(void* addr, const std::size_t sz, const Proc_t* proc = nullptr) noexcept {
            if (!addr || sz == 0) return;

            const long ps = PageSize();

            const auto start = reinterpret_cast<Address_t>(addr) & ~(ps - 1);
            // ReSharper disable once CppRedundantParentheses
            const auto end = (reinterpret_cast<Address_t>(addr) + sz + ps - 1) & ~(ps - 1);

            m_proc = proc;

            m_addr = reinterpret_cast<void*>(start);
            m_size = end - start;

            m_orig = LookupPerm(addr);
        }

        ProtGuard(void* addr, const std::size_t sz, const Proc_t* proc = nullptr) noexcept {
            (*this)(addr, sz, proc);
        }

        void Set_asm_mprotect(const bool to) {
            asm_mprotect = to;
        }

        void Restore() noexcept {
            if (!state) return;
            if (m_addr && m_size > 0) {
                if (asm_mprotect) {
                    ZUMTLib_mprotect(m_addr, m_size, m_orig);
                }
                else {
                    UNIXSTD_mprotect(m_addr, m_size, m_orig);
                }
            }
            state = false;
        }

        ~ProtGuard() noexcept {
            Restore();
        }

        ZUMTLib_NODISCARD bool make(const Prot_t prot) const noexcept {
            if (!m_addr || m_size == 0) return false;
            return (asm_mprotect ? ZUMTLib_mprotect(m_addr, m_size, prot) : UNIXSTD_mprotect(m_addr, m_size, prot)) == 0;
        }

        void RefreshTblManual() {
            RefreshTbl();
        }

        ProtGuard(const ProtGuard&) = delete;
        ProtGuard& operator=(const ProtGuard&) = delete;

        ProtGuard(ProtGuard&& other) noexcept
            : m_addr(other.m_addr), m_size(other.m_size), m_orig(other.m_orig),
              m_tbl(std::move(other.m_tbl)), m_tblValid(other.m_tblValid) {
            other.m_addr = nullptr;
            other.m_size = 0;
            other.m_orig = 0;
            other.m_tblValid = false;
        }

        ProtGuard& operator=(ProtGuard&& other) noexcept {
            if (this != &other) {
                m_addr = other.m_addr;
                m_size = other.m_size;
                m_orig = other.m_orig;
                m_tbl = std::move(other.m_tbl);
                m_tblValid = other.m_tblValid;

                other.m_addr = nullptr;
                other.m_size = 0;
                other.m_orig = 0;
                other.m_tblValid = false;
            }
            return *this;
        }
    };

    class Addr {
    public:
        using base_type = Address_t;
    private:
        base_type m_addr{};

        ZUMTLib_NODISCARD void* pageStart() const noexcept {
            return reinterpret_cast<void*>(m_addr & ~(PageSize() - 1));
        }

    public:
        Addr() noexcept = default;
        explicit Addr(void* ptr) noexcept: m_addr(reinterpret_cast<base_type>(ptr)) {}
        explicit Addr(const base_type addr) noexcept: m_addr(addr) {}
        explicit Addr(const String_t& stringAddr) : m_addr(std::stoull(stringAddr)) {}
        template<std::size_t bit>
        explicit Addr(const PtrLow<bit>& obj) noexcept : Addr(obj.value()) {}

        bool writeGuard(
            const void* buffer, const std::size_t length,
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY,
            const bool asm_mprotect = ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT,
            const Proc_t* proc = nullptr
        ) const {
            if (!m_addr || !buffer || length == 0) return false;

            ProtGuard guard(ptr(), length, proc);
            guard.Set_asm_mprotect(asm_mprotect);

            if (!guard.make(PROT_READ | PROT_WRITE | PROT_EXEC))
                return false;

            if (asm_memcpy) {
                if (length >= 128) { // NOLINT(*-branch-clone)
                    ZUMTLib_memcpy_m(ptr(), buffer, length);
                }
                else {
                    ZUMTLib_memcpy(ptr(), buffer, length);
                }
            }
            else {
                STD_memcpy(ptr(), buffer, length);
            }

            return true;
        }

        bool write(
            const void* buffer, const std::size_t length,
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY,
            const bool asm_mprotect = ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT
        ) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;

            void* p = ptr();

            const long ps = PageSize();
            const auto start = reinterpret_cast<Address_t>(p) & ~(ps - 1);
            // ReSharper disable once CppRedundantParentheses
            const auto end = (reinterpret_cast<Address_t>(p) + length + ps - 1) & ~(ps - 1);
            const size_t size = end - start;

            static Prot_t prot = PROT_READ | PROT_WRITE | PROT_EXEC;
            const long ret = asm_mprotect
                ? ZUMTLib_mprotect(reinterpret_cast<void*>(start), size, prot)
                : UNIXSTD_mprotect(reinterpret_cast<void*>(start), size, prot);

            if (ret != 0) return false;

            if (asm_memcpy) {
                if (length >= 128) { // NOLINT
                    ZUMTLib_memcpy_m(p, buffer, length);
                } else {
                    ZUMTLib_memcpy(p, buffer, length);
                }
            } else {
                STD_memcpy(p, buffer, length);
            }

            return true;
        }

        bool read(
            void* buffer, const std::size_t length,
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY
        ) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;

            if (asm_memcpy) {
                if (length >= 128) { // NOLINT(*-branch-clone)
                    ZUMTLib_memcpy_m(buffer, ptr(), length);
                }
                else {
                    ZUMTLib_memcpy(buffer, ptr(), length);
                }
            }
            else {
                STD_memcpy(buffer, ptr(), length);
            }
            return true;
        }

        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        bool writeType(
            const _Ty& value,
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY,
            const bool asm_mprotect = ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT
        ) const noexcept {
            return write(&value, _Sz, asm_memcpy, asm_mprotect);
        }

        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        bool writeTypeGuard(
            const _Ty& value,
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY,
            const bool asm_mprotect = ZUMTLib_CFG_DEFAULT_USE_ASM_MPROTECT,
            const Proc_t* proc = nullptr
        ) const {
            return writeGuard(&value, _Sz, asm_memcpy, asm_mprotect, proc);
        }

        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        _Ty readType(
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY
        ) const noexcept {
            _Ty result{};
            read(reinterpret_cast<void*>(&result), _Sz, asm_memcpy);
            return result;
        }

        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        bool readType(
            _Ty* buf,
            const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY
        ) const noexcept {
            return read(reinterpret_cast<void*>(buf), _Sz);
        }

        ZUMTLib_NODISCARD base_type address() const noexcept {
            return m_addr;
        }

        ZUMTLib_NODISCARD base_type operator*() const noexcept {
            return m_addr;
        }

        ZUMTLib_NODISCARD void* ptr() const noexcept {
            return reinterpret_cast<void*>(m_addr);
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator base_type() const noexcept {
            return address();
        }

        ZUMTLib_NODISCARD Addr alignUp(const base_type alignment) const noexcept {
            return Addr((m_addr + alignment - 1) & ~(alignment - 1));
        }
        ZUMTLib_NODISCARD Addr alignDown(const base_type alignment) const noexcept {
            return Addr(m_addr & ~(alignment - 1));
        }
        Addr operator+(const base_type offset) const noexcept {
            return Addr(m_addr + offset);
        }
        Addr operator-(const base_type offset) const noexcept {
            return Addr(m_addr - offset);
        }
        Addr& operator+=(const base_type offset) noexcept {
            m_addr += offset;
            return *this;
        }
        Addr& operator-=(const base_type offset) noexcept {
            m_addr -= offset;
            return *this;
        }
        Addr operator&(const base_type mask) const noexcept { return Addr(m_addr & mask); }
        Addr operator|(const base_type mask) const noexcept { return Addr(m_addr | mask); }
        Addr operator^(const base_type mask) const noexcept { return Addr(m_addr ^ mask); }

        Addr& operator&=(const base_type mask) noexcept {
            m_addr &= mask;
            return *this;
        }
        Addr& operator|=(const base_type mask) noexcept {
            m_addr |= mask;
            return *this;
        }
        Addr& operator^=(const base_type mask) noexcept {
            m_addr ^= mask;
            return *this;
        }
        Addr operator<<(const unsigned shift) const noexcept {
            return Addr(m_addr << shift);
        }
        Addr operator>>(const unsigned shift) const noexcept {
            return Addr(m_addr >> shift);
        }
        Addr& operator<<=(const unsigned shift) noexcept {
            m_addr <<= shift;
            return *this;
        }
        Addr& operator>>=(const unsigned shift) noexcept {
            m_addr >>= shift;
            return *this;
        }

        bool operator==(const Addr& other) const noexcept { return m_addr == other.m_addr; }
        bool operator!=(const Addr& other) const noexcept { return m_addr != other.m_addr; }
        bool operator<(const Addr& other) const noexcept { return m_addr < other.m_addr; }
        bool operator>(const Addr& other) const noexcept { return m_addr > other.m_addr; }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator bool() const noexcept { return m_addr != 0; }
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
            const String_t& targetName, bool merge = false
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
                        }
                        else {
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
        ) noexcept: m_name(std::move(name)),
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
                }
                else {
                    it->second.range().end = end;
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
            char* endptr;
            Address_t start = strtoul(str, &endptr, 16);
            if (*endptr != '-') continue;
            Address_t end = strtoul(endptr + 1, &endptr, 16);
            if (*endptr != ' ') continue;
            while (*endptr == ' ') endptr++;
            String_t perms(endptr, 4);

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
            if (addr >= r.start.address() && addr < r.end.address()) {
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
        const bool asm_memcpy = ZUMTLib_CFG_DEFAULT_USE_ASM_MEMCPY
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
                if (asm_memcpy) {
                    if (size >= 128) { // NOLINT(*-branch-clone)
                        ZUMTLib_memcpy_m(buffer.data(), ptr, size);
                    }
                    else {
                        ZUMTLib_memcpy(buffer.data(), ptr, size);
                    }
                }
                else {
                    STD_memcpy(buffer.data(), ptr, size);
                }
                ofs.write(buffer.data(), static_cast<std::streamsize>(size));
            }
            else {
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
        inline Addr operator""_Addr(const unsigned long long address) {
            return Addr{static_cast<Address_t>(address)};
        }
        inline PtrL32 operator""_PtrL32(const unsigned long long address) {
            return PtrL32{static_cast<Address_t>(address)};
        }
        inline PtrL40 operator""_PtrL40(const unsigned long long address) {
            return PtrL40{static_cast<Address_t>(address)};
        }
        inline PtrL48 operator""_PtrL48(const unsigned long long address) {
            return PtrL48{static_cast<Address_t>(address)};
        }
    }
}
#pragma clang diagnostic pop
#endif //ZUMTLib_HPP