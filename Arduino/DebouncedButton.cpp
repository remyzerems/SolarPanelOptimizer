#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(int pin)
{
  this->pin = pin;
}

void DebouncedButton::Setup(int mode)
{
  pinMode(pin, mode);
  
  buttonPrevStates = digitalRead( pin );
  buttonStatesStab = buttonPrevStates;
  buttonPrevStatesStab = buttonPrevStates;
}

void DebouncedButton::Debounce()
{
  int currentButtonState = 0;

    // Keep track of previous stable state
    buttonPrevStatesStab = buttonStatesStab;
    
    // Read pin state
    currentButtonState = digitalRead( pin );

    // If the button is in the same state as before
    if(currentButtonState == buttonPrevStates)
    {
        // Update stabilized button state
        buttonStatesStab = currentButtonState;
    }
    // Keep track of current state for next time
    buttonPrevStates = currentButtonState;
}


bool DebouncedButton::IsButtonDown(void)
{
  return (buttonStatesStab == LOW);
}
bool DebouncedButton::IsButtonStateChanged(void)
{
  return buttonPrevStatesStab != buttonStatesStab;
}
bool DebouncedButton::IsButtonPressed(void)
{
  return IsButtonDown() && IsButtonStateChanged();
}
bool DebouncedButton::IsButtonReleased(void)
{
  return (!IsButtonDown()) && IsButtonStateChanged();
}
