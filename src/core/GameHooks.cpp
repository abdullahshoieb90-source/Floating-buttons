#include "GameHooks.hpp"
#include <bedrocktools/Version.hpp>

#include "core/memory/Hooks.hpp"
#include <bedrocktools/events/EventBus.hpp>
#include <bedrocktools/memory/Signatures.hpp>
#include <EGL/egl.h>
#include <array>
#include <atomic>
#include <mutex>
#include <string>

namespace bedrocktools::core::gamehooks {
namespace {
using namespace bedrocktools::events;
using bedrocktools::memory::SignatureId;

using VersionStringFn = std::string(*)(void*);
using NormalTickFn = void(*)(void*);
using AttackFn = bool(*)(void*, void*, bool, const void*);
using ClientInstanceUpdateFn = void*(*)(void*, bool);
using ScreenFn = void*(*)(void*, void*, void*, void*, void*, void*, void*, void*);
using EglSwapBuffersFn = EGLBoolean(*)(EGLDisplay, EGLSurface);

VersionStringFn versionOriginal = nullptr;
NormalTickFn tickOriginal = nullptr;
AttackFn gameModeAttackOriginal = nullptr;
ClientInstanceUpdateFn clientUpdateOriginal = nullptr;
ScreenFn containerOpenOriginal = nullptr;
ScreenFn containerCloseOriginal = nullptr;
ScreenFn chatOpenOriginal = nullptr;
ScreenFn chatCloseOriginal = nullptr;
EglSwapBuffersFn swapBuffersOriginal = nullptr;
std::atomic<void*> currentClientInstance = nullptr;
std::array<bedrocktools::hooks::Handle, 10> handles{};
std::size_t handleCount = 0;
std::mutex installMutex;
bool installed = false;
std::string gameVersion;

template <class Function>
bool hookSignature(SignatureId id, void* detour, Function** original) {
    const auto address = bedrocktools::memory::resolve(id);
    if (!address) return false;
    bedrocktools::hooks::Handle handle = bedrocktools::hooks::install(reinterpret_cast<void*>(address), detour, reinterpret_cast<void**>(original));
    if (!handle) return false;
    if (handleCount < handles.size()) handles[handleCount++] = handle;
    return true;
}

std::string versionDetour(void* self) {
    std::string version = versionOriginal ? versionOriginal(self) : std::string{};
    if (gameVersion.empty()) gameVersion = version;
    return std::string("\xC2\xA7" "b") + std::string(bedrocktools::Name) + " v" + std::string(bedrocktools::Version) + " " + "\xC2\xA7" "fby " + "\xC2\xA7" "e" + std::string(bedrocktools::Author) + " " + "\xC2\xA7" "f- " + "\xC2\xA7" "r" + version;
}

void tickDetour(void* actor) {
    if (tickOriginal) tickOriginal(actor);
    LocalPlayerTickEvent event{reinterpret_cast<bedrocktools::sdk::Player*>(actor)};
    bus().publish(event);
}

bool gameModeAttackDetour(void* gameMode, void* target, bool playPredictiveSound, const void* hitPosition) {
    AttackKind kind = AttackKind::GameMode;
    const auto survivalAttack = bedrocktools::memory::resolve(SignatureId::SurvivalModeAttack);
    if (gameMode && survivalAttack) {
        auto** vtable = *reinterpret_cast<void***>(gameMode);
        if (vtable && reinterpret_cast<std::uintptr_t>(vtable[16]) == survivalAttack) {
            kind = AttackKind::SurvivalMode;
        }
    }
    AttackEvent event{kind, gameMode, reinterpret_cast<bedrocktools::sdk::Actor*>(target),
                      reinterpret_cast<void*>(static_cast<std::uintptr_t>(playPredictiveSound)),
                      const_cast<void*>(hitPosition)};
    bus().publish(event);
    if (event.cancelled()) return false;
    return gameModeAttackOriginal ? gameModeAttackOriginal(gameMode, target, playPredictiveSound, hitPosition) : false;
}

void* clientUpdateDetour(void* clientInstance, bool value) {
    if (clientInstance) currentClientInstance.store(clientInstance, std::memory_order_release);
    void* result = clientUpdateOriginal ? clientUpdateOriginal(clientInstance, value) : nullptr;
    ClientInstanceUpdateEvent event{reinterpret_cast<bedrocktools::sdk::ClientInstance*>(clientInstance)};
    bus().publish(event);
    return result;
}

void* containerOpenDetour(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7) {
    ScreenStateEvent event{ScreenKind::Container, ScreenPhase::Opened, a0};
    bus().publish(event);
    return containerOpenOriginal ? containerOpenOriginal(a0, a1, a2, a3, a4, a5, a6, a7) : nullptr;
}

void* containerCloseDetour(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7) {
    ScreenStateEvent event{ScreenKind::Container, ScreenPhase::Closed, a0};
    bus().publish(event);
    return containerCloseOriginal ? containerCloseOriginal(a0, a1, a2, a3, a4, a5, a6, a7) : nullptr;
}

void* chatOpenDetour(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7) {
    ScreenStateEvent event{ScreenKind::Chat, ScreenPhase::Opened, a0};
    bus().publish(event);
    return chatOpenOriginal ? chatOpenOriginal(a0, a1, a2, a3, a4, a5, a6, a7) : nullptr;
}

void* chatCloseDetour(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7) {
    ScreenStateEvent event{ScreenKind::Chat, ScreenPhase::Closed, a0};
    bus().publish(event);
    return chatCloseOriginal ? chatCloseOriginal(a0, a1, a2, a3, a4, a5, a6, a7) : nullptr;
}

EGLBoolean swapBuffersDetour(EGLDisplay display, EGLSurface surface) {
    if (eglGetCurrentContext() != EGL_NO_CONTEXT) {
        FrameEvent event;
        bus().publish(event);
    }
    return swapBuffersOriginal ? swapBuffersOriginal(display, surface) : EGL_FALSE;
}

bool hookEgl() {
    auto egl = bedrocktools::hooks::openLibrary("libEGL.so");
    if (!egl) return false;
    const auto address = bedrocktools::hooks::symbol(egl, "eglSwapBuffers");
    if (!address) {
        bedrocktools::hooks::closeLibrary(egl);
        return false;
    }
    auto handle = bedrocktools::hooks::install(reinterpret_cast<void*>(address), reinterpret_cast<void*>(swapBuffersDetour), reinterpret_cast<void**>(&swapBuffersOriginal));
    bedrocktools::hooks::closeLibrary(egl);
    if (!handle) return false;
    if (handleCount < handles.size()) handles[handleCount++] = handle;
    return true;
}

// Called with installMutex held. Failed installs must not leave hooks behind:
// Runtime may retry after a later library load.
void removeHooks() {
    for (std::size_t i = 0; i < handleCount; ++i) {
        if (handles[i]) bedrocktools::hooks::remove(handles[i]);
        handles[i] = nullptr;
    }
    handleCount = 0;
    versionOriginal = nullptr;
    tickOriginal = nullptr;
    gameModeAttackOriginal = nullptr;
    clientUpdateOriginal = nullptr;
    containerOpenOriginal = nullptr;
    containerCloseOriginal = nullptr;
    chatOpenOriginal = nullptr;
    chatCloseOriginal = nullptr;
    swapBuffersOriginal = nullptr;
    installed = false;
    currentClientInstance.store(nullptr, std::memory_order_release);
}
}

bool install() {
    std::lock_guard lock(installMutex);
    if (installed) return true;
    hookSignature(SignatureId::VersionString, reinterpret_cast<void*>(versionDetour), &versionOriginal);
    hookSignature(SignatureId::NormalTick, reinterpret_cast<void*>(tickDetour), &tickOriginal);
    hookSignature(SignatureId::GameModeAttackInternal, reinterpret_cast<void*>(gameModeAttackDetour), &gameModeAttackOriginal);
    hookSignature(SignatureId::ClientInstanceUpdate, reinterpret_cast<void*>(clientUpdateDetour), &clientUpdateOriginal);
    hookSignature(SignatureId::ContainerScreenControllerOpen, reinterpret_cast<void*>(containerOpenDetour), &containerOpenOriginal);
    hookSignature(SignatureId::ContainerScreenControllerDtor, reinterpret_cast<void*>(containerCloseDetour), &containerCloseOriginal);
    hookSignature(SignatureId::ChatScreenOpen, reinterpret_cast<void*>(chatOpenDetour), &chatOpenOriginal);
    hookSignature(SignatureId::ChatScreenDtor, reinterpret_cast<void*>(chatCloseDetour), &chatCloseOriginal);
    hookEgl();
    installed = tickOriginal != nullptr && clientUpdateOriginal != nullptr;
    if (!installed) removeHooks();
    return installed;
}

void uninstall() {
    std::lock_guard lock(installMutex);
    removeHooks();
}

void* clientInstance() {
    return currentClientInstance.load(std::memory_order_acquire);
}

}
