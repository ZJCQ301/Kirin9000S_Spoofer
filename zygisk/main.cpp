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

struct AppSpecializeArgs {
    void* placeholder[8];
};

class KirinSpoofModule : public ModuleBase {
public:
    void onLoad(void* api, JNIEnv* env) override {
        LOGD("Module loaded");
    }

    void preAppSpecialize(void* args) override {
        const char* process = get_process_name();
        LOGD("Process: %s", process);
        
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) return;
        
        LOGD("Target detected! Spoofing CPU info...");
        
        // 挂载伪造的 cpuinfo
        mount_fake_files();
        
        // Hook getauxval
        hook_getauxval();
    }

private:
    const char* get_process_name() {
        FILE* fp = fopen("/proc/self/cmdline", "r");
        if (!fp) return "unknown";
        static char cmdline[256];
        fgets(cmdline, sizeof(cmdline), fp);
        fclose(fp);
        return cmdline;
    }

    void mount_fake_files() {
        const char* fake_cpuinfo = "/data/adb/modules/kirin9000s_spoofer/cpuinfo";
        const char* fake_midr = "/data/adb/modules/kirin9000s_spoofer/midr_el1";
        
        if (access(fake_cpuinfo, F_OK) == 0) {
            if (mount(fake_cpuinfo, "/proc/cpuinfo", nullptr, MS_BIND, nullptr) == 0) {
                LOGD("Successfully mounted fake cpuinfo");
            } else {
                LOGE("Failed to mount cpuinfo");
            }
        } else {
            LOGE("Fake cpuinfo file not found: %s", fake_cpuinfo);
        }
        
        const char* midr_target = "/sys/devices/system/cpu/cpu0/regs/identification/midr_el1";
        if (access(fake_midr, F_OK) == 0 && access(midr_target, F_OK) == 0) {
            mount(fake_midr, midr_target, nullptr, MS_BIND, nullptr);
            LOGD("Mounted fake midr_el1");
        }
    }

    void hook_getauxval() {
        void* libc = dlopen("libc.so", RTLD_LAZY);
        if (!libc) {
            LOGE("Cannot open libc.so");
            return;
        }
        
        // 获取原始函数地址
        void* orig = dlsym(libc, "getauxval");
        if (orig) {
            LOGD("getauxval found at %p", orig);
            // 这里简化处理：实际生产环境需要真正的 hook 框架
            // 由于 Zygisk API 需要额外的头文件，先用日志代替
        }
        dlclose(libc);
        
        LOGD("getauxval hook setup complete (placeholder)");
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
