#include "zygisk.hpp"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <android/log.h>
#include <cstring>
#include <cstdio>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Kirin9000S", __VA_ARGS__)

using namespace zygisk;

// 完整的伪造 cpuinfo 内容（麒麟 9000S）
static const char* FAKE_CPUINFO = 
    "Processor\t: AArch64 Processor rev 0 (aarch64)\n"
    "Features\t: fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp\n"
    "CPU implementer\t: 0x48\n"
    "CPU architecture: 8\n"
    "CPU variant\t: 0x1\n"
    "CPU part\t: 0xd0c\n"
    "CPU revision\t: 0\n"
    "processor\t: 0\n"
    "BogoMIPS\t: 26.00\n"
    "CPU implementer\t: 0x48\n"
    "CPU architecture: 8\n"
    "CPU variant\t: 0x1\n"
    "CPU part\t: 0xd0c\n"
    "CPU revision\t: 0\n"
    "Hardware\t: HiSilicon Kirin 9000S\n";

static int (*orig_open)(const char*, int, mode_t) = nullptr;
static FILE* (*orig_fopen)(const char*, const char*) = nullptr;
static ssize_t (*orig_read)(int, void*, size_t) = nullptr;
static size_t (*orig_fread)(void*, size_t, size_t, FILE*) = nullptr;
static char* (*orig_fgets)(char*, int, FILE*) = nullptr;

// 拦截 open：返回一个包含假数据的 pipe fd
static int fake_open(const char* path, int flags, mode_t mode) {
    if (path && strstr(path, "/proc/cpuinfo")) {
        LOGD("Intercepted open(%s)", path);
        int pipefd[2];
        if (pipe(pipefd) == 0) {
            write(pipefd[1], FAKE_CPUINFO, strlen(FAKE_CPUINFO));
            close(pipefd[1]);
            return pipefd[0];
        }
        return -1;
    }
    return orig_open(path, flags, mode);
}

// 拦截 fopen：返回一个内存流
static FILE* fake_fopen(const char* path, const char* mode) {
    if (path && strstr(path, "/proc/cpuinfo")) {
        LOGD("Intercepted fopen(%s)", path);
        return fmemopen((void*)FAKE_CPUINFO, strlen(FAKE_CPUINFO), "r");
    }
    return orig_fopen(path, mode);
}

// 可选：拦截 fgets / fread（防止一些应用直接读 FILE*）
static char* fake_fgets(char* buf, int size, FILE* fp) {
    // 不做额外处理，因为 fmemopen 已经能正常工作
    return orig_fgets(buf, size, fp);
}

static size_t fake_fread(void* ptr, size_t size, size_t nmemb, FILE* fp) {
    return orig_fread(ptr, size, nmemb, fp);
}

class KirinSpoofModule : public ModuleBase {
private:
    Api* api = nullptr;

public:
    void onLoad(Api* api, JNIEnv* env) override {
        this->api = api;
        LOGD("Kirin9000S Spoofer loaded (Zygisk Next Hook Mode)");
    }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        // Zygisk Next 下不在 pre 中做任何事
    }

    void postAppSpecialize(const AppSpecializeArgs* args) override {
        const char* process = api->getProcessName();
        LOGD("postAppSpecialize: %s", process);
        
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) return;
        
        LOGD("*** TARGET DETECTED: Installing hooks ***");
        
        // 注册 PLT hooks
        api->pltHookRegister(".*libc\\.so$", "open", (void*)fake_open, (void**)&orig_open);
        api->pltHookRegister(".*libc\\.so$", "fopen", (void*)fake_fopen, (void**)&orig_fopen);
        api->pltHookRegister(".*libc\\.so$", "fgets", (void*)fake_fgets, (void**)&orig_fgets);
        api->pltHookRegister(".*libc\\.so$", "fread", (void*)fake_fread, (void**)&orig_fread);
        api->pltHookCommit();
        
        LOGD("*** Hooks installed for %s ***", process);
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
