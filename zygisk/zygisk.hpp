#pragma once

#include <jni.h>
#include <cstdio>
#include <string>
#include <dlfcn.h>

namespace zygisk {

    class ModuleBase {
    public:
        virtual void onLoad(void* api, JNIEnv* env) = 0;
        virtual void preAppSpecialize(void* args) {}
        virtual void postAppSpecialize(const void* args) {}
        virtual void preServerSpecialize(void* args) {}
        virtual void postServerSpecialize(const void* args) {}
        virtual ~ModuleBase() = default;
    };

}

#define REGISTER_ZYGISK_MODULE(className) \
    extern "C" __attribute__((visibility("default"))) zygisk::ModuleBase* zygisk_module_create() { \
        return new className(); \
}
