# Floating Command Buttons

Standalone native LeviLauncher/Android mod for Minecraft Bedrock 1.26.60.X.

## Design

The mod provides **10 independent floating button modules**. Each button is
its own HUD module so the launcher's existing HUD editor can move it, just like
other BedrockTools HUD modules. No custom X/Y/drag settings are created by this mod.

Each button exposes only:

- Enable / disable
- Button Name
- Opacity
- Command

Enable each button in the Mod Menu, then open the launcher's HUD Editor and
drag it to the desired position. Positions are saved independently. Buttons
without a command can still be positioned in the editor but are not displayed
or clickable during gameplay. The installed launcher and preloader must support
`submitHudEditorElements`.

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

## Compatibility and verification

Signatures and SDK offsets are based on BedrockTools Plus (commit
`148407d9a1051daed4c2ebaa4bf92a4f872d6e19`), which targets Minecraft
1.26.60.X on Android arm64. Other versions are not supported by these
version-specific hooks. The upstream table does not provide patterns for
`RenderPotionEffects` or `ItemInHandRenderOffhandItem`; those IDs remain
unresolved and are not used by the floating buttons. See `LICENSE.upstream`
for the upstream MIT notice.

The game binary is not included in this repository. Building the mod alone
does not verify that signatures resolve on a particular APK: test the packaged
mod with the intended 26.60 build in LeviLauncher before relying on it.

## Host tests

To run the HUD editor integration test, check out `preloader-android` and
`nlohmann/json` v3.11.3, then run:

```sh
PRELOADER_SOURCE=/path/to/preloader-android JSON_SOURCE=/path/to/json \
  tests/run_hud_editor_test.sh
```

This simulates editor callbacks and config persistence; an Android device is
still needed to verify dragging in the actual launcher.
