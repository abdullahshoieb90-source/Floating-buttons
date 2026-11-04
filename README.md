# Floating Command Buttons

Standalone native LeviLauncher/Android mod for Minecraft Bedrock 1.26.40.5.

## Design

The mod provides **10 independent floating button modules**. Each button is
its own HUD module so the launcher's existing HUD editor can move it, just like
other BedrockTools HUD modules. No custom X/Y/drag settings are created by this mod.

Each button exposes only:

- Enable / disable
- Button Name
- Opacity
- Command

The existing launcher HUD editor owns the button's position.

## Commands

Commands are sent through the same MCBE command packet path already present in
the supplied BedrockTools `AutoReQ` module:

- `MinecraftPacketsCreatePacket`
- `ClientInstanceGetPacketSender`
- `LoopbackPacketSenderSendToServer`
- `CommandRequestPacket` id 77

A leading `/` is added automatically when missing.

## Build

GitHub Actions uses the project's existing xmake/Android NDK pipeline.

Artifact:

`build/android/arm64-v8a/release/FloatingCommandButtons.levipack`
