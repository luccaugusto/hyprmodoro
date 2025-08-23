#pragma once
#include <hyprland/src/render/pass/PassElement.hpp>

class HyprmodoroDecoration;

class HyprmodoroPassElement : public IPassElement {
  public:
    struct SBorderPPData {
        HyprmodoroDecoration* deco = nullptr;
        float                 a    = 1.F;
    };

    HyprmodoroPassElement(const SBorderPPData& data_);
    virtual ~HyprmodoroPassElement() = default;

    virtual void        draw(const CRegion& damage);
    virtual bool        needsLiveBlur();
    virtual bool        needsPrecomputeBlur();

    virtual const char* passName() {
        return "HyprmodoroPassElement";
    }

  private:
    SBorderPPData data;
};
