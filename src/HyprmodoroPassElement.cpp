#include <hyprland/src/render/Renderer.hpp>
#include "HyprmodoroPassElement.hpp"
#include "hyprmodoroDecoration.hpp"

HyprmodoroPassElement::HyprmodoroPassElement(const HyprmodoroPassElement::SBorderPPData& data_) : data(data_) {
    ;
}

std::vector<UP<IPassElement>> HyprmodoroPassElement::draw(Render::CRenderContext& ctx) {
    data.deco->drawPass(ctx, ctx.m_data.pMonitor.lock(), data.a);
    return {};
}

bool HyprmodoroPassElement::needsLiveBlur(Render::CRenderContext&) {
    return false;
}

bool HyprmodoroPassElement::needsPrecomputeBlur(Render::CRenderContext&) {
    return false;
}