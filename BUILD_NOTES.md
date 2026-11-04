# Build / architecture notes

The uploaded BedrockTools source was used as the base.

## HUD movement

BedrockTools HUD modules use these config fields:

- `isHudModule`
- `hudPosX`
- `hudPosY`

`ModuleMenu.cpp` excludes those from ordinary settings. Each enabled button
submits an explicit `HudEditorElement` with its bounds and position keys. The
launcher HUD editor sends position changes through the module config callback,
and the mod saves them to its config file. Each button is a separate module,
so its position is independent. An enabled button without a command still has
an editor element, but is not drawn or clickable during gameplay.

## Touch

Preloader's coordinate-aware `pl::input::registerTouchCallback` is connected
to the module registry. The first matching enabled button consumes the event,
which also prevents overlapping buttons from both firing.
