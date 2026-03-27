#include <iostream>
#include <cinttypes>
#include <cstdarg>
#include <dlfcn.h>
#include "ZUMTLib/ZUMTLib.hpp"

std::vector<uintptr_t> protected_addresses{};
long long TestEditVal = 123456789LL;

void check_mem_permissions(void* ptr) {
    std::ifstream maps("/proc/self/maps");
    std::string line;
    uintptr_t start, end;
    const auto address = reinterpret_cast<uintptr_t>(ptr);
    char perm[5];

    while (std::getline(maps, line)) {
        if (sscanf(line.c_str(), "%lx-%lx %4s", &start, &end, perm) == 3) {
            if (address >= start && address < end) {
                std::cout << "权限: \"" << perm << "\"" << std::endl;
                return;
            }
        }
    }
    std::cout << "找不到maps中的地址" << std::endl;
}

using memcpy_t = void* (*)(void*, const void*, size_t);
extern "C" void* memcpy(void* dst, const void* src, size_t n) {
    static memcpy_t orig_memcpy = nullptr;

    if (!orig_memcpy) {
        orig_memcpy = reinterpret_cast<memcpy_t>(dlsym(RTLD_NEXT, "memcpy"));
    }

    if (std::any_of(
            protected_addresses.begin(),
            protected_addresses.end(),
            [dst, src](const uintptr_t x) {
                return
                    x == reinterpret_cast<uintptr_t>(dst) ||
                    x == reinterpret_cast<uintptr_t>(src);
            }
        )) {
        printf("敏感 memcpy 操作被发现:\n\t[\n\t\t复制到:0x%" PRIXPTR "\n\t\t复制自:0x%" PRIXPTR "\n\t\t大小%zu字节\n\t]\n", reinterpret_cast<uintptr_t>(dst), reinterpret_cast<uintptr_t>(src), n);
    }

    return orig_memcpy(dst, src, n);
}

void* get_mem(const long ps = ZUMTLib::PageSize()) {
    return mmap(nullptr, ps,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
}


int wr_test() {
    using test_type = int;
    const long ps = ZUMTLib::PageSize();

    {
        std::cout << "// =============== PART 1 =============== //" << std::endl;

        ZUMTLib::asm_cfg_t asm_cfg{};
        asm_cfg.memcpy = true;
        asm_cfg.mprotect = true;
        asm_cfg.syscall = true;

        void* mem = get_mem(ps);
        protected_addresses.push_back(reinterpret_cast<uintptr_t>(mem)); // 监测内存
        const auto val_ptr = static_cast<test_type*>(mem);
        *val_ptr = 0;
        const ZUMTLib::Addr val_addr(val_ptr);
        mprotect(mem, ps, PROT_READ);

        ZUMTLib::ProtGuard guard(val_ptr, sizeof(test_type));

        std::cout << "地址:0x" << std::hex << std::uppercase << *val_addr << std::endl;
        std::cout << std::dec;

        check_mem_permissions(val_ptr);

        const auto orig_val = val_addr.read<test_type>(asm_cfg); // 不会被hooked_memcpy发现
        std::cout << "读取值:" << orig_val << std::endl;

        std::cout << "修改操作:" << std::boolalpha << val_addr.writeHex("FF FF FF FF", asm_cfg) << std::endl;
        std::cout << "修改后:" << *val_ptr << std::endl;

        std::cout << "还原操作:" << val_addr.write<test_type>(orig_val, asm_cfg) << std::endl;
        std::cout << "还原后:" << *val_ptr << std::endl;

        guard.Restore();

        check_mem_permissions(val_ptr);

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 2 =============== //" << std::endl;

        void* mem = get_mem(ps);
        protected_addresses.push_back(reinterpret_cast<uintptr_t>(mem)); // 监测内存
        const auto val_ptr = static_cast<test_type*>(mem);
        *val_ptr = 0;
        const ZUMTLib::Addr val_addr(val_ptr);
        mprotect(mem, ps, PROT_READ);

        std::cout << "地址:0x" << std::hex << std::uppercase << *val_addr << std::endl;
        std::cout << std::dec;

        check_mem_permissions(val_ptr);

        const auto orig_val = val_addr.read<test_type>(); // 会被hooked_memcpy发现
        std::cout << "读取值:" << orig_val << std::endl;

        std::cout << "修改操作:" << std::boolalpha << val_addr.writeHex("FF FF FF FF") << std::endl;
        std::cout << "修改后:" << *val_ptr << std::endl;

        std::cout << "还原操作:" << val_addr.write<test_type>(orig_val) << std::endl;
        std::cout << "还原后:" << *val_ptr << std::endl;

        check_mem_permissions(val_ptr);

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 3 =============== //" << std::endl;

        const auto real_addr = reinterpret_cast<ZUMTLib::Address_t>(&TestEditVal);
        const ZUMTLib::Address_t exe_base = ZUMTLib::GetModuleBase("ZUMTLib_example");

        std::cout << "真实的地址:0x" << std::hex << std::uppercase << real_addr  << std::endl;
        std::cout << "程序的基址:0x" << std::hex << std::uppercase << exe_base << std::endl;
        std::cout << "真实的偏移:0x" << std::hex << std::uppercase << real_addr - exe_base << std::endl;

        const ZUMTLib::Addr val_addr(exe_base + 0x12010 /* TestEditVal */);
        std::cout << "偏移后地址:0x" << std::hex << std::uppercase << *val_addr << std::endl;
        std::cout << std::dec;

        const auto real_orig_val = TestEditVal;
        std::cout << "原始值:" << real_orig_val << std::endl;

        const ZUMTLib::Bytes_t orig_val = val_addr.read<8>();
        std::cout << "读取值:" << ZUMTLib::Bytes2Hex(orig_val) << std::endl;

        const ZUMTLib::Bytes_t reversed(orig_val.crbegin(), orig_val.crend());
        if (val_addr.writeFrom(reversed)) {
            std::cout << "写入值:" << ZUMTLib::Bytes2Hex(reversed) << std::endl;
        }

        const auto real_val = TestEditVal;
        std::cout << "现在值:" << real_val << std::endl;

        const auto added_val = real_orig_val + real_val;
        std::cout << "相加值:" << added_val << std::endl;

        std::cout << "// ================= END ================= //" << std::endl;
    }

    return 0;
}

int main() {
    return wr_test();
}