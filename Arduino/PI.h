#ifndef __PI_H__
#define __PI_H__

#ifdef ARDUINO
  #include "Arduino.h"
#endif

#define PI_SETPOINT(target, actual)   (target - actual)

class RegPI
{
  public:
    RegPI(float kP, float kI, float Ilimit);
    
    void Update(float input, int dt_ms);
    float GetOutput(void);

    void Reset(void);

    float out;

  private:
    float kP;
    float kI;
    float accum_limit;

    
    int out_i;

    float accumulator;
};


#endif
