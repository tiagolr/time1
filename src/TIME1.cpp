#include "TIME1.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "Test/TestSizeControl.h"
#include "View.h"
#include "Widgets.h"

const IColor TIME1::COLOR_BG = IColor::FromColorCode(0x181614);
const IColor TIME1::COLOR_ACTIVE = IColor::FromColorCode(0x00FF80);
const IColor TIME1::COLOR_ACTIVE_LIGHT = IColor::FromColorCode(0x82ffc1);
const IColor TIME1::COLOR_ACTIVE_DARK = IColor::FromColorCode(0x006633);
const IColor TIME1::COLOR_SEEK = IColor::FromColorCode(0x00FF00);

TIME1::TIME1(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets)),
  settingsSVG(nullptr)
{
  // init params
  GetParam(kPattern)->InitInt("Pattern", 1, 1, 12);
  GetParam(kSync)->InitEnum("Sync", 5, 18, "", 0, "", "Rate Hz", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "1/16t", "1/8t", "1/4t", "1/2t", "1/1t", "1/16.", "1/8.", "1/4.", "1/2.", "1/1.");
  GetParam(kPaintMode)->InitEnum("Paint", 1, 5, "", 0, "", "Erase", "Line", "Saw up", "Saw Down", "Triangle");
  GetParam(kPointMode)->InitEnum("Point", 1, 8, "", 0, "", "Hold", "Curve", "S-Curve", "Pulse", "Wave", "Triangle", "Stairs", "Smooth St");
  GetParam(kSnap)->InitBool("Snap", 0);
  GetParam(kGrid)->InitInt("Grid", 8, 2, 32);
  GetParam(kRetrigger)->InitBool("Retrigger", 0);

  preSamples.resize(PLUG_MAX_WIDTH, 0);
  postSamples.resize(PLUG_MAX_WIDTH, 0);
  makeStyles();

  // init patterns
  for (int i = 0; i < 12; i++) {
    patterns[i] = new Pattern(*this, i);
    patterns[i]->insertPoint(0, 1, 0, 1);
    patterns[i]->insertPoint(0.5, 0, 0, 1);
    patterns[i]->insertPoint(1, 1, 0, 1);
    patterns[i]->buildSegments();
  }
  pattern = patterns[0];

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };

  mLayoutFunc = [&](IGraphics* g) {
    // Layout controls on resize
    if(g->NControls()) {
      layoutControls(g);
      return;
    }

    g->EnableTooltips(true);
    g->EnableMouseOver(true);
    g->SetLayoutOnResize(true);
    g->LoadFont("Roboto-Regular", ROBOTO_FN);
    g->LoadFont("Roboto-Bold", ROBOTO_BOLD_FN);
    settingsSVG = g->LoadSVG(SETTINGS_SVG);

    makeControls(g);
    inited = true;
    layoutControls(g);
  };
}

void TIME1::makeControls(IGraphics* g)
{
  view = new View(IRECT(), *this);
  g->AttachPanelBackground(COLOR_BG);
  g->AttachControl(view);
  auto t = IText(26, COLOR_WHITE, "Roboto-Bold", EAlign::Near);
  g->AttachControl(new ITextControl(IRECT(10,14,100,35), "GATE-1", t, true));
  patternSwitches = new PatternSwitches(IRECT(), kPattern, {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"}, "", patternSwitchStyle, EVShape::EndsRounded, EDirection::Horizontal);
  patternSwitches->SetTooltip("Pattern select");
  g->AttachControl(patternSwitches);
  syncControl = new Caption(IRECT(), kSync, IText(16.f, COLOR_ACTIVE, "Roboto-Bold"), COLOR_ACTIVE_DARK);
  syncControl->SetTooltip("Tempo sync");
  g->AttachControl(syncControl);
  pointModeControl = new Caption(IRECT(), kPointMode, IText(16.f, COLOR_ACTIVE, "Roboto-Bold"), COLOR_ACTIVE_DARK);
  pointModeControl->SetTooltip("Point mode");
  g->AttachControl(pointModeControl);
  paintModeControl = new Caption(IRECT(), kPaintMode, IText(16.f, COLOR_ACTIVE, "Roboto-Bold"), COLOR_ACTIVE_DARK, true);
  paintModeControl->SetTooltip("Paint mode (RMB)");
  g->AttachControl(paintModeControl);
  playControl = new PlayButton(IRECT(), [&](IControl* pCaller){
    alwaysPlaying = !alwaysPlaying;
    playControl->SetValue(alwaysPlaying ? 1 : 0);
    playControl->SetDirty(false);
    });
  playControl->SetValue(alwaysPlaying ? 1 : 0);
  playControl->SetTooltip("Always playing on/off");
  g->AttachControl(playControl);
  midiModeControl = new Button(IRECT(), [&](IControl* pCaller) {
    midiMode = !midiMode;
    midiModeControl->SetValue(midiMode ? 1 : 0);
    midiModeControl->SetDirty(false);
    }, "", buttonStyle, "MIDI", "MIDI");
  midiModeControl->SetValue(midiMode ? 1 : 0);
  midiModeControl->SetTooltip("MIDI mode - use midi notes to trigger envelope");
  g->AttachControl(midiModeControl);
  snapControl = new Button(IRECT(), kSnap, " ", buttonStyle, "Snap", "Snap");
  g->AttachControl(snapControl);
  t = IText(16, COLOR_WHITE, "Roboto-Bold", EAlign::Near);
  gridNumber = new NumberControl(IRECT(), kGrid, nullptr, "", numberStyle, false, 8, 2, 32, "Grid %0.f", false);
  g->AttachControl(gridNumber);
  preferencesControl = new Preferences(IRECT(), [&](IControl* pCaller) {
    preferencesControl->showPopupMenu();
    }, ". . .", numberStyle, *this);
  g->AttachControl(preferencesControl);
  t = IText(16, COLOR_GRAY, "Roboto-Bold", EAlign::Near);
  paintLabel = new ITextControl(IRECT(), "Paint", t);
  pointLabel = new ITextControl(IRECT(), "Point", t);
  g->AttachControl(paintLabel);
  g->AttachControl(pointLabel);
  retriggerControl = new Button(IRECT(), [&](IControl* pCaller) {
    retriggerEnvelope();
    retriggerControl->SetValue(0);
    retriggerControl->SetDirty(false);
  }, "", buttonStyle, "R", "R");
  retriggerControl->SetTooltip("Retrigger envelope");
  g->AttachControl(retriggerControl);
  aboutControl = new About(g->GetBounds(), [](IControl* pCaller) {
    pCaller->Hide(true);
  });
  g->AttachControl(aboutControl);
  aboutControl->Hide(true);
}

void TIME1::layoutControls(IGraphics* g)
{
  if (!inited)
    return;

  const IRECT b = g->GetBounds();
  g->GetBackgroundControl()->SetTargetAndDrawRECTs(b);
  view->SetTargetAndDrawRECTs(b.GetReducedFromTop(158));

  // first row left
  int drawx = 95;
  int drawy = 10;
  syncControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy + 3, drawx+80, drawy+20+3));
  drawx += 90;
  patternSwitches->SetTargetAndDrawRECTs(IRECT(drawx, drawy+3, drawx + 250, drawy + 20+3));

  // first row right
  drawx = b.R - 35;
  preferencesControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx+25, drawy+25).GetPadded(-2.5).GetHShifted(2.5).GetVShifted(2.5));

  // third row left
  drawx = 10;
  drawy += 90;
  paintLabel->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx+40, drawy+20));
  drawx += 42;
  paintModeControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx+80, drawy+20));
  drawx += 90;
  pointLabel->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx+40, drawy+20));
  drawx += 42;
  pointModeControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx+80, drawy+20));
  drawx += 90;
  playControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx + 20 , drawy+20).GetPadded(-1));
  drawx += 30;
  retriggerControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx + 20, drawy + 20));

  // third row right
  drawx = b.R - 70;
  midiModeControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx + 60, drawy + 20));
  drawx -= 70;
  snapControl->SetTargetAndDrawRECTs(IRECT(drawx, drawy, drawx+60, drawy+20));
  drawx -= 85;
  gridNumber->SetTargetAndDrawRECTs(IRECT(drawx+30, drawy, drawx+40+40, drawy+20));

  aboutControl->SetTargetAndDrawRECTs(b);
}

void TIME1::makeStyles()
{
  //Create styles
  patternSwitchStyle = (new IVStyle())->WithColors({
    COLOR_TRANSPARENT,
    COLOR_ACTIVE_DARK,
    COLOR_ACTIVE,
    COLOR_BG,
    COLOR_TRANSPARENT,
    COLOR_TRANSPARENT,
    });
  patternSwitchStyle.roundness = 0.25;
  patternSwitchStyle.drawShadows = false;
  patternSwitchStyle.drawFrame = false;
  patternSwitchStyle.valueText.mFGColor = COLOR_BG;
  patternSwitchStyle.valueText = patternSwitchStyle.valueText.WithFont("Roboto-Bold");

  buttonStyle = (new IVStyle())->WithColors({
    COLOR_TRANSPARENT,
    COLOR_ACTIVE_DARK,
    COLOR_ACTIVE,
    COLOR_BG,
    COLOR_TRANSPARENT,
    COLOR_BG,
    });
  buttonStyle.roundness = 0.25;
  buttonStyle.drawShadows = false;
  buttonStyle.drawFrame = false;
  buttonStyle.showLabel = false;
  buttonStyle.valueText.mFGColor = COLOR_BG;
  buttonStyle.valueText.mSize = 16;
  buttonStyle.valueText = buttonStyle.valueText.WithFont("Roboto-Bold");

  numberStyle = (new IVStyle())->WithColors({
    COLOR_TRANSPARENT,
    COLOR_BG,
    COLOR_BG,
    COLOR_TRANSPARENT,
    COLOR_TRANSPARENT,
    });
  numberStyle.shadowOffset = 0;
  numberStyle.roundness = 0.5;
  numberStyle.valueText.mFGColor = COLOR_ACTIVE;
  numberStyle.valueText.mSize = 16;
  numberStyle.valueText = numberStyle.valueText.WithFont("Roboto-Bold");
  numberStyle.labelText.mFGColor = COLOR_ACTIVE;
  numberStyle.labelText.mSize = 16;
  numberStyle.labelText = numberStyle.labelText.WithFont("Roboto-Bold");
}

void TIME1::OnParamChange(int paramIdx)
{
  if (paramIdx == kPattern) {
    pattern = patterns[(int)GetParam(kPattern)->Value() - 1];
  }
  else if (paramIdx == kSync) {
    dirtyControls = true;
    auto sync = GetParam(kSync)->Value();
    if (sync == 0) syncQN = 0;
    if (sync == 1) syncQN = 1./4.; // 1/16
    if (sync == 2) syncQN = 1./2.; // 1/8
    if (sync == 3) syncQN = 1/1; // 1/4
    if (sync == 4) syncQN = 1*2; // 1/2
    if (sync == 5) syncQN = 1*4; // 1bar
    if (sync == 6) syncQN = 1*8; // 2bar
    if (sync == 7) syncQN = 1*16; // 4bar
    if (sync == 8) syncQN = 1./6.; // 1/16t
    if (sync == 9) syncQN = 1./3.; // 1/8t
    if (sync == 10) syncQN = 2./3.; // 1/4t
    if (sync == 11) syncQN = 4./3.; // 1/2t
    if (sync == 12) syncQN = 8./3.; // 1/1t
    if (sync == 13) syncQN = 1./4.*1.5; // 1/16.
    if (sync == 14) syncQN = 1./2.*1.5; // 1/8.
    if (sync == 15) syncQN = 1./1.*1.5; // 1/4.
    if (sync == 16) syncQN = 2./1.*1.5; // 1/2.
    if (sync == 17) syncQN = 4./1.*1.5; // 1/1.
  }
  else if (paramIdx == kGrid) {
    gridSegs = (int)GetParam(kGrid)->Value();
  }
  else if (paramIdx == kRetrigger && GetParam(kRetrigger)->Value() == 1) {
    midiTrigger = true;
  }
  else if (paramIdx == kRetrigger && GetParam(kRetrigger)->Value() == 1 && canRetrigger()) {
    retriggerEnvelope();
  }
}

void TIME1::OnParentWindowResize(int width, int height)
{
  if (GetUI())
    GetUI()->Resize(width, height, 1.f, false);
}

bool TIME1::OnHostRequestingSupportedViewConfiguration(int width, int height)
{
  return ConstrainEditorResize(width, height);
}

void TIME1::OnHostSelectedViewConfiguration(int width, int height)
{
  if (GetUI())
    GetUI()->Resize(width, height, 1.f, true);
}

bool TIME1::canRetrigger()
{
  bool isSync = GetParam(kSync)->Value() > 0;
  return !midiMode && (!isSync && (alwaysPlaying || isPlaying)) || (alwaysPlaying && !isPlaying);
}

void TIME1::retriggerEnvelope()
{
  beatPos = 0;
}

double inline TIME1::getY(double x)
{
  return 1 - pattern->get_y_at(x);
}

void TIME1::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
  const double srate = GetSampleRate();
  const double tempo = GetTempo();
  const double beatsPerSpl = tempo / (60. * srate);
  isPlaying = GetTransportIsRunning();
  const double sync = GetParam(kSync)->Value();
  // update beatpos only during playback
  if (!midiMode && isPlaying) {
    beatPos = GetPPQPos();
  }

  auto processDisplaySamples = [&](int s) {
    winpos = std::floor(xpos * winw);
    if (lwinpos != winpos) {
      preSamples[winpos] = 0;
      postSamples[winpos] = 0;
    }
    lwinpos = winpos;
    double avgPreSample = 0;
    double avgPostSample = 0;
    for (int c = 0; c < nChans; ++c) {
      avgPreSample += inputs[c][s];
      avgPostSample += outputs[c][s];
    }
    avgPreSample = std::abs(avgPreSample / nChans);
    avgPostSample = std::abs(avgPostSample / nChans);
    if (preSamples[winpos] < avgPreSample)
      preSamples[winpos] = avgPreSample;
    if (postSamples[winpos] < avgPostSample)
      postSamples[winpos] = avgPostSample;
  };

  if (!midiMode && (isPlaying || alwaysPlaying)) {
    for (int s = 0; s < nFrames; ++s) {
      beatPos += beatsPerSpl;
      xpos = beatPos / syncQN;
      xpos -= std::floor(xpos);

      ypos = getY(xpos);

      for (int c = 0; c < nChans; ++c) {
        outputs[c][s] = inputs[c][s] * ypos;
      }

      processDisplaySamples(s);
    }
  }

  else if (midiMode && (alwaysPlaying || midiTrigger)) {
    if (alwaysPlaying && midiTrigger) { // reset phase on midiTrigger
      xpos = 0;
      midiTrigger = false;
    }
    for (int s = 0; s < nFrames; ++s) {
      xpos += beatsPerSpl / syncQN;
      if (!alwaysPlaying && xpos >= 1) {
        midiTrigger = false;
        xpos = 1;
      }
      else {
        xpos -= std::floor(xpos);
      }
      ypos = getY(xpos);
      for (int c = 0; c < nChans; ++c) {
        outputs[c][s] = inputs[c][s] * ypos;
      }
      processDisplaySamples(s);
    }
  }

  // keep processing the same position if stopped in MIDI mode
  else if (midiMode && !alwaysPlaying && !midiTrigger) {
    for (int s = 0; s < nFrames; ++s) {
      ypos = getY(xpos);
      for (int c = 0; c < nChans; ++c) {
        outputs[c][s] = inputs[c][s] * ypos;
      }
    }
  }
}

void TIME1::ProcessMidiMsg(const IMidiMsg& msg)
{
  int status = msg.StatusMsg();
  int channel = msg.Channel();
  int vel = msg.Velocity();

  if (status == IMidiMsg::kNoteOn && vel > 0 && (channel == triggerChannel - 1 || triggerChannel == 17)) {
    GetParam(kPattern)->Set(msg.NoteNumber() % 12 + 1);
    SendCurrentParamValuesFromDelegate();
    DirtyParametersFromUI();
  }
  else if (midiMode && status == IMidiMsg::kNoteOn && vel > 0) {
    midiTrigger = true;
    xpos = 0;
  }
  SendMidiMsg(msg); // passthrough
}

void TIME1::OnReset()
{
  preSamples.clear();
  preSamples.resize(PLUG_MAX_WIDTH, 0);
  postSamples.clear();
  postSamples.resize(PLUG_MAX_WIDTH, 0);
}

void TIME1::OnIdle()
{
  auto g = GetUI();
  if (dirtyControls && g) {
    dirtyControls = false;
    layoutControls(g);
    g->SetAllControlsDirty(); // FIX - erases extra drawn knob
  }
  if (g && canRetrigger() != !retriggerControl->IsHidden()) {
    retriggerControl->Hide(!canRetrigger());
  }
}

bool TIME1::SerializeState(IByteChunk &chunk) const
{
  chunk.Put(&linkEdgePoints);
  chunk.Put(&triggerChannel);
  chunk.Put(&drawWave);
  chunk.Put(&alwaysPlaying);
  chunk.Put(&midiMode);

  // reserved space
  int cSize = chunk.Size();
  char zero[1];
  memset(zero, 0, 1);
  for (int i = 0; i < 1024 - cSize; ++i)
    chunk.PutBytes(zero, 1);

  // serialize patterns
  for (int i = 0; i < 12; ++i) {
    auto pattern = patterns[i];
    int size = pattern->points.size();
    chunk.Put(&size);
    for (int j = 0; j < size; ++j) {
      auto point = pattern->points[j];
      chunk.Put(&point);
    }
  }

  return SerializeParams(chunk);
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int TIME1::UnserializeState(const IByteChunk &chunk, int startPos)
{
  startPos = chunk.Get(&linkEdgePoints, startPos);
  startPos = chunk.Get(&triggerChannel, startPos);
  startPos = chunk.Get(&drawWave, startPos);
  startPos = chunk.Get(&alwaysPlaying, startPos);
  startPos = chunk.Get(&midiMode, startPos);

  // reserved space
  char buffer[1];
  int size = 1024 - startPos;
  for (int i = 0; i < size; ++i)
    startPos = chunk.GetBytes(buffer, 1, startPos);

  for (int i = 0; i < 12; ++i) {
    auto pattern = patterns[i];
    pattern->points.clear();
    int size;
    startPos = chunk.Get(&size, startPos);
    for (int j = 0; j < size; ++j) {
      Point point;
      startPos = chunk.Get(&point, startPos);
      pattern->points.push_back(point);
    }
    pattern->buildSegments();
  }

  return UnserializeParams(chunk, startPos);
}

void TIME1::OnRestoreState() {
  SendCurrentParamValuesFromDelegate();
  dirtyControls = true;
};