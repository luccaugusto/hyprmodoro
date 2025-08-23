#pragma once

#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprutils/math/Box.hpp>
#include "pomodoro.hpp"

inline HANDLE PHANDLE = nullptr;

class HyprmodoroDecoration;

struct SGlobalState {
    std::vector<WP<HyprmodoroDecoration>> decorations;
    UP<Pomodoro>                          pomodoroSession;
};

inline UP<SGlobalState> g_pGlobalState;
