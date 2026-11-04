#pragma once

#include "../Module.hpp"
#include <cstdint>
#include <string>

class FloatingCommandButton final : public Module {
public:
    explicit FloatingCommandButton(int slot);
    ~FloatingCommandButton() override = default;

    void onFrame() override;
    bool onTouchEvent(int action, int pointerId, float x, float y) override;
    void loadConfig(const nlohmann::json& j) override;
    void saveConfig(nlohmann::json& j) override;

    int slot() const noexcept { return m_slot; }

private:
    int m_slot = 1;

    std::string m_buttonName;
    std::string m_command;
    float m_opacity = 1.0f;

    // These are intentionally the same HUD properties used by BedrockTools
    // HUD modules, so the launcher HUD editor can move each button itself.
    float hudPosX = 40.0f;
    float hudPosY = 120.0f;
    bool isHudModule = true;

    int m_activePointer = -1;
    bool m_pressed = false;
    bool m_dragLikeMove = false;
    float m_downX = 0.0f;
    float m_downY = 0.0f;

    static constexpr float Width = 160.0f;
    static constexpr float Height = 52.0f;
    static constexpr float CornerRadius = 10.0f;

    bool contains(float x, float y) const noexcept;
    bool sendCommand();
    std::string normalizedCommand() const;
};
