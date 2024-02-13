#pragma once
#include "TIME1.h"
#include "IControls.h"

class NumberControl : public IVNumberBoxControl
{
public:
  NumberControl(const IRECT& bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool buttons = false, double defaultValue = 50.f, double minValue = 1.f, double maxValue = 100.f, const char* fmtStr = "%0.0f", bool drawTriangle = true)
    :IVNumberBoxControl(bounds, paramIdx, actionFunc, label, style, buttons, defaultValue, minValue, maxValue, fmtStr, drawTriangle)
  {
    mLargeIncrement = 0.1f;
  };
};

class Caption : public ICaptionControl
{
public:
  Caption(const IRECT& bounds, int paramIdx, const IText& text, const IColor& bgColor, bool showParamLabel = false)
    : ICaptionControl(bounds, paramIdx, text, bgColor, showParamLabel) {};

  void Draw(IGraphics& g) override;
};

class Button : public IVToggleControl
{
public:
  Button(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, const char* offText, const char* onText)
    : IVToggleControl(bounds, paramIdx, label, style, offText, onText) {};

  Button(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, const char* offText, const char* onText)
    : IVToggleControl(bounds, aF, label, style, offText, onText) {};

  void DrawWidget(IGraphics& g) override;
  void DrawValue(IGraphics& g, bool mouseOver) override;
};

class PatternSwitches : public IVTabSwitchControl
{
public:
  PatternSwitches(const IRECT& bounds, int paramIdx, const std::vector<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction)
    : IVTabSwitchControl(bounds, paramIdx, options, label, style, shape, direction) {};

  void DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment, bool disabled) override;
};

class PlayButton : public IControl
{
public:
  PlayButton(const IRECT& bounds, IActionFunction af) : IControl(bounds, af) {};
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void Draw(IGraphics& g) override;
};

class Preferences : public IVButtonControl
{
public:
  Preferences(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, TIME1& g)
    : IVButtonControl(bounds, aF, label, style), gate(g) {};

  void Draw(IGraphics& g) override;
  void showPopupMenu();
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override;

private:
  TIME1& gate;
};

class About : public IContainerBase
{
public:
  About(const IRECT& bounds, IActionFunction af) : IContainerBase(bounds, af) {};

  ITextControl* title;
  ITextControl* version;
  IURLControl* url;
  IMultiLineTextControl* multiline;

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnAttached() override;
  void Draw(IGraphics& g) override;
  void OnResize() override;
  void layoutComponents();
};