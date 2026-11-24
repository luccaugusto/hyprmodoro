#pragma once
#include <hyprland/src/render/pass/PassElement.hpp>
#include <hyprland/src/render/OpenGL.hpp>

class HyprmodoroDecoration;

class HyprmodoroPassElement : public IPassElement {
  public:
    struct SBorderPPData {
        HyprmodoroDecoration* deco = nullptr;
        float                 a    = 1.F;
    };

    HyprmodoroPassElement(const SBorderPPData& data_);
    virtual ~HyprmodoroPassElement() = default;

    virtual std::vector<UP<IPassElement>> draw(Render::CRenderContext& ctx) override;
    virtual bool                          needsLiveBlur(Render::CRenderContext&) override;
    virtual bool                          needsPrecomputeBlur(Render::CRenderContext&) override;
    virtual ePassElementType              type() override { return EK_CUSTOM; }

    virtual const char* passName() override {
        return "HyprmodoroPassElement";
    }

  private:
    SBorderPPData data;
};
