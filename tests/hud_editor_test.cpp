#include "config/ConfigManager.hpp"
#include "launcher/ModuleMenu.hpp"
#include "modules/ModuleRegistry.hpp"
#include "modules/floating/FloatingCommandButton.hpp"
#include <bedrocktools/memory/Signatures.hpp>
#include <pl/ModMenu.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace {
std::map<std::string, pl::modmenu::ModuleInfo> registered;
std::map<std::string, std::vector<pl::modmenu::HudEditorElement>> elements;
std::map<std::string, std::vector<pl::modmenu::DrawCommand>> draws;
}

namespace pl::modmenu {
bool registerModule(const ModuleInfo& info) {
    return registered.emplace(info.moduleId, info).second;
}
void submitDrawCommands(std::string_view id, std::span<const DrawCommand> commands) {
    draws[std::string(id)] = {commands.begin(), commands.end()};
}
void submitHudEditorElements(std::string_view id, std::span<const HudEditorElement> submitted) {
    elements[std::string(id)] = {submitted.begin(), submitted.end()};
}
}
namespace bedrocktools::memory {
std::uintptr_t resolve(SignatureId) { return 0; }
}
namespace bedrocktools::core::gamehooks {
void* clientInstance() { return nullptr; }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::filesystem::path path = argv[1];
    std::filesystem::remove(path);
    auto& config = bedrocktools::config::ConfigManager::get();
    config.setConfigPath(path.string());
    registerAllModules();
    registerModulesWithLauncher();
    assert(registered.size() == 10);
    const std::string one = "bedrocktools.Floating Button 1";
    const std::string three = "bedrocktools.Floating Button 3";
    auto& first = registered.at(one);
    auto& third = registered.at(three);
    assert(!first.hideInHudEditor && !third.hideInHudEditor);
    first.onToggle(one, true);
    third.onToggle(three, true);
    ModuleRegistry::get().onFrame();
    assert(elements[one].size() == 1 && draws[one].size() == 3);
    assert(elements[three].size() == 1 && draws[three].empty()); // no command yet
    assert(elements[one][0].elementId != elements[three][0].elementId);
    assert(elements[one][0].positionKeyX == "hudPosX");
    assert(elements[one][0].positionKeyY == "hudPosY");
    assert(elements[one][0].width == 160 && elements[one][0].height == 52);

    // Simulate the launcher issuing separate X and Y callbacks while dragging.
    first.onConfigChanged(one, "hudPosX", "2401.25");
    first.onConfigChanged(one, "hudPosY", "431.5");
    third.onConfigChanged(three, "hudPosX", "150");
    third.onConfigChanged(three, "hudPosY", "300");
    ModuleRegistry::get().onFrame();
    assert(elements[one][0].x == 2401.25f && elements[one][0].y == 431.5f);
    assert(draws[one][1].x == 2401.25f && draws[one][1].y == 431.5f);
    assert(elements[three][0].x == 150 && elements[three][0].y == 300);
    auto* button = ModuleRegistry::get().find(one);
    assert(!button->onTouchEvent(0, 1, 41, 91));
    assert(button->onTouchEvent(0, 1, 2405, 435));
    assert(button->onTouchEvent(3, 1, 2405, 435));

    config.flush();
    nlohmann::json saved;
    std::ifstream(path) >> saved;
    assert(saved["Modules"]["Floating Button 1"]["hudPosX"] == 2401.25f);
    assert(saved["Modules"]["Floating Button 3"]["hudPosY"] == 300.0f);
    FloatingCommandButton reloaded(1);
    reloaded.loadConfig(saved["Modules"]["Floating Button 1"]);
    nlohmann::json copy;
    reloaded.saveConfig(copy);
    assert(copy["hudPosX"] == 2401.25f && copy["hudPosY"] == 431.5f);

    // Clearing a command removes stale graphics but keeps the editor handle.
    first.onConfigChanged(one, "command", "");
    ModuleRegistry::get().onFrame();
    assert(draws[one].empty() && elements[one].size() == 1);
    assert(!button->onTouchEvent(0, 1, 2405, 435));
    first.onToggle(one, false);
    assert(draws[one].empty() && elements[one].empty());
    first.onToggle(one, true);
    ModuleRegistry::get().onFrame();
    assert(elements[one].size() == 1 && elements[one][0].x == 2401.25f);
    first.onConfigChanged(one, "command", "time set day");
    ModuleRegistry::get().onFrame();
    assert(draws[one].size() == 3);
    config.flush();
    std::filesystem::remove(path);
}
