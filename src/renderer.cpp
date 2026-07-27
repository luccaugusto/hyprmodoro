#include <hyprland/src/desktop/view/Window.hpp>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include "hyprmodoroDecoration.hpp"
#include "pomodoro.hpp"

void HyprmodoroDecoration::renderTitleBar(PHLMONITOR pMonitor, float alpha) {
    static auto const PTITLEFLOATINGWINDOW = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:floating_window"};
    static auto const PMINWINDOWWIDTH = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:window:min_width"};
    static auto const PTITLEPOSITION = CConfigValue<std::string>{"plugin:hyprmodoro:title:position"};
    static auto const PTEXTSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:size"};
    static auto const PTITLEMARGIN = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:margin"};
    static auto const PTITLEOVERLAY = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:overlay"};

    if (!*PTITLEFLOATINGWINDOW && m_pWindow.lock()->m_isFloating)
        return;

    const auto windowBox = assignedBoxGlobal();
    if (windowBox.width <= 0 || windowBox.height <= 0)
        return;

    updateHoverOffset();
    const auto        yOffset    = m_hoverOffset->value();
    const std::string position   = std::string(*PTITLEPOSITION);
    const bool        isVertical = (position == "left" || position == "right");
    const bool        isOverlay  = isVertical || *PTITLEOVERLAY;

    // For non-overlay horizontal positions, skip small windows
    if (!isOverlay && windowBox.width <= *PMINWINDOWWIDTH)
        return;

    if (isOverlay) {
        float containerW, containerH, containerX, containerY;
        if (isVertical) {
            containerW = *PTITLEMARGIN * 2 + *PTEXTSIZE * 6;
            containerH = yOffset;
            containerX = (position == "left") ? windowBox.x : (windowBox.x + windowBox.width - containerW);
            containerY = windowBox.y + (windowBox.height - containerH) / 2;
        } else {
            containerW = windowBox.width / 2;
            containerH = yOffset;
            containerX = windowBox.x + (windowBox.width - containerW) / 2;
            containerY = (position == "bottom") ? (windowBox.y + windowBox.height - containerH) : windowBox.y;
        }
        m_layout.container = CBox(containerX, containerY, containerW, containerH);
    } else {
        float containerY = (position == "bottom") ? (windowBox.y + windowBox.height - yOffset) : windowBox.y;
        m_layout.container = CBox(windowBox.x + (windowBox.width - windowBox.width / 2) / 2, containerY, windowBox.width / 2, yOffset);
    }

    m_layout.container.translate(-pMonitor->m_position).scale(pMonitor->m_scale).round();

    if (m_pTitleTex->m_texID == 0)
        m_pTitleTex->allocate({m_layout.container.width, m_layout.container.height});

    const auto CAIROSURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_layout.container.width, m_layout.container.height);

    const auto CAIRO = cairo_create(CAIROSURFACE);

    // Clear surface
    cairo_save(CAIRO);
    cairo_set_operator(CAIRO, CAIRO_OPERATOR_CLEAR);
    cairo_paint(CAIRO);
    cairo_restore(CAIRO);

    renderTimer(CAIRO, Vector2D(m_layout.container.width, m_layout.container.height), pMonitor->m_scale);
    renderButtons(CAIRO, Vector2D(m_layout.container.width, m_layout.container.height), pMonitor->m_scale);

    // Draw dark grey transparent background behind content for overlay positions
    if (isOverlay) {
        float bgOpacity = std::max(m_textOpacity->value(), m_vButtons[ButtonAction::START].opacity->value());
        if (bgOpacity > 0.0f) {
            cairo_save(CAIRO);
            cairo_set_operator(CAIRO, CAIRO_OPERATOR_DEST_OVER);
            cairo_set_source_rgba(CAIRO, 0.2, 0.2, 0.2, 0.55 * bgOpacity);
            cairo_rectangle(CAIRO, 0, 0, m_layout.container.width, m_layout.container.height);
            cairo_fill(CAIRO);
            cairo_restore(CAIRO);
        }
    }

    cairo_surface_flush(CAIROSURFACE);
    const auto DATA = cairo_image_surface_get_data(CAIROSURFACE);

    glBindTexture(GL_TEXTURE_2D, m_pTitleTex->m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_layout.container.width, m_layout.container.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);
    Render::GL::g_pHyprOpenGL->renderTexture(m_pTitleTex, m_layout.container, {.a = alpha});

    cairo_destroy(CAIRO);
    cairo_surface_destroy(CAIROSURFACE);
}

void HyprmodoroDecoration::renderTimer(cairo_t* cairo, const Vector2D& buffer, const float& scale) {
    static auto const PTEXTCOLOR = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:color"};
    static auto const PFONT = CConfigValue<std::string>{"plugin:hyprmodoro:text:font"};
    static auto const PTEXTSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:size"};
    static auto const PTEXTHOVER = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:text"};
    static auto const PRESTPREFIX = CConfigValue<std::string>{"plugin:hyprmodoro:text:rest_prefix"};
    static auto const PWORKPREFIX = CConfigValue<std::string>{"plugin:hyprmodoro:text:work_prefix"};
    static auto const PWAITINGPREFIX = CConfigValue<std::string>{"plugin:hyprmodoro:text:waiting_prefix"};

    const auto         textSize      = *PTEXTSIZE * scale;
    const auto         pomodoroState = g_pGlobalState->pomodoroSession->getState();
    const auto         textHover     = *PTEXTHOVER;
    const auto         timeText      = g_pGlobalState->pomodoroSession->getFormattedTime();

    std::string displayText;
    if (pomodoroState == PomodoroState::RESTING) {
        displayText = std::string(*PRESTPREFIX) + " " + timeText;
    } else if (pomodoroState == PomodoroState::WAITING_FOR_REST || pomodoroState == PomodoroState::WAITING_FOR_WORK) {
        displayText = std::string(*PWAITINGPREFIX) + " " + timeText;
    } else {
        displayText = std::string(*PWORKPREFIX) + " " + timeText;
    }
    
    CHyprColor         textColor   = CHyprColor((uint64_t)*PTEXTCOLOR);

    if (pomodoroState == PomodoroState::STOPPED)
        *m_textOpacity = 0.0f;
    else if (textHover)
        *m_textOpacity = m_isNearContainer ? 1.0f : 0.0f;
    else
        *m_textOpacity = 1.0f;

    cairo_push_group(cairo);
    cairo_set_source_rgba(cairo, textColor.r, textColor.g, textColor.b, textColor.a);

    PangoLayout* layout = pango_cairo_create_layout(cairo);
    pango_layout_set_text(layout, displayText.c_str(), -1);

    PangoFontDescription* fontDesc = pango_font_description_from_string((*PFONT).c_str());
    pango_font_description_set_size(fontDesc, textSize * PANGO_SCALE);
    pango_layout_set_font_description(layout, fontDesc);
    int layoutWidth, layoutHeight;
    pango_layout_get_size(layout, &layoutWidth, &layoutHeight);

    const Vector2D textCairo = Vector2D((double)(layoutWidth / PANGO_SCALE), (double)(layoutHeight / PANGO_SCALE));

    m_layout.title = CBox((buffer.x - textCairo.x) / 2, 0, textCairo.x, textCairo.y);

    cairo_move_to(cairo, m_layout.title.x, 0);
    pango_cairo_show_layout(cairo, layout);
    g_object_unref(layout);
    pango_font_description_free(fontDesc);

    cairo_pattern_t* pattern = cairo_pop_group(cairo);
    cairo_set_source(cairo, pattern);
    cairo_paint_with_alpha(cairo, m_textOpacity->value());
    cairo_pattern_destroy(pattern);
}

void HyprmodoroDecoration::renderButtons(cairo_t* cairo, const Vector2D& buffer, const float& scale) {
    static auto const PFONT = CConfigValue<std::string>{"plugin:hyprmodoro:text:font"};
    static auto const PBUTTONSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:size"};
    static auto const PTEXTSIZE = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:text:size"};
    static auto const PSPACING = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:title:spacing"};
    static auto const PBUTTONSFOREGROUND = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:color:foreground"};
    static auto const PBUTTONSBACKGROUND = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:buttons:color:background"};
    static auto const PBUTTONHOVER = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:hover:buttons"};

    const auto         currentState      = g_pGlobalState->pomodoroSession->getState();
    const std::string  PLAYICON          = g_pGlobalState->pomodoroSession->isPaused() || currentState == PomodoroState::STOPPED ? "⏵" : "⏸";
    m_vButtons[ButtonAction::START].icon = PLAYICON;

    const auto     buttonSize   = *PBUTTONSIZE * scale;
    const auto     buttonHover  = *PBUTTONHOVER;
    const auto     offset       = (*PSPACING + *PTEXTSIZE) * scale;
    const auto     spacing      = buttonSize * 1.5f;
    const Vector2D centerCoords = Vector2D(buffer.x / 2, offset + (buttonSize / 2.0f));

    const bool     isFirstRender = (m_vButtons[ButtonAction::START].position->value() == Vector2D(0, 0));
    auto           setPosition   = [&isFirstRender](PHLANIMVAR<Vector2D>& pos, const Vector2D& target) {
        if (isFirstRender) {
            pos->setValueAndWarp(target); // don't animate position
        } else {
            *pos = target;
        }
    };

    setPosition(m_vButtons[ButtonAction::START].position, Vector2D(centerCoords.x - spacing - buttonSize / 2, centerCoords.y));
    setPosition(m_vButtons[ButtonAction::STOP].position, Vector2D(centerCoords.x - buttonSize / 2, centerCoords.y));
    setPosition(m_vButtons[ButtonAction::RESTART].position, Vector2D(centerCoords.x + spacing - buttonSize / 2, centerCoords.y));

    m_vButtons[ButtonAction::STOP].visible    = true;
    m_vButtons[ButtonAction::RESTART].visible = true;

    if (currentState == PomodoroState::STOPPED) {
        // only render start button
        setPosition(m_vButtons[ButtonAction::START].position, Vector2D(centerCoords.x - buttonSize / 2, centerCoords.y));
        m_vButtons[ButtonAction::STOP].visible    = false;
        m_vButtons[ButtonAction::RESTART].visible = false;
    }

    for (auto& [_, button] : m_vButtons) {

        if (!button.visible)
            *button.opacity = 0.0f;
        else
            *button.opacity = (!buttonHover || m_isNearContainer) ? 1.0f : 0.0f;

        button.size             = buttonSize;
        button.color.foreground = CHyprColor((uint64_t)*PBUTTONSFOREGROUND);
        button.color.background = CHyprColor((uint64_t)*PBUTTONSBACKGROUND);
        button.hitbox           = CBox(button.position->value().x, button.position->value().y, buttonSize, buttonSize);

        const Vector2D centerButton = Vector2D(button.hitbox.x + button.hitbox.width / 2, button.hitbox.y + button.hitbox.height / 2);
        float          radius       = button.hitbox.width / 2;

        // Draw circle background
        cairo_set_source_rgba(cairo, button.color.background.r, button.color.background.g, button.color.background.b, button.color.background.a * button.opacity->value());
        cairo_arc(cairo, centerButton.x, centerButton.y, radius, 0, 2 * M_PI);
        cairo_fill(cairo);

        // Create icon layout
        PangoLayout* iconLayout = pango_cairo_create_layout(cairo);
        pango_layout_set_text(iconLayout, button.icon.c_str(), -1);

        PangoFontDescription* iconFont = pango_font_description_from_string((*PFONT).c_str());
        pango_font_description_set_size(iconFont, (button.size * 0.5f) * PANGO_SCALE);
        pango_layout_set_font_description(iconLayout, iconFont);

        // Get text extents for  centering
        PangoRectangle inkRect, logicalRect;
        pango_layout_get_extents(iconLayout, &inkRect, &logicalRect);

        const CBox     iconBox = CBox(logicalRect.x, logicalRect.y, logicalRect.width, logicalRect.height).scale(1.0f / PANGO_SCALE).round();

        const Vector2D iconTextPos = Vector2D(centerButton.x - (iconBox.width / 2) - iconBox.x, centerButton.y - (iconBox.height / 2) - iconBox.y);

        // Draw icon text
        cairo_set_source_rgba(cairo, button.color.foreground.r, button.color.foreground.g, button.color.foreground.b, button.color.foreground.a * button.opacity->value());
        cairo_move_to(cairo, iconTextPos.x, iconTextPos.y);
        pango_cairo_show_layout(cairo, iconLayout);

        g_object_unref(iconLayout);
        pango_font_description_free(iconFont);
    }
}

void HyprmodoroDecoration::renderProgressBorder(PHLMONITOR pMonitor, float alpha) {
    static auto const BORDERCOLOR = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:border:color"};
    static auto const BORDERFLOATINGWINDOW = CConfigValue<Config::INTEGER>{"plugin:hyprmodoro:border:floating_window"};
    static auto const PTITLEPOSITION = CConfigValue<std::string>{"plugin:hyprmodoro:title:position"};
    if (!*BORDERFLOATINGWINDOW && m_pWindow.lock()->m_isFloating)
        return;

    const auto        SESSIONPROGRESS = g_pGlobalState->pomodoroSession->getProgress();

    auto              windowBox = assignedBoxGlobal();
    const auto        PWINDOW   = m_pWindow.lock();
    const auto        BORDER    = PWINDOW->getRealBorderSize() * pMonitor->m_scale;
    const auto        ROUNDING  = PWINDOW->rounding() * pMonitor->m_scale;
    const std::string position  = std::string(*PTITLEPOSITION);

    const auto   corner       = ROUNDING + BORDER;
    const float  cb           = BORDER * 0.5f;
    const float  cc           = corner - cb;

    const float  perimeter    = 2 * (windowBox.width + windowBox.height);
    const double targetLength = perimeter * SESSIONPROGRESS * 0.5;

    windowBox.translate(-pMonitor->m_position).scale(pMonitor->m_scale).round();

    const float w = windowBox.width;
    const float h = windowBox.height;

    // Corner arc centers
    const float tlx = cc + cb, tly = cc + cb;
    const float trx = w - cc - cb, try_ = cc + cb;
    const float blx = cc + cb, bly = h - cc - cb;
    const float brx = w - cc - cb, bry = h - cc - cb;

    const CHyprColor borderColor  = CHyprColor(*BORDERCOLOR);
    const auto       CAIROSURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    const auto       CAIRO        = cairo_create(CAIROSURFACE);
    const double     dashes[]     = {targetLength, perimeter};

    cairo_set_operator(CAIRO, CAIRO_OPERATOR_CLEAR);
    cairo_paint(CAIRO);
    cairo_set_operator(CAIRO, CAIRO_OPERATOR_OVER);

    cairo_set_source_rgba(CAIRO, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    cairo_set_line_width(CAIRO, BORDER);

    if (ROUNDING > 0) {
        if (position == "bottom") {
            // CW path: bottom center -> right -> top center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, h - cb);
            cairo_line_to(CAIRO, brx, h - cb);
            cairo_arc_negative(CAIRO, brx, bry, cc, M_PI_2, 0);
            cairo_line_to(CAIRO, w - cb, try_);
            cairo_arc_negative(CAIRO, trx, try_, cc, 0, -M_PI_2);
            cairo_line_to(CAIRO, w * 0.5f, cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            // CCW path: bottom center -> left -> top center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, h - cb);
            cairo_line_to(CAIRO, blx, h - cb);
            cairo_arc(CAIRO, blx, bly, cc, M_PI_2, M_PI);
            cairo_line_to(CAIRO, cb, tly);
            cairo_arc(CAIRO, tlx, tly, cc, M_PI, 3 * M_PI_2);
            cairo_line_to(CAIRO, w * 0.5f, cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        } else if (position == "left") {
            // CW path: left center -> up -> right center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, cb, h * 0.5f);
            cairo_line_to(CAIRO, cb, tly);
            cairo_arc(CAIRO, tlx, tly, cc, -M_PI, -M_PI_2);
            cairo_line_to(CAIRO, trx, cb);
            cairo_arc(CAIRO, trx, try_, cc, -M_PI_2, 0);
            cairo_line_to(CAIRO, w - cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            // CCW path: left center -> down -> right center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, cb, h * 0.5f);
            cairo_line_to(CAIRO, cb, bly);
            cairo_arc_negative(CAIRO, blx, bly, cc, M_PI, M_PI_2);
            cairo_line_to(CAIRO, brx, h - cb);
            cairo_arc_negative(CAIRO, brx, bry, cc, M_PI_2, 0);
            cairo_line_to(CAIRO, w - cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        } else if (position == "right") {
            // CW path: right center -> down -> left center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w - cb, h * 0.5f);
            cairo_line_to(CAIRO, w - cb, bry);
            cairo_arc(CAIRO, brx, bry, cc, 0, M_PI_2);
            cairo_line_to(CAIRO, blx, h - cb);
            cairo_arc(CAIRO, blx, bly, cc, M_PI_2, M_PI);
            cairo_line_to(CAIRO, cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            // CCW path: right center -> up -> left center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w - cb, h * 0.5f);
            cairo_line_to(CAIRO, w - cb, try_);
            cairo_arc_negative(CAIRO, trx, try_, cc, 0, -M_PI_2);
            cairo_line_to(CAIRO, tlx, cb);
            cairo_arc_negative(CAIRO, tlx, tly, cc, -M_PI_2, -M_PI);
            cairo_line_to(CAIRO, cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        } else {
            // Top (default): CW path: top center -> right -> bottom center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, cb);
            cairo_line_to(CAIRO, trx, cb);
            cairo_arc(CAIRO, trx, try_, cc, -M_PI_2, 0);
            cairo_line_to(CAIRO, w - cb, bry);
            cairo_arc(CAIRO, brx, bry, cc, 0, M_PI_2);
            cairo_line_to(CAIRO, w * 0.5f, h - cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            // CCW path: top center -> left -> bottom center
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, cb);
            cairo_line_to(CAIRO, tlx, cb);
            cairo_arc_negative(CAIRO, tlx, tly, cc, -M_PI_2, -M_PI);
            cairo_line_to(CAIRO, cb, bly);
            cairo_arc_negative(CAIRO, blx, bly, cc, -M_PI, -3 * M_PI_2);
            cairo_line_to(CAIRO, w * 0.5f, h - cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        }
    } else {
        if (position == "bottom") {
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, h - cb);
            cairo_line_to(CAIRO, w - cb, h - cb);
            cairo_line_to(CAIRO, w - cb, cb);
            cairo_line_to(CAIRO, w * 0.5f, cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, h - cb);
            cairo_line_to(CAIRO, cb, h - cb);
            cairo_line_to(CAIRO, cb, cb);
            cairo_line_to(CAIRO, w * 0.5f, cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        } else if (position == "left") {
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, cb, h * 0.5f);
            cairo_line_to(CAIRO, cb, cb);
            cairo_line_to(CAIRO, w - cb, cb);
            cairo_line_to(CAIRO, w - cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, cb, h * 0.5f);
            cairo_line_to(CAIRO, cb, h - cb);
            cairo_line_to(CAIRO, w - cb, h - cb);
            cairo_line_to(CAIRO, w - cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        } else if (position == "right") {
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w - cb, h * 0.5f);
            cairo_line_to(CAIRO, w - cb, h - cb);
            cairo_line_to(CAIRO, cb, h - cb);
            cairo_line_to(CAIRO, cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w - cb, h * 0.5f);
            cairo_line_to(CAIRO, w - cb, cb);
            cairo_line_to(CAIRO, cb, cb);
            cairo_line_to(CAIRO, cb, h * 0.5f);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        } else {
            // Top (default)
            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, cb);
            cairo_line_to(CAIRO, w - cb, cb);
            cairo_line_to(CAIRO, w - cb, h - cb);
            cairo_line_to(CAIRO, w * 0.5f, h - cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);

            cairo_new_path(CAIRO);
            cairo_move_to(CAIRO, w * 0.5f, cb);
            cairo_line_to(CAIRO, cb, cb);
            cairo_line_to(CAIRO, cb, h - cb);
            cairo_line_to(CAIRO, w * 0.5f, h - cb);
            cairo_set_dash(CAIRO, dashes, 2, 0);
            cairo_stroke(CAIRO);
        }
    }

    if (m_pProgressTex->m_texID == 0) {
        m_pProgressTex->allocate({w, h});
    }

    cairo_set_dash(CAIRO, NULL, 0, 0);

    cairo_surface_flush(CAIROSURFACE);
    const auto DATA = cairo_image_surface_get_data(CAIROSURFACE);

    glBindTexture(GL_TEXTURE_2D, m_pProgressTex->m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowBox.width, windowBox.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);
    Render::GL::g_pHyprOpenGL->renderTexture(m_pProgressTex, windowBox, {.a = alpha});

    cairo_destroy(CAIRO);
    cairo_surface_destroy(CAIROSURFACE);
}