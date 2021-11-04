#ifndef __DEBOUNCED_BUTTON_H__
#define __DEBOUNCED_BUTTON_H__

#include <Arduino.h>


class DebouncedButton
{
  public:
    DebouncedButton(int pin);
    void Setup(int mode = INPUT_PULLUP);
    void Debounce();

    bool IsButtonDown(void);
    bool IsButtonStateChanged(void);
    bool IsButtonPressed(void);
    bool IsButtonReleased(void);

  private:
    int pin;

    int buttonStatesStab;
    int buttonPrevStatesStab;
    int buttonPrevStates;

    
};

#endif
