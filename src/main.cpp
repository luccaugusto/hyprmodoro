#define WLR_USE_UNSTABLE

#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/event/EventBus.hpp>

#include <string>
#include <hyprland/src/config/values/ConfigValues.hpp>

#include "hyprmodoroDecoration.hpp"
#include "globals.hpp"
#include "pomodoro.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

// Dispatchers
SDispatchResult startTimer(std::string) {
    if (g_pGlobalState->pomodoroSession && g_pGlobalState->pomodoroSession->getState() == STOPPED) {
        g_pGlobalState->pomodoroSession->start();
        return SDispatchResult{.success = true};
    }

    return SDispatchResult{.error = "hyprmodoro:start: failed to start timer"};
}

SDispatchResult stopTimer(std::string) {
    if (g_pGlobalState->pomodoroSession) {
        g_pGlobalState->pomodoroSession->stop();
        return SDispatchResult{.success = true};
    }
    return SDispatchResult{.error = "hyprmodoro:stop: failed to stop timer"};
}

SDispatchResult togglePauseTimer(std::string) {
    if (g_pGlobalState->pomodoroSession) {
        g_pGlobalState->pomodoroSession->isPaused() ? g_pGlobalState->pomodoroSession->resume() : g_pGlobalState->pomodoroSession->pause();
        return SDispatchResult{.success = true};
    }

    return SDispatchResult{.error = "hyprmodoro:pause: failed to pause timer"};
}

SDispatchResult setTimer(std::string data) {
    if (g_pGlobalState->pomodoroSession) {
        g_pGlobalState->pomodoroSession->setSessionLength(std::stoi(data));
        return SDispatchResult{.success = true};
    }
    return SDispatchResult{.error = "hyprmodoro:set: failed to set timer"};
}

SDispatchResult setRest(std::string data) {
    if (g_pGlobalState->pomodoroSession) {
        g_pGlobalState->pomodoroSession->setRestLength(std::stoi(data));
        return SDispatchResult{.success = true};
    }
    return SDispatchResult{.error = "hyprmodoro:setRest: failed to set rest duration"};
}

SDispatchResult skip(std::string) {
    if (g_pGlobalState->pomodoroSession) {
        g_pGlobalState->pomodoroSession->skip();
        return SDispatchResult{.success = true};
    }
    return SDispatchResult{.error = "hyprmodoro:skip: failed to skip timer"};
}

// hyprctl commands
std::string getTime(eHyprCtlOutputFormat, std::string) {
    if (!g_pGlobalState->pomodoroSession)
        return std::string("00:00");
    return g_pGlobalState->pomodoroSession->getFormattedTime();
}

std::string getState(eHyprCtlOutputFormat, std::string) {
    if (!g_pGlobalState->pomodoroSession)
        return std::string("STOPPED");
    const std::string states[] = {"STOPPED", "WORKING", "RESTING", "FINISHED", "WAITING_FOR_REST", "WAITING_FOR_WORK"};
    return states[g_pGlobalState->pomodoroSession->getState()];
}

std::string getProgress(eHyprCtlOutputFormat, std::string) {
    if (!g_pGlobalState->pomodoroSession)
        return std::string("0.0");
    return std::to_string(g_pGlobalState->pomodoroSession->getProgress());
}

// handlers
void onWindowOpen(void* self, std::any data) {
    const auto         PWINDOW = std::any_cast<PHLWINDOW>(data);

    static auto const IS_HYPRMODORO_ENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:enabled"};
    if (!(*IS_HYPRMODORO_ENABLED) || PWINDOW == nullptr)
        return;

    if (std::ranges::any_of(PWINDOW->m_windowDecorations, [](const auto& d) { return d->getDisplayName() == "hyprmodoro"; }))
        return;

    auto decoration = makeUnique<HyprmodoroDecoration>(PWINDOW);
    g_pGlobalState->decorations.emplace_back(decoration);
    decoration->m_self = decoration;
    HyprlandAPI::addWindowDecoration(PHANDLE, PWINDOW, std::move(decoration));
}

void onWindowClose(void* self, std::any data) {
    if (g_pGlobalState) {
        g_pGlobalState->decorations.erase(std::remove_if(g_pGlobalState->decorations.begin(), g_pGlobalState->decorations.end(), [](const auto& wp) { return wp.expired(); }),
                                          g_pGlobalState->decorations.end());
    }
}

// Safety check: if title is disabled, auto-transition must be enabled
// Otherwise user has no way to manually transition (no timer to click)
bool getEffectiveAutoTransition() {
    static auto const PAUTOTRANSITION = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:auto_transition"};
    static auto const PTITLEENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:enabled"};

    bool autoTransition = *PAUTOTRANSITION;
    if (!*PTITLEENABLED) {
        autoTransition = true;
    }

    return autoTransition;
}

void onConfigReload() {
    static auto const SESSIONLENGTH = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:work_duration"};
    static auto const RESTLENGTH = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:rest_duration"};

    if (g_pGlobalState->pomodoroSession) {
        g_pGlobalState->pomodoroSession->setSessionLength(*SESSIONLENGTH);
        g_pGlobalState->pomodoroSession->setRestLength(*RESTLENGTH);
        g_pGlobalState->pomodoroSession->setAutoTransition(getEffectiveAutoTransition());
    }
}

// init plugin

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprmodoro] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprmodoro] Version mismatch");
    }

    g_pGlobalState = makeUnique<SGlobalState>();

    using namespace Config::Values;

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:enabled", "Enable hyprmodoro", 1));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:work_duration", "Work session duration in minutes", 25));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:rest_duration", "Rest duration in minutes", 5));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:auto_transition", "Auto-transition between work and rest", 1));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:border:enabled", "Enable progress border", 1));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:border:floating_window", "Show border on floating windows", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:border:all_windows", "Show border on all windows", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Color>("plugin:hyprmodoro:border:color", "Border color", 0x88333333));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Color>("plugin:hyprmodoro:text:color", "Text color", 0xffffffff));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:text:font", "Font family", "Sans"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:text:size", "Text size", 17));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:text:work_prefix", "Work timer prefix", "🍅"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:text:rest_prefix", "Rest timer prefix", "☕"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:text:waiting_prefix", "Waiting timer prefix", "⏸"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:text:skip_on_click", "Skip session on click", 1));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:sound:player", "Sound player command", "pw-play"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:sound:work_end", "Sound file for work end", ""));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:sound:rest_end", "Sound file for rest end", ""));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:notification:enabled", "Enable notifications", 1));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:notification:use_system_notifications", "Use system notifications", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:notification:work_end", "Work end notification text", "Work session complete"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:notification:rest_end", "Rest end notification text", "Break is over"));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:exec_on_work_end", "Command to run on work end", ""));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:exec_on_rest_end", "Command to run on rest end", ""));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:buttons:size", "Button size", 17));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Color>("plugin:hyprmodoro:buttons:color:foreground", "Button foreground color", 0xffffffff));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Color>("plugin:hyprmodoro:buttons:color:background", "Button background color", 0x44ffffff));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:enabled", "Enable title bar", 1));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:reserve_space_all", "Reserve space on all windows", 0));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:floating_window", "Show title on floating windows", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:all_windows", "Show title on all windows", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:margin", "Title margin", 15));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:spacing", "Title spacing", 8));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<String>("plugin:hyprmodoro:title:position", "Title position", "top"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:title:overlay", "Overlay title on window", 0));

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:hover:text", "Show text on hover", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:hover:buttons", "Show buttons on hover", 1));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Float>("plugin:hyprmodoro:hover:height", "Hover zone height percent", 10.f));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Float>("plugin:hyprmodoro:hover:width", "Hover zone width percent", 20.f));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:window:min_width", "Minimum window width for title", 300));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Int>("plugin:hyprmodoro:window:padding", "Window padding", 0));

    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprmodoro:start", startTimer);
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprmodoro:stop", stopTimer);
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprmodoro:pause", togglePauseTimer);
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprmodoro:set", setTimer);
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprmodoro:setRest", setRest);
    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprmodoro:skip", skip);

    HyprlandAPI::registerHyprCtlCommand(PHANDLE, SHyprCtlCommand{"hyprmodoro:getProgress", true, getProgress});
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, SHyprCtlCommand{"hyprmodoro:getState", true, getState});
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, SHyprCtlCommand{"hyprmodoro:getTime", true, getTime});

    static auto const SESSIONLENGTH = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:work_duration"};
    static auto const RESTLENGTH = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:rest_duration"};

    g_pGlobalState->pomodoroSession = makeUnique<Pomodoro>(*SESSIONLENGTH, *RESTLENGTH);

    // Apply autoTransition setting with safety check
    g_pGlobalState->pomodoroSession->setAutoTransition(getEffectiveAutoTransition());

    g_pGlobalState->pomodoroSession->setOnSessionEndCallback([](State endedState) {
        static auto const PSOUNDPLAYER = CConfigValue<std::string>{"plugin:hyprmodoro:sound:player"};
        static auto const PWORKENDFILE = CConfigValue<std::string>{"plugin:hyprmodoro:sound:work_end"};
        static auto const PRESTENDFILE = CConfigValue<std::string>{"plugin:hyprmodoro:sound:rest_end"};
        static auto const PWORKENDNOTIF = CConfigValue<std::string>{"plugin:hyprmodoro:notification:work_end"};
        static auto const PRESTENDNOTIF = CConfigValue<std::string>{"plugin:hyprmodoro:notification:rest_end"};
        static auto const PNOTIFENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:notification:enabled"};
        static auto const PEXECWORKEND = CConfigValue<std::string>{"plugin:hyprmodoro:exec_on_work_end"};
        static auto const PEXECRESTEND = CConfigValue<std::string>{"plugin:hyprmodoro:exec_on_rest_end"};

        bool soundPlayed      = false;
        bool soundConfigured  = false;
        // Try to play sound if player and sound files are configured
        std::string soundFile = (endedState == State::WORKING) ? std::string(*PWORKENDFILE) : std::string(*PRESTENDFILE);
        std::string player    = std::string(*PSOUNDPLAYER);

        if (!player.empty() && !soundFile.empty()) {
            soundConfigured = true;
            soundPlayed     = playSound(soundFile, player);
        }

        // Show notification if enabled, or as fallback if sound was configured but failed to play
        if (*PNOTIFENABLED || (soundConfigured && !soundPlayed)) {
            std::string message = (endedState == State::WORKING) ? std::string(*PWORKENDNOTIF) : std::string(*PRESTENDNOTIF);
            CHyprColor  color   = (endedState == State::WORKING) ? CHyprColor{0.2, 1.0, 0.2, 1.0} : CHyprColor{0.2, 0.6, 1.0, 1.0};
            sendNotification(message, color);
        }

        // custom callback execution
        const std::string& exec_commands = (endedState == State::WORKING) ? *PEXECWORKEND : *PEXECRESTEND;

        if (!exec_commands.empty()) {
            std::stringstream ss(exec_commands);
            std::string       command;
            while (std::getline(ss, command, ';')) {
                // Trim leading/trailing whitespace
                command.erase(0, command.find_first_not_of(" \t\n\r"));
                command.erase(command.find_last_not_of(" \t\n\r") + 1);
                if (!command.empty()) {
                    executeCommand(command);
                }
            }
        }
    });

    static auto configReloadCallback = Event::bus()->m_events.config.reloaded.listen(onConfigReload);

    static auto closeWindowCallback = Event::bus()->m_events.window.close.listen([](PHLWINDOW pWindow) { onWindowClose(nullptr, pWindow); });

    static auto openWindowCallback = Event::bus()->m_events.window.open.listen([](PHLWINDOW pWindow) { onWindowOpen(nullptr, pWindow); });

    for (auto& window : g_pCompositor->m_windows) {
        if (window->isHidden() || !window->m_isMapped)
            continue;
        onWindowOpen(nullptr, window);
    }

    return {"hyprmodoro", "A Pomodoro timer plugin for Hyprland", "0xFMD", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    for (auto& m : g_pCompositor->m_monitors)
        m->m_scheduledRecalc = true;

    g_pHyprRenderer->m_renderPass.removeAllOfType("HyprmodoroPassElement");
}
