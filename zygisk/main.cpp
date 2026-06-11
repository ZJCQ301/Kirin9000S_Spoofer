#include "zygisk.hpp"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/mman.h>
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
static bool file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

static void bind_mount(const char* target, const char* source) {
    if (mount(source, target, nullptr, MS_BIND, nullptr) == 0) {
        LOGD("[+] Bind mount: %s -> %s", source, target);
    } else {
        LOGE("[-] Bind mount failed: %s -> %s", source, target);
    }
}

// 伪造文件时间戳 (防止stat检测)
static void fix_file_times(const char* path) {
    struct timespec ts[2];
    ts[0].tv_sec = 1600000000;  // 2020-09-13
    ts[1].tv_sec = 1600000000;
    utimensat(AT_FDCWD, path, ts, 0);
}

// ========== PLT Hook 函数 ==========
static unsigned long fake_getauxval(unsigned long type) {
    if (type == 16) return FAKE_HWCAP;   // AT_HWCAP
    if (type == 26) return FAKE_HWCAP2;  // AT_HWCAP2
    return orig_getauxval ? orig_getauxval(type) : 0;
}

static int fake_system_property_get(const char *name, char *value) {
    // 麒麟9000S 应有的属性
    if (strcmp(name, "ro.board.platform") == 0) { strcpy(value, "kirin9000s"); return strlen(value); }
    if (strcmp(name, "ro.hardware") == 0) { strcpy(value, "kirin9000s"); return strlen(value); }
    if (strcmp(name, "ro.soc.model") == 0) { strcpy(value, "Kirin 9000S"); return strlen(value); }
    if (strcmp(name, "ro.chipname") == 0) { strcpy(value, "Kirin 9000S"); return strlen(value); }
    if (strcmp(name, "ro.product.board") == 0) { strcpy(value, "kirin9000s"); return strlen(value); }
    if (strstr(name, "mediatek") != nullptr) { value[0] = 0; return 0; } // 抹掉联发科
    if (strcmp(name, "ro.vendor.product.cpu.abi") == 0) { strcpy(value, "arm64-v8a"); return strlen(value); }
    
    return orig_system_property_get ? orig_system_property_get(name, value) : 0;
}

// ========== 隐藏模块 so 文件 (从 /proc/self/maps 抹去) ==========
static void hide_self_so() {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return;
    char line[512];
    unsigned long start, end;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libkirin9000s_spoofer.so")) {
            sscanf(line, "%lx-%lx", &start, &end);
            size_t len = end - start;
            // 修改内存权限
            mprotect((void*)(start & ~0xFFF), len + 0x1000, PROT_READ | PROT_WRITE);
            // 覆盖路径名字符串
            for (char* p = (char*)start; p < (char*)end - 21; p++) {
                if (memcmp(p, "libkirin9000s_spoofer", 21) == 0)
                    memcpy(p, "libandroid_runtime.so", 21);
            }
            LOGD("[+] Hidden module from /proc/self/maps");
            break;
        }
    }
    fclose(fp);
}

// ========== 主模块类 ==========
class KirinSpoofModule : public ModuleBase {
private:
    Api* api = nullptr;
    const char* module_path = "/data/adb/modules/kirin9000s_spoofer";
    
public:
    void onLoad(Api* api, JNIEnv* env) override {
        this->api = api;
        LOGD("Kirin9000S Spoofer v2.0 loaded");
        hide_self_so();  // 立即隐藏自身
    }
    
    void preAppSpecialize(AppSpecializeArgs* args) override {
        const char* process = api->getProcessName();
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) return;
        
        LOGD("=== Target detected: %s ===", process);
        
        // 1. 伪造系统属性 (hook)
        api->pltHookRegister(".*libc\\.so$", "getauxval", (void*)fake_getauxval, (void**)&orig_getauxval);
        api->pltHookRegister(".*libc\\.so$", "__system_property_get", (void*)fake_system_property_get, (void**)&orig_system_property_get);
        api->pltHookCommit();
        LOGD("[+] PLT hooks installed");
        
        // 2. 挂载伪造文件
        char fake_cpuinfo[256], fake_midr[256], fake_version[256], fake_mountinfo[256], fake_socid[256];
        snprintf(fake_cpuinfo, sizeof(fake_cpuinfo), "%s/cpuinfo", module_path);
        snprintf(fake_midr, sizeof(fake_midr), "%s/midr_el1", module_path);
        snprintf(fake_version, sizeof(fake_version), "%s/version", module_path);
        snprintf(fake_mountinfo, sizeof(fake_mountinfo), "%s/mountinfo", module_path);
        snprintf(fake_socid, sizeof(fake_socid), "%s/soc_id", module_path);
        
        // 等待文件就绪
        for (int i = 0; i < 10; i++) {
            if (file_exists(fake_cpuinfo)) break;
            usleep(50000);
        }
        
        bind_mount("/proc/cpuinfo", fake_cpuinfo);
        bind_mount("/proc/version", fake_version);
        bind_mount("/proc/self/mountinfo", fake_mountinfo);
        bind_mount("/sys/devices/soc0/soc_id", fake_socid);
        
        // 可选: midr_el1
        if (file_exists("/sys/devices/system/cpu/cpu0/regs/identification/midr_el1")) {
            bind_mount("/sys/devices/system/cpu/cpu0/regs/identification/midr_el1", fake_midr);
        }
        
        // 伪造时间戳
        fix_file_times("/proc/cpuinfo");
        fix_file_times("/proc/version");
        fix_file_times("/proc/self/mountinfo");
        
        LOGD("[+] All fake files mounted and timestamps fixed");
    }
    
    void postAppSpecialize(const AppSpecializeArgs* args) override {
        const char* process = api->getProcessName();
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) return;
        
        // 后挂载：防止竞争条件
        char fake_cpuinfo[256];
        snprintf(fake_cpuinfo, sizeof(fake_cpuinfo), "%s/cpuinfo", module_path);
        if (file_exists(fake_cpuinfo)) {
            mount(fake_cpuinfo, "/proc/cpuinfo", nullptr, MS_BIND, nullptr);
            LOGD("[+] Re-mounted cpuinfo in postAppSpecialize (race condition fix)");
        }
        
        LOGD("=== Spoofing fully activated for %s ===", process);
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
