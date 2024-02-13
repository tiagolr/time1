#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "ISender.h"
#include "IControls.h"
#include "Pattern.h"

const int kNumPresets = 1;

enum EParams
{
  kPattern = 0,
  kSync,
  kRetrigger,
  kSnap,
  kGrid,
  kPaintMode,
  kPointMode,
  kNumParams
};

enum EControlTags
{
  kCtrlTagScope = 0,
  kNumCtrlTags
};

class View;
class Preferences;
class PlayButton;
class Rotary;
class PatternSwitches;
class Button;
class Caption;
class About;
class NumberControl;

using namespace iplug;
using namespace igraphics;

class TIME1 final : public Plugin
{
public:
  // prefs
  bool linkEdgePoints = false;
  int triggerChannel = 10;
  bool drawWave = true;
  bool alwaysPlaying = false;
  bool midiMode = false;
  int anoise = 1; // anti-noise/clicks 0 = no anti-noise, 1 = low, 2 - strong
  // state
  bool dirtyControls = false; // used to re-layout components during Idle(), avoids crashes when laying out components directly
  int winw = PLUG_WIDTH; // quick fix for referencing view width during process block, set by view::onResize
  bool inited = false;
  bool isPlaying = false;
  bool snap = false;
  bool midiTrigger = false; // trigger envelope via midi notes
  int gridSegs = 8;
  double syncQN = 0;
  double xpos = 0;
  double ypos = 0;
  double beatPos = 0;
  int winpos = 0;
  int lwinpos = 0;
  std::vector<sample> preSamples;
  std::vector<sample> postSamples;

  static const IColor COLOR_BG;
  static const IColor COLOR_ACTIVE;
  static const IColor COLOR_ACTIVE_DARK;
  static const IColor COLOR_ACTIVE_LIGHT;
  static const IColor COLOR_SEEK;

  Pattern* pattern;
  Pattern* patterns[12];
  View* view;
  PatternSwitches* patternSwitches;
  Caption* syncControl;
  Preferences* preferencesControl;
  Caption* pointModeControl;
  Caption* paintModeControl;
  PlayButton* playControl;
  Button* snapControl;
  NumberControl* gridNumber;
  Button* midiModeControl;
  Button* retriggerControl;
  About* aboutControl;
  ITextControl* paintLabel;
  ITextControl* pointLabel;

  IVStyle patternSwitchStyle;
  IVStyle buttonStyle;
  IVStyle numberStyle;

  ISVG settingsSVG;

  TIME1(const InstanceInfo& info);

  void OnParentWindowResize(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;
  void OnHostSelectedViewConfiguration(int width, int height) override;
  void layoutControls(IGraphics* g);
  void makeStyles();
  void makeControls(IGraphics* g);

  double getY(double x);
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnIdle() override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  bool SerializeState(IByteChunk &chunk) const override;
  int UnserializeState(const IByteChunk &chunk, int startPos) override;
  void OnRestoreState() override;
  bool canRetrigger();
  void retriggerEnvelope();
};

