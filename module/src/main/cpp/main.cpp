#include <jni.h>
#include <string.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include "zygisk.hpp"

// =============================================================================
// HOOKING OPENGL ES (Adreno 830)
// =============================================================================
typedef const GLubyte* (*PFNGLGETSTRINGPROC)(GLenum name);
PFNGLGETSTRINGPROC orig_glGetString = nullptr;

const GLubyte* hooked_glGetString(GLenum name) {
    if (name == GL_RENDERER) return (const GLubyte*)"Adreno (TM) 830";
    if (name == GL_VENDOR) return (const GLubyte*)"Qualcomm";
    if (orig_glGetString) return orig_glGetString(name);
    return (const GLubyte*)"";
}

// =============================================================================
// HOOKING VULKAN API (Adreno 830)
// =============================================================================
typedef void (*PFN_vkGetPhysicalDeviceProperties)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties);
PFN_vkGetPhysicalDeviceProperties orig_vkGetPhysicalDeviceProperties = nullptr;

void hooked_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) {
    if (orig_vkGetPhysicalDeviceProperties) orig_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
    pProperties->vendorID = 0x5143;
    pProperties->deviceID = 0x41383330;
    strcpy(pProperties->deviceName, "Adreno (TM) 830");
    pProperties->driverVersion = VK_MAKE_VERSION(512, 800, 0);
}

// =============================================================================
// FUNGSI UTAMA UNTUK MENGAKTIFKAN JALUR PENIPUAN DI MEMORI GAME
// =============================================================================
void aktifkan_spoofing_driver() {
    void* gles_handle = dlopen("libGLESv2.so", RTLD_LAZY);
    if (gles_handle) orig_glGetString = (PFNGLGETSTRINGPROC)dlsym(gles_handle, "glGetString");
    void* vulkan_handle = dlopen("libvulkan.so", RTLD_LAZY);
    if (vulkan_handle) orig_vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)dlsym(vulkan_handle, "vkGetPhysicalDeviceProperties");
}

// =============================================================================
// ENGINES ZYGISK: PENYARINGAN APLIKASI
// =============================================================================
class GPUSpooferModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override { 
        this->api_peta = api; 
        this->env_peta = env; 
    }
    
    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        const char *process_name = env_peta->GetStringUTFChars(args->nice_name, nullptr);
        if (!process_name) return;
        
        // Kode daftar hitam yang sudah diperbaiki total dari typo
        if (strstr(process_name, "com.android.systemui") || 
            strstr(process_name, "ksu.manager") || 
            strstr(process_name, "com.google.android") ||
            strstr(process_name, "android.process")) {
            
            api_peta->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            env_peta->ReleaseStringUTFChars(args->nice_name, process_name);
            return;
        }
        
        aktifkan_spoofing_driver();
        env_peta->ReleaseStringUTFChars(args->nice_name, process_name);
    }
private:
    zygisk::Api *api_peta;
    JNIEnv *env_peta;
};

REGISTER_ZYGISK_MODULE(GPUSpooferModule)
