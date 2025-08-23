#include "HyprmodoroPassElement.hpp"
#include "hyprmodoroDecoration.hpp"

HyprmodoroPassElement::HyprmodoroPassElement(const HyprmodoroPassElement::SBorderPPData& data_) : data(data_) {
    ;
}

void HyprmodoroPassElement::draw(const CRegion& damage) {
    data.deco->drawPass(g_pHyprOpenGL->m_renderData.pMonitor.lock(), data.a);
}

bool HyprmodoroPassElement::needsLiveBlur() {
    return false;
}

bool HyprmodoroPassElement::needsPrecomputeBlur() {
    return false;
}