#include <zygisk.hpp>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <android/log.h>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <link.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Kirin9000S", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Kirin9000S", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

static const char *TARGET_PACKAGES[] = {"com.tencent.tmgp.dfm"};
static constexpr int TARGET_COUNT = sizeof(TARGET_PACKAGES) / sizeof(TARGET_PACKAGES[0]);

static constexpr unsigned long FAKE_HWCAP  = 0x7efefeff;
static constexpr unsigned long FAKE_HWCAP2 = 0x0000001f;

static unsigned long (*orig_getauxval)(unsigned long) = nullptr;
static int (*orig_system_property_get)(const char*, char*) = nullptr;

// ------------------------------------------------------------
// 挂载伪造文件 (bind mount)
// ------------------------------------------------------------
static void bind_mount_fake_file(const char *target, const char *fake_path) {
    if (mount(fake_path, target, nullptr, MS_BIND, nullptr) == 0) {
        LOGD("[+] bind mount success: %s -> %s", fake_path, target);
    } else {
        LOGE("[-] bind mount failed: %s", target);
    }
}

static void install_fake_files() {
    char module_path[256];
    snprintf(module_path, sizeof(module_path), "/data/adb/modules/kirin9000s_spoofer");
    
    char fake_cpuinfo[512], fake_midr[512];
    snprintf(fake_cpuinfo, sizeof(fake_cpuinfo), "%s/cpuinfo", module_path);
    snprintf(fake_midr,   sizeof(fake_midr),   "%s/midr_el1", module_path);
    
    if (access(fake_cpuinfo, F_OK) != 0 || access(fake_midr, F_OK) != 0) {
        LOGE("fake files not found, maybe module not installed correctly");
        return;
    }
    
    bind_mount_fake_file("/proc/cpuinfo", fake_cpuinfo);
    
    const char *midr_target = "/sys/devices/system/cpu/cpu0/regs/identification/midr_el1";
    if (access(midr_target, F_OK) == 0) {
        bind_mount_fake_file(midr_target, fake_midr);
    } else {
        LOGD("midr_el1 not exist, skip");
    }
}

// ------------------------------------------------------------
// PLT Hook
// ------------------------------------------------------------
static unsigned long fake_getauxval(unsigned long type) {
    if (type == 16) return FAKE_HWCAP;
    if (type == 26) return FAKE_HWCAP2;
    return orig_getauxval ? orig_getauxval(type) : 0;
}

static int fake_system_property_get(const char *name, char *value) {
    if (strstr(name, "ro.board.platform") || strstr(name, "ro.hardware") ||
        strstr(name, "ro.soc.model") || strstr(name, "ro.chipname")) {
        strcpy(value, "kirin9000s");
        return strlen(value);
    }
    if (strstr(name, "ro.mediatek.platform")) {
        value[0] = 0;
        return 0;
    }
    return orig_system_property_get ? orig_system_property_get(name, value) : 0;
}

// 隐藏模块不被 dl_iterate_phdr 枚举
static int fake_dl_iterate_phdr(int (*cb)(struct dl_phdr_info*, size_t, void*), void *data) {
    struct Wrapper {
        int (*orig_cb)(struct dl_phdr_info*, size_t, void*);
        void *orig_data;
    };
    
    auto wrapper = [](struct dl_phdr_info *info, size_t size, void *arg) -> int {
        Wrapper *w = (Wrapper*)arg;
        if (info->dlpi_name && (strstr(info->dlpi_name, "kirin9000s_spoofer") ||
                                strstr(info->dlpi_name, "libkirin9000s_spoofer.so"))) {
            return 0;
        }
        return w->orig_cb(info, size, w->orig_data);
    };
    
    Wrapper w = {cb, data};
    static auto real_dl_iterate_phdr = (decltype(&dl_iterate_phdr))dlsym(RTLD_DEFAULT, "dl_iterate_phdr");
    if (real_dl_iterate_phdr) {
        return real_dl_iterate_phdr(wrapper, &w);
    }
    return dl_iterate_phdr(wrapper, &w);
}

// ------------------------------------------------------------
// Zygisk 模块主类
// ------------------------------------------------------------
class KirinSpoofModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        void *libc = dlopen("libc.so", RTLD_LAZY);
        if (libc) {
            orig_getauxval = (decltype(orig_getauxval))dlsym(libc, "getauxval");
            orig_system_property_get = (decltype(orig_system_property_get))dlsym(libc, "__system_property_get");
            dlclose(libc);
        }
    }
    
    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = api->getProcessName();
        bool is_target = false;
        for (int i = 0; i < TARGET_COUNT; ++i) {
            if (strcmp(process, TARGET_PACKAGES[i]) == 0) {
                is_target = true;
                break;
            }
        }
        if (!is_target) return;
        
        LOGD("Target process detected: %s, start spoofing", process);
        
        install_fake_files();
        
        const char *libc_regex = ".*libc\\.so$";
        api->pltHookRegister(libc_regex, "getauxval", (void*)fake_getauxval, (void**)&orig_getauxval);
        api->pltHookRegister(libc_regex, "__system_property_get", (void*)fake_system_property_get, (void**)&orig_system_property_get);
        api->pltHookRegister(libc_regex, "dl_iterate_phdr", (void*)fake_dl_iterate_phdr, nullptr);
        
        LOGD("Spoofing fully activated for %s", process);
    }
    
private:
    Api *api;
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
