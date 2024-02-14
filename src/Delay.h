#pragma once
#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace igraphics;

class Delay {
public:
  void resize(int size);
  void write(sample s);
  sample read(double delay);
  sample read3(double delay);
  int size = 0;

private:
  std::vector<sample> buf;
  int curpos = 0;
  sample curval = 0;
};