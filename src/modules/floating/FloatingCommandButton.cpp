#include "FloatingCommandButton.hpp"
#include "modules/ModuleRegistry.hpp"
#include "config/ConfigManager.hpp"
#include "core/GameHooks.hpp"
#include <bedrocktools/memory/Signatures.hpp>
#include <bedrocktools/sdk/Offsets.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace {

const char* moduleNameForSlot(int slot) {
    switch (slot) {
        case 1: return "Floating Button 1";
        case 2: return "Floating Button 2";
        case 3: return "Floating Button 3";
        case 4: return "Floating Button 4";
        case 5: return "Floating Button 5";
        case 6: return "Floating Button 6";
        case 7: return "Floating Button 7";
        case 8: return "Floating Button 8";
        case 9: return "Floating Button 9";
        default: return "Floating Button 10";
    }
}

using SendToServerFn = void* (*)(void*, void*);
using GetPacketSenderFn = void* (*)(void*);
using CreatePacketFn = std::shared_ptr<void> (*)(int);

SendToServerFn g_sendToServer = nullptr;
GetPacketSenderFn g_getPacketSender = nullptr;
CreatePacketFn g_createPacket = nullptr;

void resolveCommandPath() {
    if (!g_sendToServer) {
        g_sendToServer = reinterpret_cast<SendToServerFn>(
            bedrocktools::memory::resolve(
                bedrocktools::memory::SignatureId::LoopbackPacketSenderSendToServer));
    }
    if (!g_getPacketSender) {
        g_getPacketSender = reinterpret_cast<GetPacketSenderFn>(
            bedrocktools::memory::resolve(
                bedrocktools::memory::SignatureId::ClientInstanceGetPacketSender));
    }
    if (!g_createPacket) {
        g_createPacket = reinterpret_cast<CreatePacketFn>(
            bedrocktools::memory::resolve(
                bedrocktools::memory::SignatureId::MinecraftPacketsCreatePacket));
    }
}

uint32_t applyOpacity(uint32_t color, float opacity) {
    const auto alpha = static_cast<uint32_t>(
        static_cast<float>((color >> 24) & 0xFFu) *
        std::clamp(opacity, 0.0f, 1.0f));
    return (alpha << 24) | (color & 0x00FFFFFFu);
}

} // namespace

FloatingCommandButton::FloatingCommandButton(int slot)
    : Module(
          moduleNameForSlot(slot),
          "A configurable floating Minecraft command button."),
      m_slot(slot),
      m_buttonName(std::string("Button ") + std::to_string(slot)) {
    isHudModule = true;
    hideInHudEditor = false;
    showInMenu = true;

    // Give each button a useful initial position without creating a separate
    // position editor. The launcher HUD editor owns these two coordinates.
    hudPosX = 40.0f;
    hudPosY = 90.0f + static_cast<float>(slot - 1) * 58.0f;

    switch (slot) {
        case 1: m_command = "/time set day"; break;
        case 2: m_command = "/time set night"; break;
        default: m_command = ""; break;
    }
}

std::string FloatingCommandButton::normalizedCommand() const {
    const auto first = m_command.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};

    const auto last = m_command.find_last_not_of(" \t\r\n");
    std::string result = m_command.substr(first, last - first + 1);
    if (!result.empty() && result.front() != '/') {
        result.insert(result.begin(), '/');
    }
    return result;
}

bool FloatingCommandButton::sendCommand() {
    const std::string command = normalizedCommand();
    if (command.empty() || command.size() > 4096) return false;

    resolveCommandPath();

    void* client = bedrocktools::core::gamehooks::clientInstance();
    if (!client || !g_sendToServer || !g_getPacketSender || !g_createPacket) {
        return false;
    }

    std::shared_ptr<void> packet = g_createPacket(77);
    if (!packet) return false;

    const uintptr_t payload = reinterpret_cast<uintptr_t>(packet.get()) +
                              bedrocktools::sdk::offsets::Packet::Size;

    *reinterpret_cast<std::string*>(
        payload + bedrocktools::sdk::offsets::CommandRequestPacketPayload::mCommand) = command;

    *reinterpret_cast<uint8_t*>(
        payload +
        bedrocktools::sdk::offsets::CommandRequestPacketPayload::mOrigin +
        bedrocktools::sdk::offsets::CommandOriginData::mType) = 0;

    *reinterpret_cast<bool*>(
        payload + bedrocktools::sdk::offsets::CommandRequestPacketPayload::mInternalSource) = true;

    void* sender = g_getPacketSender(client);
    if (!sender) return false;

    g_sendToServer(sender, packet.get());
    return true;
}

bool FloatingCommandButton::contains(float x, float y) const noexcept {
    return x >= hudPosX && x <= hudPosX + Width &&
           y >= hudPosY && y <= hudPosY + Height;
}

void FloatingCommandButton::onFrame() {
    if (!enabled || !isHudModule || m_buttonName.empty() || normalizedCommand().empty()) {
        return;
    }

    std::vector<PLModMenu_DrawCommand> commands;
    commands.reserve(3);

    const uint32_t background = m_pressed
        ? applyOpacity(0xF04A90E2u, m_opacity)
        : applyOpacity(0xD92B2B2Bu, m_opacity);

    PLModMenu_DrawCommand shadow{};
    shadow.type = PL_DRAW_RECT_FILLED;
    shadow.x = hudPosX + 3.0f;
    shadow.y = hudPosY + 4.0f;
    shadow.w = Width;
    shadow.h = Height;
    shadow.color = applyOpacity(0x88000000u, m_opacity);
    shadow.x3 = CornerRadius;
    commands.push_back(shadow);

    PLModMenu_DrawCommand body{};
    body.type = PL_DRAW_RECT_FILLED;
    body.x = hudPosX;
    body.y = hudPosY;
    body.w = Width;
    body.h = Height;
    body.color = background;
    body.x3 = CornerRadius;
    commands.push_back(body);

    PLModMenu_DrawCommand label{};
    label.type = PL_DRAW_TEXT;
    label.x = hudPosX + 8.0f;
    label.y = hudPosY + 4.0f;
    label.w = Width - 16.0f;
    label.h = Height - 8.0f;
    label.color = applyOpacity(0xFFFFFFFFu, m_opacity);
    label.size = 18.0f;
    label.text = m_buttonName;
    commands.push_back(label);

    submitDrawCommands(moduleId, commands);
}

bool FloatingCommandButton::onTouchEvent(int action, int pointerId, float x, float y) {
    if (!enabled || !isHudModule || normalizedCommand().empty()) return false;

    // Android MotionEvent values: DOWN=0, UP=1, MOVE=2, CANCEL=3.
    constexpr int ACTION_DOWN = 0;
    constexpr int ACTION_UP = 1;
    constexpr int ACTION_CANCEL = 3;

    if (action == ACTION_DOWN) {
        if (m_activePointer != -1 || !contains(x, y)) return false;
        m_activePointer = pointerId;
        m_pressed = true;
        m_dragLikeMove = false;
        m_downX = x;
        m_downY = y;
        return true;
    }

    if (pointerId != m_activePointer) return false;

    if (action == ACTION_UP) {
        const bool activate = m_pressed && !m_dragLikeMove && contains(x, y);
        m_pressed = false;
        m_activePointer = -1;
        m_dragLikeMove = false;
        if (activate) sendCommand();
        return true;
    }

    if (action == ACTION_CANCEL) {
        m_pressed = false;
        m_activePointer = -1;
        m_dragLikeMove = false;
        return true;
    }

    // The launcher HUD editor may use MOVE events while repositioning a HUD
    // item. We deliberately do not change hudPosX/Y here; the launcher owns it.
    if (std::fabs(x - m_downX) > 8.0f || std::fabs(y - m_downY) > 8.0f) {
        m_dragLikeMove = true;
    }
    return true;
}

void FloatingCommandButton::loadConfig(const nlohmann::json& j) {
    Module::loadConfig(j);

    if (j.contains("buttonName")) m_buttonName = j["buttonName"].get<std::string>();
    if (j.contains("command")) m_command = j["command"].get<std::string>();
    if (j.contains("opacity")) {
        m_opacity = std::clamp(j["opacity"].get<float>(), 0.05f, 1.0f);
    }
    if (j.contains("hudPosX")) {
        hudPosX = std::clamp(j["hudPosX"].get<float>(), 0.0f, 2000.0f);
    }
    if (j.contains("hudPosY")) {
        hudPosY = std::clamp(j["hudPosY"].get<float>(), 0.0f, 2000.0f);
    }
    if (j.contains("isHudModule")) isHudModule = j["isHudModule"].get<bool>();
}

void FloatingCommandButton::saveConfig(nlohmann::json& j) {
    Module::saveConfig(j);

    j["buttonName"] = m_buttonName;
    j["command"] = m_command;
    j["opacity"] = m_opacity;
    j["hudPosX"] = hudPosX;
    j["hudPosY"] = hudPosY;
    j["isHudModule"] = isHudModule;
}
