#ifndef __CONTROL_TOOLBOX_H__
#define __CONTROL_TOOLBOX_H__

#ifdef ARDUINO
  #include "Arduino.h"
#endif

// Limiter block. Limit a signal value to a specific range
class Limiter
{
  public:
    Limiter(float min, float max);

    float Update(float input);
    float GetOutput();
  private:
    float min, max;
    float out;
};

// Rate limiting block. Limit the rate of change of a signal
class RateLimiter
{
  public:
    RateLimiter(float slope);
    
    void Update(float input, int dt_ms);
    float GetOutput(void);

    void SetCurrentState(float output);

  private:
    int slope;
    float out_i;
    float out;
};

#endif
