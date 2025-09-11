#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#include "pomodoro.hpp"

inline HANDLE PHANDLE = nullptr;

class HyprmodoroDecoration;

struct SGlobalState {
    std::vector<WP<HyprmodoroDecoration>> decorations;
    UP<Pomodoro>                          pomodoroSession;
};

inline UP<SGlobalState> g_pGlobalState;
