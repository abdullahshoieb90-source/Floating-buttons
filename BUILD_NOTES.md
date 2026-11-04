# Build / architecture notes

The uploaded BedrockTools source was used as the base.

## HUD movement

BedrockTools HUD modules use these config fields:

- `isHudModule`
- `hudPosX`
- `hudPosY`

`ModuleMenu.cpp` excludes those from ordinary settings, while the launcher HUD
editor can use them for positioning. Each floating button is therefore a
separate `FloatingCommandButton` module so every button has an independent
HUD-editor position.

## Touch

Preloader's coordinate-aware `pl::input::registerTouchCallback` is connected
to the module registry. The first matching enabled button consumes the event,
which also prevents overlapping buttons from both firing.
