// language: C++, file: jni_entry.cpp, runtime: Android NDK (ARM64), target: libff_native.so
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>

#define LOG_TAG "ff_native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class MemoryReader {
public:
    virtual ~MemoryReader() = default;
    virtual bool read_bytes(std::uintptr_t addr, void* out, std::size_t size) const = 0;
    template <typename T>
    bool read(std::uintptr_t addr, T& out) const {
        return read_bytes(addr, &out, sizeof(T));
    }
    static bool is_valid_ptr(std::uintptr_t ptr) {
        return ptr >= 0x10000ULL && ptr <= 0x00007FFFFFFFFFFFULL && (ptr & 0x7ULL) == 0;
    }
};

class LocalProcessReader final : public MemoryReader {
public:
    bool read_bytes(std::uintptr_t addr, void* out, std::size_t size) const override {
        if (!out || size == 0 || !is_valid_ptr(addr)) {
            return false;
        }
        std::memcpy(out, reinterpret_cast<const void*>(addr), size);
        return true;
    }
};

static std::uintptr_t find_module_base_arm64(const char* mod_name) {
    FILE* fp = std::fopen("/proc/self/maps", "r");
    if (!fp) {
        return 0;
    }
    char line[512];
    std::uintptr_t base = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        if (std::strstr(line, mod_name)) {
            unsigned long long start = 0;
            if (std::sscanf(line, "%llx-", &start) == 1) {
                base = static_cast<std::uintptr_t>(start);
                break;
            }
        }
    }
    std::fclose(fp);
    return base;
}

static void* worker_thread(void*) {
    LocalProcessReader reader;
    std::uintptr_t il2cpp_base = 0;
    while (il2cpp_base == 0) {
        il2cpp_base = find_module_base_arm64("libil2cpp.so");
        usleep(500000);
    }
    LOGI("libil2cpp.so base=0x%llx reader=%p",
         static_cast<unsigned long long>(il2cpp_base),
         static_cast<const void*>(&reader));
    while (true) {
        // Resolve live singleton/matrix pointers here per client build before invoking FreeFireEngine.
        usleep(16000);
    }
    return nullptr;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    if (!vm || vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    pthread_t tid{};
    if (pthread_create(&tid, nullptr, worker_thread, nullptr) != 0) {
        return JNI_ERR;
    }
    pthread_detach(tid);
    return JNI_VERSION_1_6;
}
