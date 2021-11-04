#ifdef ARDUINO
  #include "Arduino.h"
#else
  // In standard C, the abs function is not a standard function so...
  #include <stdlib.h>
#endif

#include "ControlToolbox.h"


Limiter::Limiter(float min, float max)
{
  this->min = min;
  this->max = max;

  out = 0.0;
}

float Limiter::Update(float input)
{
  out = input;

  if(out < min)
  {
    out = min;
  } else {
    if(out > max)
    {
      out = max;
    }
  }

  return out;
}
float Limiter::GetOutput(void)
{
  return out;
}





RateLimiter::RateLimiter(float slope)
{
  this->slope = slope;
  out = 0.0;
}

void RateLimiter::Update(float input, int dt_ms)
{
  if(dt_ms > 0)
  {
    float step_req = ((input - out) * 1000.0) / (float)dt_ms;

    float step = slope;
    if(step_req < 0)
    {
      step = -step;
    }

    if(abs(step_req) < abs(step))
    {
      step = step_req;
    }

    out += (step * (float)dt_ms) / 1000.0;
  }
}

void RateLimiter::SetCurrentState(float output)
{
  out = output;
}

float RateLimiter::GetOutput(void)
{
  return out;
}
