#include "ModuleRegistry.hpp"
#include "floating/FloatingCommandButton.hpp"

ModuleRegistry& ModuleRegistry::get() {
    static ModuleRegistry registry;
    return registry;
}

Module* ModuleRegistry::find(std::string_view id) const {
    const auto it = mById.find(id);
    return it == mById.end() ? nullptr : it->second;
}

const std::vector<Module*>& ModuleRegistry::modules() const {
    return mView;
}

void ModuleRegistry::initialize() {
    if (mInitialized) return;
    for (auto* module : mView) module->onInit();
    mInitialized = true;
}

void ModuleRegistry::onFrame() {
    for (auto* module : mView) {
        if (module->enabled) module->onFrame();
    }
}

bool ModuleRegistry::onMouseEvent(int button, bool isDown) {
    for (auto* module : mView) {
        if (module->enabled && module->onMouseEvent(button, isDown)) return true;
    }
    return false;
}

bool ModuleRegistry::onTouchEvent(int action, int pointerId, float x, float y) {
    // First matching button wins. This prevents two overlapping buttons from
    // both executing a command on the same touch release.
    for (auto* module : mView) {
        if (module->enabled && module->onTouchEvent(action, pointerId, x, y)) return true;
    }
    return false;
}

void ModuleRegistry::setKeybindBlocked(bool blocked) {
    mKeybindBlocked = blocked;
}

bool ModuleRegistry::keybindBlocked() const {
    return mKeybindBlocked;
}

void registerAllModules() {
    auto& registry = ModuleRegistry::get();
    if (!registry.modules().empty()) return;

    for (int i = 1; i <= 10; ++i) {
        registry.emplace<FloatingCommandButton>(i);
    }
}
