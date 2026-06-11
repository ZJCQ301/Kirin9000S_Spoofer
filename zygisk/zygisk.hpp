#pragma once

#include <jni.h>
#include <cstdio>
#include <string>
#include <dlfcn.h>
#include <android/log.h>

namespace zygisk {

    struct Api;
    struct AppSpecializeArgs;
    struct ServerSpecializeArgs;

    class ModuleBase {
    public:
        virtual void onLoad(Api *api, JNIEnv *env) = 0;
        virtual void preAppSpecialize(AppSpecializeArgs *args) {}
        virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
        virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
        virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
        virtual ~ModuleBase() = default;
    };

    struct Api {
        virtual const char* getProcessName() = 0;
        virtual void pltHookRegister(const char *regex, const char *symbol, void *newFunc, void **oldFunc) = 0;
        virtual void pltHookCommit() = 0;
    };

    struct AppSpecializeArgs {
        void* padding[10];
    };

    struct ServerSpecializeArgs {
        void* padding[6];
    };

}

#define REGISTER_ZYGISK_MODULE(className) \
    extern "C" __attribute__((visibility("default"))) zygisk::ModuleBase *zygisk_module_create() { \
        return new className(); \
}
