#include "zygisk.hpp"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <android/log.h>
#include <cstring>
#include <fstream>
#include <sstream>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Kirin9000S", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Kirin9000S", __VA_ARGS__)

using namespace zygisk;

// ========== 伪造的 HWCAP 值 (麒麟9000S特征) ==========
static constexpr unsigned long FAKE_HWCAP  = 0x7efefeff;
static constexpr unsigned long FAKE_HWCAP2 = 0x0000001f;

// ========== 原始函数指针 ==========
static unsigned long (*orig_getauxval)(unsigned long) = nullptr;
static int (*orig_system_property_get)(const char*, char*) = nullptr;

// ========== 辅助函数 ==========
static const char* get_process_name() {
    static char cmdline[256];
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd < 0) return "unknown";
    int len = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    if (len > 0) { cmdline[len] = '\0'; return cmdline; }
    return "unknown";
}

// ========== 1. 挂载伪造文件 (增强版: 重试+后挂载) ==========
static bool mount_fake_cpuinfo(const char* fake_path) {
    for (int i = 0; i < 5; i++) {
        if (mount(fake_path, "/proc/cpuinfo", nullptr, MS_BIND, nullptr) == 0) {
            LOGD("[+] Mounted fake cpuinfo on attempt %d", i+1);
            return true;
        }
        usleep(50000); // 等待50ms
    }
    LOGE("[-] Failed to mount fake cpuinfo after 5 attempts");
    return false;
}

// ========== 2. Hook: getauxval (防止通过syscall绕过) ==========
static unsigned long fake_getauxval(unsigned long type) {
    if (type == 16) return FAKE_HWCAP;   // AT_HWCAP
    if (type == 26) return FAKE_HWCAP2;  // AT_HWCAP2
    return orig_getauxval ? orig_getauxval(type) : 0;
}

// ========== 3. Hook: 系统属性 (全面伪造麒麟9000S) ==========
static int fake_system_property_get(const char *name, char *value) {
    // 麒麟9000S 应有的属性
    if (strcmp(name, "ro.board.platform") == 0) { strcpy(value, "kirin9000s"); return strlen(value); }
    if (strcmp(name, "ro.hardware") == 0) { strcpy(value, "kirin9000s"); return strlen(value); }
    if (strcmp(name, "ro.soc.model") == 0) { strcpy(value, "Kirin 9000S"); return strlen(value); }
    if (strcmp(name, "ro.chipname") == 0) { strcpy(value, "Kirin 9000S"); return strlen(value); }
    if (strcmp(name, "ro.product.board") == 0) { strcpy(value, "kirin9000s"); return strlen(value); }
    if (strcmp(name, "ro.mediatek.platform") == 0) { value[0] = 0; return 0; } // 抹掉联发科
    if (strcmp(name, "ro.vendor.product.cpu.abi") == 0) { strcpy(value, "arm64-v8a"); return strlen(value); }
    
    return orig_system_property_get ? orig_system_property_get(name, value) : 0;
}

// ========== 4. 隐藏挂载点: 过滤 /proc/self/mountinfo ==========
// 注意: 完整实现需要 hook fopen/fgets，这里提供核心过滤逻辑
static bool is_hidden_line(const char* line) {
    return (strstr(line, "/proc/cpuinfo") != nullptr && strstr(line, "bind") != nullptr);
}
// (实际 hook 需要更多代码，为保持编译稳定，此功能作为可选增强)

// ========== 5. 伪装内核版本和SoC信息 ==========
static void mount_extra_fake_files() {
    const char* module_path = "/data/adb/modules/kirin9000s_spoofer";
    char fake_version[512];
    snprintf(fake_version, sizeof(fake_version), "%s/version", module_path);
    
    // 创建假的内核版本文件
    std::ofstream ver(fake_version);
    ver << "Linux version 5.10.43-android12-9-g12345678-abcdefgh (build-user@build-host) "
        << "(clang version 14.0.7, lld 14.0.7) #1 SMP PREEMPT Thu Jun 12 10:00:00 CST 2026\n";
    ver.close();
    chmod(fake_version, 0644);
    
    // 挂载到 /proc/version (如果存在)
    if (access("/proc/version", F_OK) == 0) {
        mount(fake_version, "/proc/version", nullptr, MS_BIND, nullptr);
        LOGD("[+] Mounted fake /proc/version");
    }
}

// ========== Zygisk 模块主类 ==========
class KirinSpoofModule : public ModuleBase {
public:
    void onLoad(void* api, JNIEnv* env) override {
        LOGD("Kirin9000S Spoofer loaded");
        // 获取 libc 函数地址
        void* libc = dlopen("libc.so", RTLD_LAZY);
        if (libc) {
            orig_getauxval = (decltype(orig_getauxval))dlsym(libc, "getauxval");
            orig_system_property_get = (decltype(orig_system_property_get))dlsym(libc, "__system_property_get");
            dlclose(libc);
        }
    }
    
    void preAppSpecialize(void* args) override {
        const char* process = get_process_name();
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) return;
        
        LOGD("=== Target process detected: %s ===", process);
        
        // 1. 挂载伪造 cpuinfo (核心)
        const char* fake_cpuinfo = "/data/adb/modules/kirin9000s_spoofer/cpuinfo";
        if (access(fake_cpuinfo, F_OK) == 0) {
            mount_fake_cpuinfo(fake_cpuinfo);
        } else {
            LOGE("Fake cpuinfo not found at %s", fake_cpuinfo);
        }
        
        // 2. 挂载额外伪造文件 (version, soc)
        mount_extra_fake_files();
        
        // 注意: getauxval hook 需要 Zygisk API 支持，这里保留原函数指针
        // 实际运行时，如果 Zygisk 提供了 pltHookRegister，可以取消下面注释
        /*
        if (api->pltHookRegister) {
            api->pltHookRegister(".*libc\\.so$", "getauxval", (void*)fake_getauxval, (void**)&orig_getauxval);
            api->pltHookRegister(".*libc\\.so$", "__system_property_get", (void*)fake_system_property_get, (void**)&orig_system_property_get);
        }
        */
    }
    
    void postAppSpecialize(const void* args) override {
        const char* process = get_process_name();
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) return;
        
        // 后挂载: 防止竞争条件，再次确保 cpuinfo 被覆盖
        const char* fake_cpuinfo = "/data/adb/modules/kirin9000s_spoofer/cpuinfo";
        if (access(fake_cpuinfo, F_OK) == 0) {
            mount(fake_cpuinfo, "/proc/cpuinfo", nullptr, MS_BIND, nullptr);
            LOGD("[+] Re-mounted fake cpuinfo in postAppSpecialize (anti-race)");
        }
        
        LOGD("=== Spoofing fully activated for %s ===", process);
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
