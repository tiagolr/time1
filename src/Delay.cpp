#include "Delay.h"

void Delay::resize(int size)
{
  buf.clear();
  buf.resize(size, 0);
  this->size = buf.size();
  curpos = 0;
  curval = 0;
}

void Delay::write(sample s)
{
  buf[curpos] = s;
  curpos += 1;
  if (curpos == size)
    curpos = 0;
  //this.buf[this.curpos] = signal;
  //this.buf[this.size] = this.buf[0];
  //this.buf[this.size+1] = this.buf[1];
  //this.buf[-1] = this.buf[this.size-1];
  //this.curpos += 1;
  //this.curpos == this.size ? this.curpos = 0;
}

sample Delay::read(double delay)
{
  if (delay >= 0 && delay < size) {
    double pos = (double)curpos - delay;
    if (pos < 0) {
      pos += (double)size;
      if (pos == size) { // avoids error with floating point arithmetics
        pos = 0;
      }
    }
    curval = buf[std::floor(pos)];
  }
  return curval;
}
/*
  Interpolated delay read
*/
sample Delay::read3(double delay)
{
  if (delay >= 0 && delay < size) {
    double pos = (double)curpos - delay;
    if (pos < 0) {
      pos += (double)size;
      if (pos == size) { // avoids error with floating point arithmetics
        pos = 0;
      }
    }
    double i = std::floor(pos);
    double f = pos - i;
    sample x0, x1, x2, x3, a0, a1, a2, a3;
    x0 = i == 0 ? buf[size - 1] : buf[i - 1];
    x1 = buf[i];
    x2 = i == size - 1 ? buf[0] : buf[i + 1];
    x3 = i >= size - 2 ? buf[i - size + 2] : buf[i + 2];
    a3 = f * f; a3 -= 1.0; a3 *= (1.0 / 6.0);
    a2 = (f + 1.0) * 0.5; a0 = a2 - 1.0;
    a1 = a3 * 3.0; a2 -= a1; a0 -= a3; a1 -= f;
    a0 *= f; a1 *= f; a2 *= f; a3 *= f; a1 += 1.0;
    curval = a0 * x0 + a1 * x1 + a2 * x2 + a3 * x3;
    //i_ = floor(pos_);
    //f_ = pos_ - i_;
    //x0_ = this.buf[i_-1]; x1_ = this.buf[i_];
    //x2_ = this.buf[i_+1]; x3_ = this.buf[i_+2];
    //a3_ = f_ * f_; a3_ -= 1.0; a3_ *= (1.0 / 6.0);
    //a2_ = (f_ + 1.0) * 0.5; a0_ = a2_ - 1.0;
    //a1_ = a3_ * 3.0; a2_ -= a1_; a0_ -= a3_; a1_ -= f_;
    //a0_ *= f_; a1_ *= f_; a2_ *= f_; a3_ *= f_; a1_ += 1.0;
    //this.curval = a0_*x0_ + a1_*x1_ + a2_*x2_ + a3_*x3_;
  }
  return curval;
}