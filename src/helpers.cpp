#include <hyprland/src/config/shared/animation/AnimationTree.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/protocols/LayerShell.hpp>
#include <hyprland/src/managers/animation/AnimationManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>

#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>

#include "hyprmodoroDecoration.hpp"

Vector2D HyprmodoroDecoration::cursorRelativeToContainer() {
    const auto PMONITOR  = m_pWindow.lock()->m_monitor.lock();
    auto       cursorPos = g_pInputManager->getMouseCoordsInternal() - PMONITOR->m_position;
    cursorPos.x *= PMONITOR->m_scale;
    cursorPos.y *= PMONITOR->m_scale;
    return cursorPos - m_layout.container.pos();
}

bool HyprmodoroDecoration::isHoveringTitle(const CBox& windowBox, const float& scale) {
    if (!isValidInput())
        return false;

    static auto const PHOVERTITLE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:text"};
    static auto const PHOVERBUTTONS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:buttons"};
    static auto const PHOVERHEIGHT = CConfigValue<Config::FLOAT>{"plugin:hyprmodoro:hover:height"};
    static auto const PHOVERWIDTH = CConfigValue<Config::FLOAT>{"plugin:hyprmodoro:hover:width"};
    static auto const PTITLEPOSITION = CConfigValue<std::string>{"plugin:hyprmodoro:title:position"};
    static auto const PTITLEOVERLAY = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:overlay"};

    const auto         cursorPos = cursorRelativeToContainer();

    const float        hoverWidth  = windowBox.width * scale * (*PHOVERWIDTH / 100.0f);
    const float        hoverHeight = windowBox.height * scale * (*PHOVERHEIGHT / 100.0f);

    const std::string  position   = std::string(*PTITLEPOSITION);
    const bool         isVertical = (position == "left" || position == "right");
    const bool         isOverlay  = isVertical || *PTITLEOVERLAY;

    CBox               hoverBox;
    if (isOverlay) {
        // For overlay, the container floats on the window — extend hover zone in all directions
        hoverBox = CBox(-hoverWidth, -hoverHeight, (hoverWidth * 2.0f) + m_layout.container.width, (hoverHeight * 2.0f) + m_layout.container.height);
    } else {
        hoverBox = CBox((m_layout.container.width - hoverWidth) * 0.5f, -hoverHeight, hoverWidth, (hoverHeight * 2.0f) + m_layout.container.height);
    }

    return hoverBox.containsPoint(cursorPos);
}

void HyprmodoroDecoration::updateHoverOffset() {
    static auto const PTEXTHOVER = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:text"};
    static auto const PBUTTONHOVER = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:buttons"};
    static auto const PTITLEMARGIN = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:margin"};
    static auto const PBUTTONSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:size"};
    static auto const PSPACING = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:spacing"};
    static auto const PTEXTSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:size"};

    const auto         buttonsSpace = *PSPACING + *PBUTTONSIZE;
    const auto         textHover    = *PTEXTHOVER;
    const auto         buttonHover  = *PBUTTONHOVER;
    const auto         textSize     = *PTEXTSIZE;

    m_hoverOffset->setValueAndWarp(*PTITLEMARGIN + textSize + (((buttonHover && m_isNearContainer) || !buttonHover) ? buttonsSpace : 0.0f));
}

bool HyprmodoroDecoration::isValidInput() {
    static auto const PENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:enabled"};

    if (!*PENABLED)
        return false;

    const auto PWINDOW = m_pWindow.lock();

    if (!m_pWindow->m_workspace || !validMapped(m_pWindow) || !m_pWindow->m_workspace->isVisible() || !g_pInputManager->m_exclusiveLSes.empty() ||
        (g_pSeatManager->m_seatGrab && !g_pSeatManager->m_seatGrab->accepts(m_pWindow->resource())))
        return false;

    const auto WINDOWATCURSOR = g_pCompositor->vectorToWindowUnified(g_pInputManager->getMouseCoordsInternal(), Desktop::View::RESERVED_EXTENTS | Desktop::View::INPUT_EXTENTS | Desktop::View::ALLOW_FLOATING);

    if (WINDOWATCURSOR != PWINDOW && PWINDOW != Desktop::focusState()->window())
        return false;

    // Check if input is on top or overlay shell layers
    auto     PMONITOR     = Desktop::focusState()->monitor();
    PHLLS    foundSurface = nullptr;
    Vector2D surfaceCoords;

    // Check top layer
    g_pCompositor->vectorToLayerSurface(g_pInputManager->getMouseCoordsInternal(), &PMONITOR->m_layerSurfaceLayers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &surfaceCoords, &foundSurface);

    if (foundSurface)
        return false;

    // Check overlay layer
    g_pCompositor->vectorToLayerSurface(g_pInputManager->getMouseCoordsInternal(), &PMONITOR->m_layerSurfaceLayers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &surfaceCoords,
                                        &foundSurface);

    if (foundSurface)
        return false;

    // if (!VECINRECT(cursorRelativeToContainer(), 0, 0, m_layout.container.width, m_layout.container.height))
    //     return false;

    return true;
}

void HyprmodoroDecoration::setupButtons() {
    static auto const PBUTTONSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:size"};
    static auto const PBUTTONFOREGROUND = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:color:foreground"};
    static auto const PBUTTONBACKGROUND = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:color:background"};

    m_vButtons[ButtonAction::STOP]    = {.action = ButtonAction::STOP,
                                         .color =
                                             Button::ButtonColor{.foreground = CHyprColor((uint64_t)*PBUTTONFOREGROUND), .background = CHyprColor((uint64_t)*PBUTTONBACKGROUND)},
                                         .size = (float)*PBUTTONSIZE,
                                         .icon = "⏹"};
    m_vButtons[ButtonAction::START]   = {.action = ButtonAction::START,
                                         .color =
                                             Button::ButtonColor{.foreground = CHyprColor((uint64_t)*PBUTTONFOREGROUND), .background = CHyprColor((uint64_t)*PBUTTONBACKGROUND)},
                                         .size = (float)*PBUTTONSIZE,
                                         .icon = "⏵"};
    m_vButtons[ButtonAction::RESTART] = {.action = ButtonAction::RESTART,
                                         .color =
                                             Button::ButtonColor{.foreground = CHyprColor((uint64_t)*PBUTTONFOREGROUND), .background = CHyprColor((uint64_t)*PBUTTONBACKGROUND)},
                                         .size = (float)*PBUTTONSIZE,
                                         .icon = "↻"};

    g_pAnimationManager->createAnimation(0.0f, m_vButtons[ButtonAction::STOP].opacity, Config::animationTree()->getAnimationPropertyConfig("fadeOut"), AVARDAMAGE_ENTIRE);
    g_pAnimationManager->createAnimation(0.0f, m_vButtons[ButtonAction::START].opacity, Config::animationTree()->getAnimationPropertyConfig("fadeOut"), AVARDAMAGE_ENTIRE);
    g_pAnimationManager->createAnimation(0.0f, m_vButtons[ButtonAction::RESTART].opacity, Config::animationTree()->getAnimationPropertyConfig("fadeOut"), AVARDAMAGE_ENTIRE);

    g_pAnimationManager->createAnimation(Vector2D(0, 0), m_vButtons[ButtonAction::START].position, Config::animationTree()->getAnimationPropertyConfig("windowsMove"), AVARDAMAGE_ENTIRE);
    g_pAnimationManager->createAnimation(Vector2D(0, 0), m_vButtons[ButtonAction::STOP].position, Config::animationTree()->getAnimationPropertyConfig("windowsMove"), AVARDAMAGE_ENTIRE);
    g_pAnimationManager->createAnimation(Vector2D(0, 0), m_vButtons[ButtonAction::RESTART].position, Config::animationTree()->getAnimationPropertyConfig("windowsMove"),
                                         AVARDAMAGE_ENTIRE);
}

// Sends a notification using the configured method
// Tries system notifications first if enabled, falls back to Hyprland API if they fail
void sendNotification(const std::string& message, const CHyprColor& color) {
    static auto const PUSESYSNOTIF =
        CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:notification:use_system_notifications"};

    bool notificationSent = false;

    // Try system notifications if configured
    if (*PUSESYSNOTIF) {
        notificationSent = sendLibnotifyNotification("Hyprmodoro", message);
        // Fall back to Hyprland API notifications
        if (!notificationSent) {
            HyprlandAPI::addNotification(PHANDLE, std::format("[hyprmodoro] {}", message), color, 5000);
        }
    }
}


bool executeCommand(const std::string& command) {
    if (command.empty()) {
        return false;
    }

    std::string commandWithBg = command + " &";
    int result = system(commandWithBg.c_str());
    return result == 0;
}

static bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool playSound(const std::string& soundFile, const std::string& player) {
    if (soundFile.empty() || !fileExists(soundFile) || player.empty()) {
        return false;
    }

    std::string command = player + " \"" + soundFile + "\" > /dev/null 2>&1";
    return executeCommand(command);
}

// Helper function to escape shell arguments
// Replaces single quotes with '\'' to safely pass arguments to shell
static std::string escapeShellArg(const std::string& arg) {
    std::string escaped;
    for (char c : arg) {
        if (c == '\'') {
            // End quote, add escaped quote, start new quote
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

// Sends a notification via libnotify (notify-send)
// Returns true if successful, false otherwise
bool sendLibnotifyNotification(const std::string& title, const std::string& message) {
    if (message.empty()) {
        return false;
    }

    // Check if notify-send is available
    std::string checkCmd = "command -v notify-send > /dev/null 2>&1";
    if (system(checkCmd.c_str()) != 0) {
        return false;
    }

    // Build notification command with proper escaping
    // Use app-name for proper categorization and urgency normal
    std::string escapedTitle = escapeShellArg(title);
    std::string escapedMessage = escapeShellArg(message);
    std::string command = "notify-send -a 'Hyprmodoro' -u normal '" + escapedTitle + "' '" + escapedMessage + "' 2>&1";

    int result = system(command.c_str());
    return (result == 0);
}
