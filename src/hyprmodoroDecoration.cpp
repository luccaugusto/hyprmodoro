#include <hyprland/src/config/shared/animation/AnimationTree.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/animation/AnimationManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>

#include "hyprmodoroDecoration.hpp"
#include "HyprmodoroPassElement.hpp"
#include "pomodoro.hpp"

HyprmodoroDecoration::HyprmodoroDecoration(PHLWINDOW pWindow) : IHyprWindowDecoration(pWindow), m_pWindow(pWindow) {
    m_pTitleTex    = makeShared<Render::GL::CGLTexture>();
    m_pProgressTex = makeShared<Render::GL::CGLTexture>();
    setupButtons();

    m_pMouseButtonCallback = Event::bus()->m_events.input.mouse.button.listen([this](IPointer::SButtonEvent e, Event::SCallbackInfo& info) { onMouseDown(info, e); });

    Animation::mgr()->createAnimation(25.0f, m_hoverOffset, Config::animationTree()->getAnimationPropertyConfig("windowsMove"), AVARDAMAGE_ENTIRE);
    Animation::mgr()->createAnimation(0.0f, m_textOpacity, Config::animationTree()->getAnimationPropertyConfig("fadeOut"), AVARDAMAGE_ENTIRE);
}

HyprmodoroDecoration::~HyprmodoroDecoration() {
    m_pMouseButtonCallback = nullptr;
    m_vButtons.clear();
    std::erase(g_pGlobalState->decorations, m_self);
}

SDecorationPositioningInfo HyprmodoroDecoration::getPositioningInfo() {
    static auto const PISENABLED      = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:enabled"};
    static auto const PTITLEISENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:enabled"};
    static auto const PMINWINDOWWIDTH = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:window:min_width"};

    static auto const PRESERVESPACEALL = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:reserve_space_all"};
    static auto const PTITLEALLWINDOWS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:all_windows"};
    static auto const PTEXTHOVER       = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:text"};
    static auto const PBUTTONHOVER     = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:buttons"};
    static auto const PTITLEMARGIN     = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:margin"};
    static auto const PBUTTONSIZE      = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:size"};
    static auto const PSPACING         = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:spacing"};
    static auto const PTEXTSIZE        = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:size"};
    static auto const PWINDOWPADDING   = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:window:padding"};
    static auto const PTITLEPOSITION   = CConfigValue<std::string>{"plugin:hyprmodoro:title:position"};
    static auto const PTITLEOVERLAY    = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:overlay"};

    SDecorationPositioningInfo info;
    info.edges    = DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP;
    info.policy   = DECORATION_POSITION_STICKY;
    info.reserved = true;
    info.priority = 500;

    if (!*PISENABLED || !*PTITLEISENABLED)
        return info;

    const auto PWINDOW     = m_pWindow.lock();
    const auto PLASTWINDOW = Desktop::focusState()->window();

    if (!*PRESERVESPACEALL && !*PTITLEALLWINDOWS && PWINDOW != PLASTWINDOW) // only focused windows
        return info;

    if (PWINDOW->size(Desktop::View::IGeometric::GEOMETRIC_GOAL).x <= *PMINWINDOWWIDTH)
        return info;

    const auto        textSize      = *PTEXTSIZE;
    const auto        buttonsSpace  = *PSPACING + *PBUTTONSIZE;
    const auto        textHover     = *PTEXTHOVER;
    const auto        buttonHover   = *PBUTTONHOVER;
    const auto        titleMargin   = *PTITLEMARGIN;
    const auto        windowPadding = *PWINDOWPADDING;
    const std::string position      = std::string(*PTITLEPOSITION);
    const bool        isVertical    = (position == "left" || position == "right");
    const bool        isOverlay     = isVertical || *PTITLEOVERLAY;

    float offset = 0.0f;

    if (textHover && buttonHover && !m_isNearContainer) {
        offset = windowPadding;
    } else if ((buttonHover && m_isNearContainer) || !buttonHover) {
        offset = titleMargin + textSize + buttonsSpace;
    } else {
        offset = titleMargin + textSize;
    }

    if (isOverlay) {
        // Overlay mode: no space reservation, UI renders on top of window content.
        // For left/right this is required because Hyprland's 4-edge positioner
        // applies desiredSize uniformly to all sides.
        info.desiredExtents = {{0.0, 0.0}, {0.0, 0.0}};
    } else {
        if (position == "bottom")
            info.desiredExtents = {{0.0, 0.0}, {0.0, offset}};
        else
            info.desiredExtents = {{0.0, offset}, {0.0, 0.0}};
    }

    return info;
}

void HyprmodoroDecoration::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_bAssignedGeometry = reply.assignedGeometry;
}

uint64_t HyprmodoroDecoration::getDecorationFlags() {
    return DECORATION_PART_OF_MAIN_WINDOW | DECORATION_ALLOWS_MOUSE_INPUT;
}

eDecorationLayer HyprmodoroDecoration::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

eDecorationType HyprmodoroDecoration::getDecorationType() {
    return DECORATION_CUSTOM;
}

std::string HyprmodoroDecoration::getDisplayName() {
    return "hyprmodoro";
}

CBox HyprmodoroDecoration::assignedBoxGlobal() {

    CBox       box     = m_bAssignedGeometry;
    const auto PWINDOW = m_pWindow.lock();

    box.translate(g_pDecorationPositioner->getEdgeDefinedPoint(DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP, PWINDOW));

    const auto PWORKSPACE      = PWINDOW->m_workspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !PWINDOW->m_pinned ? PWORKSPACE->m_renderOffset->value() : Vector2D();

    return box.translate(WORKSPACEOFFSET);
}

void HyprmodoroDecoration::onMouseDown(Event::SCallbackInfo& info, IPointer::SButtonEvent e) {
    if (!isValidInput())
        return;

    if (e.state != WL_POINTER_BUTTON_STATE_PRESSED || e.button != BTN_LEFT)
        return;

    const auto         cursorPos = cursorRelativeToContainer();

    static auto const PSKIPONCLICK = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:skip_on_click"};

    const auto         currentState = g_pGlobalState->pomodoroSession->getState();
    const auto         isRunning    = currentState == PomodoroState::WORKING || currentState == PomodoroState::RESTING;

    if (m_layout.title.containsPoint(cursorPos) && *PSKIPONCLICK) {
        // If waiting for rest, start rest session
        if (currentState == PomodoroState::WAITING_FOR_REST) {
            g_pGlobalState->pomodoroSession->startRest();
            sendNotification("Starting rest", CHyprColor{0.0, 1.0, 0.0, 1.0});
            return;
        }
        
        // If waiting for work, start work session
        if (currentState == PomodoroState::WAITING_FOR_WORK) {
            g_pGlobalState->pomodoroSession->start();
            sendNotification("Starting work", CHyprColor{0.0, 1.0, 0.0, 1.0});
            return;
        }
        
        // If session is running, allow skip
        if (isRunning) {
            const std::string sessionType = (currentState == PomodoroState::WORKING) ? "work" : "rest";
            sendNotification(std::format("Skipped {}", sessionType), CHyprColor{0.0, 1.0, 0.0, 1.0});
            g_pGlobalState->pomodoroSession->skip();
            return;
        }
    }

    for (const auto& [buttonAction, button] : m_vButtons) {
        if (button.visible && button.hitbox.containsPoint(cursorPos)) {
            handleButtonClick(buttonAction);
            info.cancelled = true;
            return;
        }
    }
}

void HyprmodoroDecoration::handleButtonClick(ButtonAction buttonAction) {
    const auto currentState = g_pGlobalState->pomodoroSession->getState();
    if (buttonAction == ButtonAction::START) {
        if ((currentState == PomodoroState::WORKING || currentState == PomodoroState::RESTING) && !g_pGlobalState->pomodoroSession->isPaused())
            g_pGlobalState->pomodoroSession->pause();
        else if (g_pGlobalState->pomodoroSession->isPaused())
            g_pGlobalState->pomodoroSession->resume();
        else
            g_pGlobalState->pomodoroSession->start();

    } else if (buttonAction == ButtonAction::STOP)
        g_pGlobalState->pomodoroSession->stop();

    else if (buttonAction == ButtonAction::RESTART)
        g_pGlobalState->pomodoroSession->reset();
}

void HyprmodoroDecoration::draw(PHLMONITOR pMonitor, const float& a) {
    if (!validMapped(m_pWindow))
        return;

    const auto PWINDOW = m_pWindow.lock();

    if (!PWINDOW->m_ruleApplicator->decorate().valueOrDefault())
        return;

    if (!PWINDOW->m_workspace->isVisible() || PWINDOW->isHidden())
        return;

    HyprmodoroPassElement::SBorderPPData data;
    data.deco = this;

    g_pHyprRenderer->m_renderPass.add(makeUnique<HyprmodoroPassElement>(data));
}

void HyprmodoroDecoration::drawPass(PHLMONITOR pMonitor, const float& a) {
    static auto const PENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:enabled"};
    static auto const PHOVERTITLE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:text"};
    static auto const PHOVERBUTTONS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:buttons"};

    static auto const PBORDERENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:border:enabled"};
    static auto const PTITLEENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:enabled"};
    static auto const PBORDERALLWINDOWS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:border:all_windows"};
    static auto const PTITLEALLWINDOWS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:all_windows"};

    if (!*PENABLED)
        return;
    const auto windowBox = assignedBoxGlobal();
    if (windowBox.width <= 0 || windowBox.height <= 0)
        return;

    const auto SESSIONPROGRESS = g_pGlobalState->pomodoroSession->getProgress();
    const auto PWINDOW         = m_pWindow.lock();
    const auto PLASTWINDOW     = Desktop::focusState()->window();
    if (*PBORDERENABLED && SESSIONPROGRESS <= 1.0f && SESSIONPROGRESS > 0.0f) {
        if (*PBORDERALLWINDOWS || PWINDOW == PLASTWINDOW) {
            renderProgressBorder(pMonitor, a);
        }
    }

    if (*PTITLEENABLED) {

        if ((*PTITLEALLWINDOWS && (!Fullscreen::controller()->hasFullscreen(pMonitor->m_activeWorkspace) || Fullscreen::controller()->isFullscreen(PWINDOW))) ||
            PWINDOW == PLASTWINDOW) {
            renderTitleBar(pMonitor, a);
        }
    }
    updateWindow(m_pWindow.lock());
    damageEntire();
}

void HyprmodoroDecoration::updateWindow(PHLWINDOW pWindow) {
    static auto const PISTITLEENABLED = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:enabled"};

    if (!*PISTITLEENABLED || !g_pGlobalState->pomodoroSession)
        return;

    static auto const PTITLEALLWINDOWS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:all_windows"};
    static auto const PHOVERTITLE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:text"};
    static auto const PHOVERBUTTONS = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:buttons"};
    const auto         PWINDOW          = m_pWindow.lock();
    const auto         PLASTWINDOW      = Desktop::focusState()->window();
    const auto         PMONITOR         = PWINDOW->m_monitor.lock();
    bool               hoverState       = isHoveringTitle(assignedBoxGlobal(), PMONITOR->m_scale);
    bool               needReposition   = false;

    if (!*PTITLEALLWINDOWS && PWINDOW != PLASTWINDOW) {
        needReposition = true;
    }

    if (g_pGlobalState->pomodoroSession->getState() != g_pGlobalState->pomodoroSession->getLastState())
        needReposition = true;

    if (hoverState != m_isNearContainer && (*PHOVERTITLE || *PHOVERBUTTONS)) {
        m_isNearContainer = hoverState;
        needReposition    = true;
    }

    if (needReposition) {
        g_pDecorationPositioner->repositionDeco(this);
    }
}

void HyprmodoroDecoration::damageEntire() {
    g_pHyprRenderer->damageBox(assignedBoxGlobal());
}