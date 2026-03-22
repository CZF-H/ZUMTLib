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

// For using ZUMTLib_Alias;
#define ZUMTLib_Alias namespace ::ZUMTLib::alias

namespace ZUMTLib {
    const char* self_maps = "/proc/self/maps";
    const char* self_cmdline = "/proc/self/cmdline";
    const char* bss_sign = ":bss";
    
    using Offset_t = std::uintptr_t;
    using Address_t = std::uintptr_t;
    using String_t = std::string;
    using Prot_t = signed int;
    using Inode_t = ino_t;
    
    namespace details {
        inline String_t remove_spaces(const String_t& str) {
            String_t result = str;
            result.erase(
                std::remove_if(result.begin(), result.end(),
                               [](unsigned char c){ return std::isspace(c); }),
                result.end());
            return result;
        }
    }
    namespace alias {
        using BYTE = int8_t;
        using WORD = int16_t;
        using DWORD = int32_t;
        using QWORD = int64_t;
        using FLOAT = float;
        using DOUBLE = double;
        
        using B = BYTE;
        using W = WORD;
        using D = DWORD;
        using Q = QWORD;
        using F = FLOAT;
        using E = DOUBLE;
    }
    
    pid_t GetPid() noexcept {
        return getpid();
    }
    
    struct Proc_t {
        pid_t pid;
        std::string maps;
        std::string cmdline;
        
        Proc_t() :
            pid(GetPid()),
            maps(self_maps),
            cmdline(self_maps)
        {}
        
        Proc_t(const pid_t& _pid) {
            static const std::string _proc = "/proc/";
            static const std::string _maps = "/maps";
            static const std::string _cmdline = "/cmdline";
            if (pid != _pid) {
                pid = _pid;
                const std::string _pid_str = std::to_string(pid);
                {
                    ((maps = _proc) += _pid_str) += _maps;
                    ((cmdline = _proc) += _pid_str) += _cmdline;
                }
            }
        }
    };
    
    struct ProcBasedClass {
    protected:
        Proc_t* m_proc{};
    public:
        void ChangeProc(Proc_t* proc = nullptr) noexcept {
            m_proc = proc;
        }
        ZUMTLib_NODISCARD Proc_t* Proc() const noexcept {
            return m_proc;
        }
    };
    
    static bool ReadBuffer(Address_t addr, void* buffer, size_t size) noexcept {
        iovec local{};
        iovec remote{};
        
        local.iov_base = buffer;
        local.iov_len = size;
        remote.iov_base = reinterpret_cast<void*>(addr);
        remote.iov_len = size;
        
        const ssize_t ret = syscall(
            SYS_process_vm_readv,
            getpid(),
            &local, 1,
            &remote, 1,
            0
        );
        
        return ret == static_cast<ssize_t>(size);
    }
    
    template<size_t bit>
    class PtrLow {
        static_assert(bit <= ZUMTLib_BIT, "You truncated more bits than the platform limit, are you sure?");
    private:
        Address_t local;
    public:
        explicit PtrLow(Address_t address) noexcept {
            Address_t value = 0;
            ReadBuffer(address, &value, ZUMTLib_PTR_BYTE);
            local = value & ((1ULL << bit) - 1);
        }
    private:
        explicit PtrLow(Address_t value, bool is_value) noexcept {
            local = value & ((1ULL << bit) - 1);
        }
    
    public:
        PtrLow Next(Offset_t offset = 0) const noexcept {
            Address_t addr = (local & ((1ULL << bit) - 1)) + offset;
            Address_t value = 0;
            ReadBuffer(addr, &value, ZUMTLib_PTR_BYTE);
            return PtrLow<bit>(value, true);
        }
        
        PtrLow& ToNext(Offset_t offset = 0) noexcept {
            local = Next(offset).value();
            return *this;
        }
        
        PtrLow Offset(Offset_t offset) const noexcept {
            PtrLow<bit> result = *this;
            result.local += offset;
            return result;
        }
        
        PtrLow operator+(Offset_t offset) const noexcept { return Offset(offset); }
        PtrLow& operator+=(Offset_t offset) noexcept {
            this->local += offset;
            return *this;
        }
        PtrLow operator-(Offset_t offset) const noexcept { return Offset(-offset); }
        PtrLow& operator-=(Offset_t offset) {
            this->local -= offset;
            return *this;
        }
        
        PtrLow operator>>(Offset_t offset) const noexcept { return Next(offset); }
        PtrLow& operator>>=(Offset_t offset) {
            ToNext(offset);
            return *this;
        }
        PtrLow operator()(Offset_t offset) const noexcept { return Next(offset); }
        
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
    #endif
    
    inline long PageSize() noexcept {
        static long ps = sysconf(_SC_PAGESIZE);
        return ps > 0 ? ps : 4096; // fallback
    }
    
    Prot_t ParsePerm(const String_t& in) noexcept {
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
    private:
        void* m_addr{};
        size_t m_size{};
        Prot_t m_orig{};
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
                
                size_t dash = addrRange.find('-');
                if (dash == String_t::npos) continue;
                
                Address_t start = std::stoull(addrRange.substr(0, dash), nullptr, 16);
                Address_t end   = std::stoull(addrRange.substr(dash + 1), nullptr, 16);
                
                m_tbl.push_back({start, end, perm});
            }
            m_tblValid = true;
        }
        
        Prot_t LookupPerm(void* addr) {
            if (!m_tblValid) RefreshTbl();
            auto target = reinterpret_cast<Address_t>(addr);
            for (auto& r : m_tbl) {
                if (target >= r.start && target < r.end) {
                    return ParsePerm(r.perm);
                }
            }
            return PROT_READ;
        }
    
    public:
        ProtGuard() noexcept = default;
        
        void operator()(void* addr, size_t sz, Prot_t prot, Proc_t* proc = nullptr) noexcept {
            m_addr = addr;
            m_size = sz;
            m_orig = prot;
            m_proc = proc;
        }
        
        ProtGuard(void* addr, size_t sz, Prot_t prot, Proc_t* proc = nullptr) noexcept {
            (*this)(addr, sz, prot, proc);
        }
        
        void operator()(void* addr, size_t sz, Proc_t* proc = nullptr) noexcept {
            if (!addr || sz == 0) return;
            
            long ps = PageSize();
            
            auto start = reinterpret_cast<Address_t>(addr) & ~(ps - 1);
            auto end   = (reinterpret_cast<Address_t>(addr) + sz + ps - 1) & ~(ps - 1);
            
            m_addr = reinterpret_cast<void*>(start);
            m_size = end - start;
            
            m_orig = LookupPerm(addr);
        }
        
        ProtGuard(void* addr, size_t sz) noexcept {
            (*this)(addr, sz);
        }
        
        ~ProtGuard() noexcept {
            if (m_addr && m_size > 0) {
                mprotect(m_addr, m_size, m_orig);
            }
        }
        
        ZUMTLib_NODISCARD bool make(const Prot_t prot) const noexcept {
            if (!m_addr || m_size == 0) return false;
            return mprotect(m_addr, m_size, prot) == 0;
        }
        
        void RefreshTblManual() {
            RefreshTbl();
        }
        
        ProtGuard(const ProtGuard&)  = delete;
        ProtGuard& operator=(const ProtGuard&) = delete;
        
        ProtGuard(ProtGuard&& other) noexcept
            : m_addr(other.m_addr), m_size(other.m_size), m_orig(other.m_orig),
              m_tbl(std::move(other.m_tbl)), m_tblValid(other.m_tblValid)
        {
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
        Addr() noexcept : m_addr(0) {}
        explicit Addr(void* ptr) noexcept : m_addr(reinterpret_cast<base_type>(ptr)) {}
        explicit Addr(const base_type addr) noexcept : m_addr(addr) {}
        explicit Addr(const String_t& stringAddr) : m_addr(std::stoull(stringAddr)) {}
        template<size_t bit>
        explicit Addr(const PtrLow<bit>& obj) noexcept : Addr(obj.value()) {}
        
        bool writeGuard(const void* buffer, std::size_t length) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;
            
            ProtGuard guard(ptr(), length);
            
            if (!guard.make(PROT_READ | PROT_WRITE | PROT_EXEC))
                return false;
            
            std::memcpy(ptr(), buffer, length);
            
            return true;
        }
        
        bool write(const void* buffer, std::size_t length) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;
            
            if (mprotect(ptr(), length, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
                return false;
            
            std::memcpy(ptr(), buffer, length);
            
            return true;
        }
        
        bool read(void* buffer, const std::size_t length) const noexcept {
            if (!m_addr || !buffer || length == 0) return false;
            
            std::memcpy(buffer, ptr(), length);
            return true;
        }
        
        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        bool writeType(const _Ty& value) const noexcept {
            return write(&value, _Sz);
        }
        
        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        bool writeTypeGuard(const _Ty& value) const noexcept {
            return writeGuard(&value, _Sz);
        }
        
        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        _Ty readType() const noexcept {
            _Ty result{};
            read(reinterpret_cast<void*>(&result), _Sz);
            return result;
        }
        
        template<typename _Ty, std::size_t _Sz = sizeof(_Ty)> // NOLINT(*-reserved-identifier)
        bool readType(_Ty* value) const noexcept {
            return read(reinterpret_cast<void*>(value), _Sz);
        }
        
        ZUMTLib_NODISCARD base_type address() const noexcept {
            return m_addr;
        }
        
        ZUMTLib_NODISCARD void* ptr() const noexcept {
            return reinterpret_cast<void*>(m_addr);
        }
        
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
        
        const Module& Scan(const String_t& targetName, bool merge = false) noexcept {
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
                        
                        size_t dash = addr.find('-');
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
        
        explicit Module(const String_t& targetName, Proc_t* proc = nullptr, bool merge = false) noexcept {
            m_proc = proc;
            Scan(targetName, merge);
        }
        
        void operator()(const String_t& targetName, Proc_t* proc = nullptr, bool merge = false) noexcept {
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
            Proc_t* proc = nullptr
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
            AddrRange range = {},
            String_t perms = {},
            Offset_t offset = {},
            String_t dev = {},
            String_t path = {},
            Proc_t* proc = {}
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
    
    inline ModuleList GetModuleList(bool merge = false, Proc_t* proc = {}) {
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
                size_t pos = path.find_first_not_of(' ');
                if (pos == String_t::npos) continue;
                path = path.substr(pos);
                if (path.empty()) continue;
                
                size_t dash = addr.find('-');
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
    
    inline MapRegions ParseMaps(Proc_t* proc = {}) {
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
            regions.push_back({Addr(start), Addr(end), perms});
        }
        
        return regions;
    }
    
    inline bool IsReadable(Address_t addr, const MapRegions& maps) {
        for (const auto& r : maps) {
            if (addr >= r.start.address() && addr < r.end.address()) {
                return !r.perms.empty() && r.perms[0] == 'r';
            }
        }
        return false;
    }
    
    inline bool Dump(
        const AddrRange& range,
        const String_t& outPath,
        Addr::base_type chunkSize = 0
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
            size_t remaining = range.end.address() - current.address();
            size_t size = std::min(chunkSize, remaining);
            
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
                std::memcpy(buffer.data(), ptr, size);
                ofs.write(buffer.data(), static_cast<std::streamsize>(size));
            } else {
                ofs.write(zeros.data(), static_cast<std::streamsize>(size));
            }
            
            current += size;
        }
        
        return true;
    }
    
    inline bool IsPtrValid(void* ptr, size_t size = 1) noexcept {
        if (!ptr) return false;
        
        const auto start = reinterpret_cast<Address_t>(ptr);
        const auto end = start + size;
        
        long ps = PageSize();
        
        for (Address_t page = start & ~(ps - 1); page < end; page += ps) {
            unsigned char vec;
            const int ret = mincore(reinterpret_cast<void*>(page), ps, &vec);
            if (ret != 0 || (vec & 1) == 0) {
                return false;
            }
        }
        return true;
    }
    
    inline bool IsSafeAddress(Address_t addr, size_t size) noexcept {
        if (addr <= 0x10000000 || addr >= 0x10000000000) return false;
        return IsPtrValid(reinterpret_cast<void*>(addr), size);
    }
    
    inline bool IsLibraryLoaded(const String_t& name, Proc_t* proc = {}) {
        std::ifstream mapsFile(proc ? proc->maps : self_maps);
        if (!mapsFile.is_open()) return false;
        String_t line;
        while (std::getline(mapsFile, line))
            if (line.find(name) != String_t::npos) return true;
        return false;
    }
    
    
    inline Address_t GetModuleBase(String_t name, Proc_t* proc = {}) {
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
    
    inline Address_t GetModuleEnd(String_t name, Proc_t* proc = {}) {
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
    
    template<typename Rep, typename Period>
    bool AskLibBase(
        const char* libName,
        Address_t* outBasePtr,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        while (!*outBasePtr) {
            *outBasePtr = GetModuleBase(libName);
            if (*outBasePtr) return true;
            if (sleep_time.count() != 0)
                std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        }
    }
    
    template<typename Rep, typename Period>
    bool AskLibBase(
        const std::initializer_list<
            std::pair<const char*, Address_t*>
        >& libs,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        for (auto& [libName, outBasePtr]: libs)
            AskLibBase(libName, outBasePtr, sleep_time);
        return true;
    }
    
    template<typename Rep, typename Period>
    bool AskLibEnd(
        const char* libName,
        Address_t* outBasePtr,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        while (!*outBasePtr) {
            *outBasePtr = GetModuleEnd(libName);
            if (*outBasePtr) return true;
            if (sleep_time.count() != 0)
                std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        }
    }
    
    template<typename Rep, typename Period>
    bool AskLibEnd(
        const std::initializer_list<
            std::pair<const char*, Address_t*>
        >& libs,
        const std::chrono::duration<Rep, Period>& sleep_time = std::chrono::seconds(1)
    ) {
        for (auto& [libName, outBasePtr]: libs)
            AskLibEnd(libName, outBasePtr, sleep_time);
        return true;
    }
    
    inline String_t ReadCmdlineName(bool stop_at_colon = false,  Proc_t* proc = {}) {
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
    
    
}
#pragma clang diagnostic pop

#endif //ZUMTLib_HPP
