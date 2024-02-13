#include "Widgets.h"
#include <string>

void Caption::Draw(IGraphics& g)
{
  const IParam* pParam = GetParam();

  if(pParam)
  {
    pParam->GetDisplay(mStr);

    if (mShowParamLabel)
    {
      mStr.Append(" ");
      mStr.Append(pParam->GetLabel());
    }
  }

  g.DrawRoundRect(TIME1::COLOR_ACTIVE, mRECT, 2.5);

  if (mStr.GetLength() && g.GetControlInTextEntry() != this)
    g.DrawText(mText, mStr.Get(), mRECT, &mBlend);

  if(mTri.W() > 0.f)
  {
    g.FillTriangle(TIME1::COLOR_ACTIVE, mTri.L, mTri.T, mTri.R, mTri.T, mTri.MW(), mTri.B, GetMouseIsOver() ? 0 : &BLEND_50);
  }
}

void Button::DrawWidget(IGraphics& g)
{
  if (GetValue() > 0.5)
    g.FillRoundRect(TIME1::COLOR_ACTIVE, mWidgetBounds, 2.5);
  else
    g.DrawRoundRect(TIME1::COLOR_ACTIVE, mWidgetBounds, 2.5);
}

void Button::DrawValue(IGraphics& g, bool mouseOver)
{
  if(GetValue() > 0.5) {
    mStyle.valueText.mFGColor = TIME1::COLOR_BG;
    g.DrawText(mStyle.valueText, mOnText.Get(), mValueBounds, &mBlend);
  }
  else {
    mStyle.valueText.mFGColor = TIME1::COLOR_ACTIVE;
    g.DrawText(mStyle.valueText, mOffText.Get(), mValueBounds, &mBlend);
  }
}

void PatternSwitches::DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment, bool disabled)
{
  DrawPressableRectangle(g, r, pressed, mouseOver, disabled, segment == ETabSegment::Start, segment == ETabSegment::End, segment == ETabSegment::Start, segment == ETabSegment::End);
}

void PlayButton::Draw(IGraphics& g)
{
  IRECT r = mRECT.GetPadded(-2);
  if (GetValue() > 0.5) {
    g.FillRect(TIME1::COLOR_ACTIVE, r);
  }
  else {
    g.FillTriangle(TIME1::COLOR_ACTIVE, r.L, r.T, r.R, (r.T + r.B) / 2, r.L, r.B);
  }
}

void PlayButton::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  SetDirty(true);
}

void Preferences::Draw(IGraphics& g)
{
  g.DrawSVG(gate.settingsSVG, mRECT,0, &COLOR_TRANSPARENT, &TIME1::COLOR_ACTIVE);
}

void Preferences::showPopupMenu()
{
  IPopupMenu* menu = new IPopupMenu();
  IPopupMenu* optionsMenu = new IPopupMenu();
  IPopupMenu* triggerMenu = new IPopupMenu();
  IPopupMenu* loadMenu = new IPopupMenu();


  triggerMenu->AddItem("Off")->SetChecked(gate.triggerChannel == 0);
  for (int i = 0; i < 16; ++i) {
    triggerMenu->AddItem(std::to_string(i+1).c_str())->SetChecked(gate.triggerChannel == i + 1);
  }
  triggerMenu->AddItem("Omni")->SetChecked(gate.triggerChannel == 17);

  optionsMenu->AddItem("Trigger channel")->SetSubmenu(triggerMenu);
  optionsMenu->AddItem("Link edge points")->SetChecked(gate.linkEdgePoints);

  loadMenu->AddItem("Sine");
  loadMenu->AddItem("Triangle");
  loadMenu->AddItem("Random");

  menu->AddItem("Options")->SetSubmenu(optionsMenu);
  menu->AddItem("Draw wave")->SetChecked(gate.drawWave);
  menu->AddSeparator();
  menu->AddItem("Invert");
  menu->AddItem("Reverse");
  menu->AddItem("Clear");
  menu->AddItem("Copy");
  menu->AddItem("Paste");
  menu->AddSeparator();
  menu->AddItem("Load")->SetSubmenu(loadMenu);
  menu->AddItem("About");
  
  GetUI()->CreatePopupMenu(*this, *menu, GetWidgetBounds());
}

void Preferences::OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) {
  if (pSelectedMenu == nullptr)
    return;

  auto text = pSelectedMenu->GetChosenItem()->GetText();

  for (int i = 1; i < 17; ++i) {
    if (strcmp(text, std::to_string(i).c_str()) == 0) {
      gate.triggerChannel = i;
    }
  }
  if (strcmp(text, "Omni") == 0) {
    gate.triggerChannel = 17;
  }
  else if (strcmp(text, "Off") == 0) {
    gate.triggerChannel = 0;
  }
  else if (strcmp(text, "Link edge points") == 0) {
    gate.linkEdgePoints = !gate.linkEdgePoints;
  }
  else if (strcmp(text, "Draw wave") == 0) {
    gate.drawWave = !gate.drawWave;
  }
  else if (strcmp(text, "Sine") == 0) {
    gate.pattern->loadSine();
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "Triangle") == 0) {
    gate.pattern->loadTriangle();
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "Random") == 0) {
    gate.pattern->loadRandom(gate.gridSegs);
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "Copy") == 0) {
    gate.pattern->copy();
  }
  else if (strcmp(text, "Paste") == 0) {
    gate.pattern->paste();
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "Clear") == 0) {
    gate.pattern->clear();
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "Invert") == 0) {
    gate.pattern->invert();
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "Reverse") == 0) {
    gate.pattern->reverse();
    gate.pattern->buildSegments();
  }
  else if (strcmp(text, "About") == 0) {
    gate.aboutControl->Hide(false);
  }
}

void About::OnAttached()
{
  title = new ITextControl(IRECT(), PLUG_NAME, IText(25, COLOR_WHITE, "Roboto-Bold"));
  version = new ITextControl(IRECT(), PLUG_VERSION_STR, IText(16, COLOR_WHITE, "Roboto-Bold"));
  url = new IURLControl(IRECT(), "Github", PLUG_URL_STR, IText(16, COLOR_WHITE, "Roboto-Bold"));
  auto text = std::string("");
  text += PLUG_COPYRIGHT_STR;
  text += "\n\nToggle patterns 1-12 by sending midi notes on chn 10 (default)\n";
  text += "Left click - move points or set tension\n";
  text += "Right click - paint mode\n";
  text += "Double click - remove or add points\n";
  text += "Alt right click - erase mode\n";
  text += "Control - toggle snapping\n";
  text += "Mouse wheel - set grid\n";
  
  multiline = new IMultiLineTextControl(IRECT(), text.c_str(), IText(16, COLOR_WHITE, "Roboto-Bold", EAlign::Center, EVAlign::Top));
  AddChildControl(title);
  AddChildControl(version);
  AddChildControl(url);
  AddChildControl(multiline);

  layoutComponents();
}

void About::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  GetActionFunction()(this);
}

void About::Draw(IGraphics& g)
{
  g.FillRect(COLOR_BLACK.WithOpacity(0.75), mRECT);
}

void About::OnResize()
{
  if (mChildren.GetSize()) {
    layoutComponents();
  }
};

void About::layoutComponents()
{
  title->SetTargetAndDrawRECTs(mRECT.GetPadded(-100).GetFromTop(25));
  version->SetTargetAndDrawRECTs(title->GetRECT().GetVShifted(25));
  url->SetTargetAndDrawRECTs(version->GetRECT().GetVShifted(25));
  multiline->SetTargetAndDrawRECTs(IRECT(0,190,500,400).GetHAlignedTo(mRECT, EAlign::Center));
}