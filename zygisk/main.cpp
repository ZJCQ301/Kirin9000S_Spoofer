#include <zygisk.hpp>
#include <android/log.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "KirinSpoof", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "KirinSpoof", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

class KirinSpoofModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        LOGD("Module loaded, registering companion");
        // 注册 companion 入口点（关键！）
        api->registerCompanion(reinterpret_cast<void*>(companion_entry));
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        // 不做任何挂载，只用于可能需要的 hook
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        // 也不做挂载，因为 companion 已经做了
    }

private:
    Api *api = nullptr;

    // 静态 companion 入口函数（独立进程执行）
    static void companion_entry(void *arg) {
        LOGD("Companion started");
        int fd = *reinterpret_cast<int*>(arg);
        LOGD("Companion fd = %d", fd);

        // 读取指令（和隔壁模块一样）
        char cmd;
        if (read(fd, &cmd, 1) <= 0) {
            LOGE("Failed to read command");
            return;
        }

        if (cmd == 1) {
            LOGD("Enter mount flow");
            // 创建状态目录
            mkdir("/data/adb/modules/kirin9000s_spoofer/running_state", 0755);
            
            // 读取实例计数
            int instance_count = 0;
            FILE *fp = fopen("/data/adb/modules/kirin9000s_spoofer/running_state/instance_count", "r");
            if (fp) {
                fscanf(fp, "%d", &instance_count);
                fclose(fp);
            }
            LOGD("Current instance count = %d", instance_count);

            if (instance_count == 0) {
                // 执行挂载
                const char *fake_file = "/data/adb/modules/kirin9000s_spoofer/cpuinfo";
                if (access(fake_file, F_OK) == 0) {
                    if (mount(fake_file, "/proc/cpuinfo", nullptr, MS_BIND, nullptr) == 0) {
                        LOGD("Mount success: %s -> /proc/cpuinfo", fake_file);
                        // 创建 mounted 标记文件
                        int mfd = open("/data/adb/modules/kirin9000s_spoofer/running_state/mounted", O_CREAT | O_WRONLY, 0644);
                        if (mfd >= 0) close(mfd);
                    } else {
                        LOGE("Mount failed: %s", strerror(errno));
                    }
                } else {
                    LOGE("Fake file not found: %s", fake_file);
                }
            } else {
                LOGD("Fake cpuinfo already mounted, skip");
            }

            // 增加实例计数
            instance_count++;
            fp = fopen("/data/adb/modules/kirin9000s_spoofer/running_state/instance_count", "w");
            if (fp) {
                fprintf(fp, "%d", instance_count);
                fclose(fp);
            }
            LOGD("New instance count = %d", instance_count);

            // 等待卸载指令（保持进程运行）
            while (read(fd, &cmd, 1) > 0) {
                // 空循环，等待父进程关闭 fd
            }
            LOGD("Exit mount flow");

            // 卸载流程
            fp = fopen("/data/adb/modules/kirin9000s_spoofer/running_state/instance_count", "r");
            int new_count = 0;
            if (fp) {
                fscanf(fp, "%d", &new_count);
                fclose(fp);
            }
            LOGD("Current instance count before unload = %d", new_count);
            
            if (new_count <= 1) {
                LOGD("Unmounting /proc/cpuinfo");
                umount2("/proc/cpuinfo", MNT_DETACH);
                unlink("/data/adb/modules/kirin9000s_spoofer/running_state/mounted");
            }
            new_count--;
            fp = fopen("/data/adb/modules/kirin9000s_spoofer/running_state/instance_count", "w");
            if (fp) {
                fprintf(fp, "%d", new_count);
                fclose(fp);
            }
            LOGD("Final instance count = %d", new_count);
        }

        LOGD("Companion exiting");
    }
};

REGISTER_ZYGISK_MODULE(KirinSpoofModule)
