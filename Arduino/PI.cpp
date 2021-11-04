#ifdef ARDUINO
  #include "Arduino.h"
#endif
#include <stdio.h>
#include "PI.h"

RegPI::RegPI(float kP, float kI, float Ilimit)
{
  this->kP = kP;
  this->kI = kI;
  accum_limit = Ilimit;

  accumulator = 0;
}

void RegPI::Update(float input, int dt_ms)
{
  out = (input * kP);

  if(kI != 0.0)
  {
    out += accumulator;
  }

  accumulator += ((input * kI * (float)dt_ms) / 1000.0);
  if(accumulator >= accum_limit)
  {
    accumulator = accum_limit;
  } else {
    if(accumulator <= -accum_limit)
    {
      accumulator = -accum_limit;
    }
  }
}

void RegPI::Reset(void)
{
  accumulator = 0;
}

float RegPI::GetOutput(void)
{
  return out;
}
