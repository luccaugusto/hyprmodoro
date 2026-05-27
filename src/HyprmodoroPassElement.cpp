#include <hyprland/src/render/Renderer.hpp>
#include "HyprmodoroPassElement.hpp"
#include "hyprmodoroDecoration.hpp"

HyprmodoroPassElement::HyprmodoroPassElement(const HyprmodoroPassElement::SBorderPPData& data_) : data(data_) {
    ;
}

std::vector<UP<IPassElement>> HyprmodoroPassElement::draw() {
    data.deco->drawPass(g_pHyprRenderer->m_renderData.pMonitor.lock(), data.a);
    return {};
}

bool HyprmodoroPassElement::needsLiveBlur() {
    return false;
}

bool HyprmodoroPassElement::needsPrecomputeBlur() {
    return false;
}