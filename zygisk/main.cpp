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

class KirinSpoofModule : public ModuleBase {
public:
    void onLoad(void* api, JNIEnv* env) override {
        LOGD("Module loaded successfully");
    }

    void preAppSpecialize(void* args) override {
        const char* process = get_process_name();
        LOGD("Process: %s", process);
        
        if (strcmp(process, "com.tencent.tmgp.dfm") != 0) {
            LOGD("Not target, skipping");
            return;
        }
        
        LOGD("=== Target detected! Spoofing CPU info ===");
        
        // 挂载伪造的 cpuinfo
        if (mount_fake_files()) {
            LOGD("CPU info spoofed successfully!");
        } else {
            LOGE("Failed to spoof CPU info");
        }
    }

private:
    const char* get_process_name() {
        static char cmdline[256];
        int fd = open("/proc/self/cmdline", O_RDONLY);
        if (fd < 0) return "unknown";
        int len = read(fd, cmdline, sizeof(cmdline) - 1);
        close(fd);
        if (len > 0) {
            cmdline[len] = '\0';
            return cmdline;
        }
        return "unknown";
    }

    bool mount_fake_files() {
        bool success = false;
        
        // 模块安装路径
        const char* module_path = "/data/adb/modules/kirin9000s_spoofer";
        char fake_cpuinfo[256];
        snprintf(fake_cpuinfo, sizeof(fake_cpuinfo), "%s/cpuinfo", module_path);
        
        // 检查伪造文件是否存在
        if (access(fake_cpuinfo, F_OK) != 0) {
            LOGE("Fake cpuinfo not found: %s", fake_cpuinfo);
            return false;
        }
        
        // 挂载 /proc/cpuinfo
        if (mount(fake_cpuinfo, "/proc/cpuinfo", nullptr, MS_BIND, nullptr) == 0) {
            LOGD("Successfully mounted fake cpuinfo");
            success = true;
        } else {
            LOGE("Failed to mount cpuinfo");
        }
        
        // 尝试挂载 midr_el1（可选）
        char fake_midr[256];
        snprintf(fake_midr, sizeof(fake_midr), "%s/midr_el1", module_path);
        const char* midr_target = "/sys/devices/system/cpu/cpu0/regs/identification/midr_el1";
        
        if (access(fake_midr, F_OK) == 0 && access(midr_target, F_OK) == 0) {
            if (mount(fake_midr, midr_target, nullptr, MS_BIND, nullptr) == 0) {
                LOGD("Successfully mounted fake midr_el1");
            }
        }
        
        return success;
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
