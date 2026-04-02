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

struct Base { // NOLINT
    virtual void foo() { std::cout << "Base::foo\n"; } // NOLINT
    virtual void bar() { std::cout << "Base::bar\n"; } // NOLINT
};

void new_foo() { std::cout << "Hooked foo!\n"; }

int wr_test() {
    using test_type = int;
    const long ps = ZUMTLib::PageSize();

    using ZUMTConfig;

    {
        std::cout << "// =============== PART 1 =============== //" << std::endl;
        std::cout << "// ============= 保护读写测试 ============= //" << std::endl;

        constexpr asm_cfg_t my_cfg = asm_memcpy | asm_syscall | asm_mprotect;

        void* mem = get_mem(ps);
        protected_addresses.push_back(reinterpret_cast<uintptr_t>(mem)); // 监测内存
        const auto val_ptr = static_cast<test_type*>(mem);
        *val_ptr = 1234;
        const ZUMTLib::Addr val_addr(val_ptr);

        mprotect(mem, ps, PROT_READ);

        std::cout << "地址:0x" << std::hex << std::uppercase << ~val_addr << std::endl;
        std::cout << std::dec;

        check_mem_permissions(val_ptr);

        const auto orig_val = val_addr.read<test_type>(my_cfg); // 不会被hooked_memcpy发现
        std::cout << "读取值:" << orig_val << std::endl;

        std::cout << "修改操作:" << std::boolalpha << val_addr.writeGuardHex("A2 5D 4F 00", my_cfg) << std::endl;
        std::cout << "修改后:" << *val_ptr << std::endl;

        std::cout << "还原操作:" << val_addr.writeGuard<test_type>(orig_val, my_cfg) << std::endl;
        std::cout << "还原后:" << *val_ptr << std::endl;

        check_mem_permissions(val_ptr);

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 2 =============== //" << std::endl;
        std::cout << "// ============ 无保护读写测试 ============ //" << std::endl;

        void* mem = get_mem(ps);
        protected_addresses.push_back(reinterpret_cast<uintptr_t>(mem)); // 监测内存
        const auto val_ptr = static_cast<test_type*>(mem);
        *val_ptr = 1234;
        const ZUMTLib::Addr val_addr(val_ptr);
        mprotect(mem, ps, PROT_READ);

        std::cout << "地址:0x" << std::hex << std::uppercase << ~val_addr << std::endl;
        std::cout << std::dec;

        check_mem_permissions(val_ptr);

        const auto orig_val = val_addr.read<test_type>(); // 会被hooked_memcpy发现
        std::cout << "读取值:" << orig_val << std::endl;

        std::cout << "修改操作:" << std::boolalpha << val_addr.writeHex("A2 5D 4F 00") << std::endl;
        std::cout << "修改后:" << *val_ptr << std::endl;

        std::cout << "还原操作:" << val_addr.write<test_type>(orig_val) << std::endl;
        std::cout << "还原后:" << *val_ptr << std::endl;

        check_mem_permissions(val_ptr);

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 3 =============== //" << std::endl;
        std::cout << "// =========== 基址偏移读写测试 =========== //" << std::endl;

        const auto real_addr = reinterpret_cast<ZUMTLib::Address_t>(&TestEditVal);
        const ZUMTLib::Address_t exe_base = ZUMTLib::GetModuleBase("ZUMTLib_example");

        constexpr ZUMTLib::Addr my_offset(0x13010);

        std::cout << "真实的地址:0x" << std::hex << std::uppercase << real_addr  << std::endl;
        std::cout << "程序的基址:0x" << std::hex << std::uppercase << exe_base << std::endl;
        std::cout << "真实的偏移:0x" << std::hex << std::uppercase << real_addr - exe_base << std::endl;
        std::cout << "写入的偏移:0x" << std::hex << std::uppercase << ~my_offset << std::endl;

        const ZUMTLib::Addr val_addr(exe_base + ~my_offset /* TestEditVal */);
        std::cout << "偏移后地址:0x" << std::hex << std::uppercase << ~val_addr << std::endl;
        std::cout << std::dec;

        const auto& bind_val = TestEditVal;
        std::cout << "原始值:" << bind_val << std::endl;

        const ZUMTLib::Bytes_t orig_val = val_addr.read<8>();
        std::cout << "读取值:" << ZUMTLib::Bytes2Hex(orig_val) << std::endl;

        const ZUMTLib::Bytes_t reversed(orig_val.crbegin(), orig_val.crend()); // 反转字节
        if (val_addr.writeFrom(reversed)) {
            std::cout << "写入值:" << ZUMTLib::Bytes2Hex(reversed) << std::endl;
        }

        std::cout << "现在值:" << bind_val << std::endl;

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 4 =============== //" << std::endl;
        std::cout << "// ========== 虚函数手动Hook测试 ========== //" << std::endl;

        Base obj;

        void(*old_func_ptr)(Base*) = {};

        std::cout << "Hook前: ";
        ZUMTLib::PolyCall(obj)->foo();

        ZUMTLib::VTable(&obj)[0].hook(
            reinterpret_cast<void*>(&new_foo),
            reinterpret_cast<void**>(&old_func_ptr)
        );

        std::cout << "Hook后: ";
        ZUMTLib::PolyCall(obj)->foo();

        std::cout << "原函数: ";
        old_func_ptr(&obj);

        ZUMTLib::VTable(&obj)[0].hook(reinterpret_cast<void*>(old_func_ptr)); // 手动还原

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 5 =============== //" << std::endl;
        std::cout << "// ========== 虚函数自动Hook测试 ========== //" << std::endl;

        Base obj;

        {
            std::cout << "Hook前: ";
            ZUMTLib::PolyCall(obj)->foo();
            {
                const ZUMTLib::VFuncHookRAII hook{ZUMTLib::VTable(&obj)[0], reinterpret_cast<void*>(&new_foo)};
                std::cout << "Hook后: ";
                ZUMTLib::PolyCall(obj)->foo();
                std::cout << "原函数: " << hook.osub() << std::endl;
            }
            std::cout << "自动还原后: ";
            ZUMTLib::PolyCall(obj)->foo();
        }

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 6 =============== //" << std::endl;
        std::cout << "// ========= 虚函数手动延迟Hook测试 ========= //" << std::endl;

        Base obj;

        {
            std::cout << "构造前: ";
            ZUMTLib::PolyCall(obj)->foo();
            {
                ZUMTLib::VFuncHookRAII hook{ZUMTLib::VTable(&obj)[0]};
                std::cout << "Hook前: ";
                ZUMTLib::PolyCall(obj)->foo();
                hook.Hook(reinterpret_cast<void*>(&new_foo));
                std::cout << "Hook后: ";
                ZUMTLib::PolyCall(obj)->foo();
                hook.DeHook();
                std::cout << "DeHook后: ";
                ZUMTLib::PolyCall(obj)->foo();
                hook.Hook(reinterpret_cast<void*>(&new_foo));
                std::cout << "Hook后: ";
                ZUMTLib::PolyCall(obj)->foo();
                std::cout << "原函数: " << hook.osub() << std::endl;
            }
            std::cout << "自动还原后: ";
            ZUMTLib::PolyCall(obj)->foo();
        }

        std::cout << "// ================= END ================= //" << std::endl;
    }
    return 0;
}

int main() {
    return wr_test();
}