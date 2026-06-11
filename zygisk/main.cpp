#include "zygisk.hpp"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <android/log.h>
#include <cstring>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Kirin9000S", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Kirin9000S", __VA_ARGS__)

using namespace zygisk;

static unsigned long (*orig_getauxval)(unsigned long) = nullptr;

static unsigned long fake_getauxval(unsigned long type) {
    if (type == 16) return 0x7efefeff;   // AT_HWCAP 麒麟特征
    if (type == 26) return 0x0000001f;   // AT_HWCAP2
    return orig_getauxval ? orig_getauxval(type) : 0;
}

class KirinSpoofModule : public ModuleBase {
private:
    Api* api = nullptr;
    const char* module_path = "/data/adb/modules/kirin9000s_spoofer";
    bool is_target = false;

public:
    void onLoad(Api* api, JNIEnv* env) override {
        this->api = api;
        LOGD("Kirin9000S Spoofer loaded");
    }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        const char* process = api->getProcessName();
        LOGD("preAppSpecialize: %s", process);
        
        if (strcmp(process, "com.tencent.tmgp.dfm") == 0) {
            is_target = true;
            LOGD("*** TARGET DETECTED: %s ***", process);
            
            // 尝试挂载（在 zygote 阶段）
            do_mount();
        }
    }

    void postAppSpecialize(const AppSpecializeArgs* args) override {
        const char* process = api->getProcessName();
        LOGD("postAppSpecialize: %s", process);
        
        if (!is_target) return;
        
        LOGD("*** POST SPECIALIZE: remounting for %s ***", process);
        
        // 再次挂载，确保对应用进程生效（关键！）
        do_mount();
        
        // Hook getauxval（防止 HWCAP 泄露）
        api->pltHookRegister(".*libc\\.so$", "getauxval", (void*)fake_getauxval, (void**)&orig_getauxval);
        api->pltHookCommit();
        
        // 验证挂载是否成功
        verify_mount();
    }

private:
    void do_mount() {
        char fake_cpuinfo[256];
        snprintf(fake_cpuinfo, sizeof(fake_cpuinfo), "%s/cpuinfo", module_path);
        
        if (access(fake_cpuinfo, F_OK) != 0) {
            LOGE("Fake cpuinfo not found: %s", fake_cpuinfo);
            return;
        }
        
        // 先尝试强制卸载已有挂载
        umount2("/proc/cpuinfo", MNT_DETACH);
        
        // 绑定挂载
        if (mount(fake_cpuinfo, "/proc/cpuinfo", nullptr, MS_BIND, nullptr) == 0) {
            LOGD("[+] Mount success: %s -> /proc/cpuinfo", fake_cpuinfo);
        } else {
            LOGE("[-] Mount failed: %s", strerror(errno));
        }
        
        // 额外挂载 midr_el1（如果存在）
        char fake_midr[256];
        snprintf(fake_midr, sizeof(fake_midr), "%s/midr_el1", module_path);
        const char* midr_target = "/sys/devices/system/cpu/cpu0/regs/identification/midr_el1";
        if (access(fake_midr, F_OK) == 0 && access(midr_target, F_OK) == 0) {
            mount(fake_midr, midr_target, nullptr, MS_BIND, nullptr);
        }
    }
    
    void verify_mount() {
        FILE* fp = fopen("/proc/cpuinfo", "r");
        if (!fp) {
            LOGE("Cannot open /proc/cpuinfo for verification");
            return;
        }
        
        char line[256];
        bool found = false;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "Kirin 9000S")) {
                found = true;
                break;
            }
        }
        fclose(fp);
        
        if (found) {
            LOGD("*** VERIFICATION: /proc/cpuinfo shows Kirin 9000S ✅ ***");
        } else {
            LOGE("*** VERIFICATION: /proc/cpuinfo is NOT spoofed ❌ ***");
            // 再次尝试挂载
            do_mount();
        }
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
