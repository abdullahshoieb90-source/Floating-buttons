#!/usr/bin/env bash
set -euo pipefail

# Paths to checkouts of preloader-android and nlohmann/json v3.11.3.
: "${PRELOADER_SOURCE:?Set PRELOADER_SOURCE to a preloader-android checkout}"
: "${JSON_SOURCE:?Set JSON_SOURCE to a nlohmann/json checkout}"
cd "$(dirname "$0")/.."
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

"${CXX:-g++}" -std=c++20 -Wall -Wextra -Wno-unused-parameter \
    -I"$PRELOADER_SOURCE/include" -I"$JSON_SOURCE/single_include" -Iinclude -Isrc \
    tests/hud_editor_test.cpp src/launcher/ModuleMenu.cpp \
    src/modules/ModuleRegistry.cpp src/modules/floating/FloatingCommandButton.cpp \
    src/config/ConfigManager.cpp -pthread -o "$tmp_dir/hud-editor-test"
"$tmp_dir/hud-editor-test" "$tmp_dir/config.json"
