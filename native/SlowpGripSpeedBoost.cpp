// Slowp 0.99.144 — QuestLoader native mod prototype.
// Target package: com.CruelByte.Slowp
// Build target: Android arm64-v8a. Requires a compatible Dobby library.
// IMPORTANT: this source is not yet validated on a headset. See README.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "SlowpGripBoost"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
constexpr float kBoost = 1.25f;
constexpr int kRightHandNode = 5;  // UnityEngine.XR.XRNode.RightHand from dump.cs
constexpr int kGripButtonUsage = 5; // Slowp.Input.XRCommonUsage.GripButton from dump.cs
constexpr std::uintptr_t kValueOffset = 0x40;
constexpr std::uintptr_t kNodeOffset = 0x30;
constexpr std::uintptr_t kUsageOffset = 0x34;

// Dobby's public hook API. The symbol must be available from the mod's linked
// Dobby library or the process global namespace.
using DobbyHookFn = int (*)(void* address, void* replace_call, void** origin_call);
DobbyHookFn g_dobbyHook = nullptr;
std::atomic<bool> g_rightGripHeld{false};

struct Vec3 { float x, y, z; };
struct Il2CppObject { void* klass; void* monitor; };
struct Il2CppMethodInfo { void* methodPointer; };

using DomainGetFn = void* (*)();
using DomainGetAssembliesFn = const void** (*)(void*, std::size_t*);
using AssemblyGetImageFn = const void* (*)(const void*);
using ClassFromNameFn = void* (*)(const void*, const char*, const char*);
using ClassGetMethodFn = const Il2CppMethodInfo* (*)(void*, const char*, int);

DomainGetFn g_domainGet = nullptr;
DomainGetAssembliesFn g_domainGetAssemblies = nullptr;
AssemblyGetImageFn g_assemblyGetImage = nullptr;
ClassFromNameFn g_classFromName = nullptr;
ClassGetMethodFn g_classGetMethod = nullptr;

using UpdateFn = void (*)(void* self, const void* methodInfo);
UpdateFn g_originalBoolInputUpdate = nullptr;

// Native ABI mirrors Board::Force(Vector3, Vector3, ref Vector3, ref Vector3,
// ref Vector3, ref float, ref float, ref float, ref float, ref float,
// ref float, ref bool, ref bool, out Vector3) as declared in dump.cs.
using BoardForceFn = Vec3 (*)(void* self, Vec3 point, Vec3 normal,
    Vec3* boardUp, Vec3* boardForward, Vec3* boardRight,
    float* carvingCoeff, float* sidewaysCoeff, float* forwardCoeff,
    float* slipnessMax, float* frictionMax, float* boardVelocityMag,
    bool* isUsingCurve, bool* isUsingForwardDamper, Vec3* velocityPoint,
    const void* methodInfo);
BoardForceFn g_originalBoardForce = nullptr;

void HookedBoolInputUpdate(void* self, const void* methodInfo) {
    if (g_originalBoolInputUpdate) g_originalBoolInputUpdate(self, methodInfo);
    if (!self) return;

    // The class is used for many controller inputs. Only sample right-hand GripButton.
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(self);
    const int node = *reinterpret_cast<const int*>(bytes + kNodeOffset);
    const int usage = *reinterpret_cast<const int*>(bytes + kUsageOffset);
    if (node == kRightHandNode && usage == kGripButtonUsage) {
        const bool pressed = *reinterpret_cast<const bool*>(bytes + kValueOffset);
        g_rightGripHeld.store(pressed, std::memory_order_relaxed);
    }
}

Vec3 HookedBoardForce(void* self, Vec3 point, Vec3 normal,
    Vec3* boardUp, Vec3* boardForward, Vec3* boardRight,
    float* carvingCoeff, float* sidewaysCoeff, float* forwardCoeff,
    float* slipnessMax, float* frictionMax, float* boardVelocityMag,
    bool* isUsingCurve, bool* isUsingForwardDamper, Vec3* velocityPoint,
    const void* methodInfo) {
    if (!g_originalBoardForce) return Vec3{0.f, 0.f, 0.f};

    // Scale only the forward force coefficient in a temporary. Never mutate the
    // game's stored coefficient and never change gravity or repeatedly multiply velocity.
    if (forwardCoeff && g_rightGripHeld.load(std::memory_order_relaxed)) {
        float boostedForwardCoeff = *forwardCoeff * kBoost;
        return g_originalBoardForce(self, point, normal, boardUp, boardForward,
            boardRight, carvingCoeff, sidewaysCoeff, &boostedForwardCoeff,
            slipnessMax, frictionMax, boardVelocityMag, isUsingCurve,
            isUsingForwardDamper, velocityPoint, methodInfo);
    }

    return g_originalBoardForce(self, point, normal, boardUp, boardForward,
        boardRight, carvingCoeff, sidewaysCoeff, forwardCoeff,
        slipnessMax, frictionMax, boardVelocityMag, isUsingCurve,
        isUsingForwardDamper, velocityPoint, methodInfo);
}

void* FindMethod(const char* namesp, const char* klassName, const char* methodName, int argc) {
    void* domain = g_domainGet ? g_domainGet() : nullptr;
    if (!domain) return nullptr;
    std::size_t count = 0;
    const void** assemblies = g_domainGetAssemblies(domain, &count);
    if (!assemblies) return nullptr;
    for (std::size_t i = 0; i < count; ++i) {
        const void* image = g_assemblyGetImage(assemblies[i]);
        if (!image) continue;
        void* klass = g_classFromName(image, namesp, klassName);
        if (!klass) continue;
        const auto* method = g_classGetMethod(klass, methodName, argc);
        if (method && method->methodPointer) return method->methodPointer;
    }
    return nullptr;
}

bool InitIl2CppApi() {
    void* il2cpp = dlopen("libil2cpp.so", RTLD_NOW | RTLD_NOLOAD);
    if (!il2cpp) il2cpp = dlopen("libil2cpp.so", RTLD_NOW);
    if (!il2cpp) { LOGE("Could not open libil2cpp.so"); return false; }
    g_domainGet = reinterpret_cast<DomainGetFn>(dlsym(il2cpp, "il2cpp_domain_get"));
    g_domainGetAssemblies = reinterpret_cast<DomainGetAssembliesFn>(dlsym(il2cpp, "il2cpp_domain_get_assemblies"));
    g_assemblyGetImage = reinterpret_cast<AssemblyGetImageFn>(dlsym(il2cpp, "il2cpp_assembly_get_image"));
    g_classFromName = reinterpret_cast<ClassFromNameFn>(dlsym(il2cpp, "il2cpp_class_from_name"));
    g_classGetMethod = reinterpret_cast<ClassGetMethodFn>(dlsym(il2cpp, "il2cpp_class_get_method_from_name"));
    return g_domainGet && g_domainGetAssemblies && g_assemblyGetImage &&
           g_classFromName && g_classGetMethod;
}

bool InstallHooks() {
    void* dobby = dlopen("libdobby.so", RTLD_NOW | RTLD_NOLOAD);
    if (!dobby) dobby = dlopen("libdobby.so", RTLD_NOW);
    g_dobbyHook = reinterpret_cast<DobbyHookFn>(dlsym(dobby ? dobby : RTLD_DEFAULT, "DobbyHook"));
    if (!g_dobbyHook) {
        g_dobbyHook = reinterpret_cast<DobbyHookFn>(dlsym(RTLD_DEFAULT, "DobbyHook"));
    }
    if (!g_dobbyHook) { LOGE("DobbyHook not found; provide compatible Dobby library"); return false; }

    void* boolUpdate = FindMethod("Slowp.Input", "XRCommonBoolInput", "Update", 0);
    void* boardForce = FindMethod("Slowp.Player", "Board", "Force", 14);
    if (!boolUpdate || !boardForce) {
        LOGE("Could not resolve target methods (boolUpdate=%p boardForce=%p)", boolUpdate, boardForce);
        return false;
    }
    if (g_dobbyHook(boolUpdate, reinterpret_cast<void*>(&HookedBoolInputUpdate),
                    reinterpret_cast<void**>(&g_originalBoolInputUpdate)) != 0) {
        LOGE("Hook install failed for XRCommonBoolInput.Update"); return false;
    }
    if (g_dobbyHook(boardForce, reinterpret_cast<void*>(&HookedBoardForce),
                    reinterpret_cast<void**>(&g_originalBoardForce)) != 0) {
        LOGE("Hook install failed for Board.Force"); return false;
    }
    LOGI("Hooks installed. Right grip boost = %.2fx", kBoost);
    return true;
}
} // namespace

// QuestLoader (sc2ad) mod lifecycle exports. Its loader calls init/load if present.
extern "C" __attribute__((visibility("default"))) void init() {
    LOGI("SlowpGripSpeedBoost init");
}

extern "C" __attribute__((visibility("default"))) void load() {
    if (!InitIl2CppApi()) {
        LOGE("IL2CPP exports unavailable; mod not activated");
        return;
    }
    if (!InstallHooks()) LOGE("Mod did not activate; see logcat for details");
}
