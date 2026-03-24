#include <iostream>
#include <cinttypes>
#include <cstdarg>
#include <dlfcn.h>
#include "ZUMTLib/ZUMTLib.hpp"

bool HookedMemcpy;
class HookMemcpy {
    bool orig;
public:
    HookMemcpy() : orig(HookedMemcpy) {
        HookedMemcpy = true;
    }
    ~HookMemcpy() {
        HookedMemcpy = orig;
    }
};
class DeHookMemcpy {
    bool orig;
public:
    DeHookMemcpy() : orig(HookedMemcpy) {
        HookedMemcpy = false;
    }
    ~DeHookMemcpy() {
        HookedMemcpy = orig;
    }
};

void check_mem_permissions(void* ptr) {
    DeHookMemcpy DeHookMemcpy_mgr;

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

    if (HookedMemcpy) {
        printf("memcpy 操作被发现:\n\t[\n\t\t复制到:0x%" PRIXPTR "\n\t\t复制自:0x%" PRIXPTR "\n\t\t大小%zu字节\n\t]\n", reinterpret_cast<uintptr_t>(dst), reinterpret_cast<uintptr_t>(src), n);
    }

    return orig_memcpy(dst, src, n);
}

void* get_mem(const long ps = ZUMTLib::PageSize()) {
    DeHookMemcpy DeHookMemcpy_mgr;
    return mmap(nullptr, ps,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
}


int main() {
    HookMemcpy HookMemcpy_mgr;
    using test_type = int;
    const long ps = ZUMTLib::PageSize();

    {
        std::cout << "// =============== PART 1 =============== //" << std::endl;

        void* mem = get_mem(ps);
        const auto val_ptr = static_cast<test_type*>(mem);
        *val_ptr = 0;
        const ZUMTLib::Addr val_addr(val_ptr);
        mprotect(mem, ps, PROT_READ);

        const bool in_orig_HookedMemcpy = HookedMemcpy;
        HookedMemcpy = false;
        ZUMTLib::ProtGuard guard(val_ptr, sizeof(test_type));
        HookedMemcpy = in_orig_HookedMemcpy;

        std::cout << "地址:0x" << std::hex << std::uppercase << *val_addr << std::endl;
        std::cout << std::dec;

        check_mem_permissions(val_ptr);

        const auto orig_val = val_addr.readType<test_type>(true); // 不会被hooked_memcpy发现
        std::cout << "读取值:" << orig_val << std::endl;

        std::cout << "修改操作:" << std::boolalpha << val_addr.write("FF FF FF FF", true) << std::endl;
        std::cout << "修改后:" << *val_ptr << std::endl;

        std::cout << "还原操作:" << val_addr.writeType<test_type>(orig_val, true) << std::endl;
        std::cout << "还原后:" << *val_ptr << std::endl;

        const bool out_orig_HookedMemcpy = HookedMemcpy;
        HookedMemcpy = false;
        guard.Restore();
        HookedMemcpy = out_orig_HookedMemcpy;

        check_mem_permissions(val_ptr);

        std::cout << "// ================= END ================= //" << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "// =============== PART 2 =============== //" << std::endl;

        void* mem = get_mem(ps);
        const auto val_ptr = static_cast<test_type*>(mem);
        *val_ptr = 0;
        const ZUMTLib::Addr val_addr(val_ptr);
        mprotect(mem, ps, PROT_READ);

        std::cout << "地址:0x" << std::hex << std::uppercase << *val_addr << std::endl;
        std::cout << std::dec;

        check_mem_permissions(val_ptr);

        const auto orig_val = val_addr.readType<test_type>(); // 会被hooked_memcpy发现
        std::cout << "读取值:" << orig_val << std::endl;

        std::cout << "修改操作:" << std::boolalpha << val_addr.write("FF FF FF FF") << std::endl;
        std::cout << "修改后:" << *val_ptr << std::endl;

        std::cout << "还原操作:" << val_addr.writeType<test_type>(orig_val) << std::endl;
        std::cout << "还原后:" << *val_ptr << std::endl;

        check_mem_permissions(val_ptr);

        std::cout << "// ================= END ================= //" << std::endl;
    }

    return 0;
}
