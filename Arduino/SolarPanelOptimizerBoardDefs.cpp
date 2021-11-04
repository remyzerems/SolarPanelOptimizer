#include "Arduino.h"
#include "SolarPanelOptimizerBoardDefs.h"

void setupPWM16();
void analogWrite16(uint8_t pin, uint16_t val);

void board_setup(void)
{
	// Initialize serial bus communication
	Serial.begin(9600);

	// Consider all IOs are for buttons
	pinMode(IO1, INPUT_PULLUP);
	pinMode(IO2, INPUT_PULLUP);
	pinMode(IO3, INPUT_PULLUP);
	pinMode(IO4, INPUT_PULLUP);

	// Initialize SCR LED and T2 pins (both on Timer1)
	setupPWM16();
	analogWrite16(SCR_LED, 0);
	analogWrite16(T2, 0);

	// Initiliaze T1 and T3
	pinMode(T1, OUTPUT);
	analogWrite16(T1, 0);
	pinMode(T3, OUTPUT);
	analogWrite16(T3, 0);
}

// Sets the specified PWM duty to the specified pin
// "duty" is in the binary form and has to match the correct range depending on the pin
void setPWM(int pin, unsigned int duty)
{
	switch(pin)
	{
		case SCR_LED:
		case T2:
			analogWrite16(pin, duty);
		break;

		case T1:
		case T3:
			analogWrite(pin, duty);
		break;

		default:
		break;
	}
}
void setPWM(int pin, long int duty)
{
  setPWM(pin, (unsigned int)duty);
}

// Sets the specified PWM duty to the specified pin
// "duty" is meant to be [0, 1]. If it's not, it's being limited to it.
void setPWM(int pin, float duty)
{
	if(duty > 1.0)
	{
		duty = 1.0;
	} else {
		if(duty < 0.0)
		{
			duty = 0.0;
		}
	}
	long int gain = 0;

	switch(pin)
	{
		case 9:
		case 10:
			gain = MAX_PWM_16_BITS;
		break;

		case 5:
		case 11:
			gain = MAX_PWM_8_BITS;
		break;

		default:
		break;
	}

	setPWM(pin, (long int)(duty * gain));
}


long int getPWMMaxDuty(int pin)
{
  long int dutyMax = 0;

  switch(pin)
  {
    case 9:
    case 10:
      dutyMax = MAX_PWM_16_BITS;
    break;

    default:
      dutyMax = MAX_PWM_8_BITS;
    break;
  }

  return dutyMax;
}

// Kindly got from there : https://www.arduinoslovakia.eu/blog/2017/7/16-bitove-rozlisenie-pwm-pre-arduino?lang=en

void setupPWM16() {
  DDRB  |= _BV(PB1) | _BV(PB2);       /* set pins as outputs */
  TCCR1A = _BV(COM1A1) | _BV(COM1B1)  /* non-inverting PWM */
        | _BV(WGM11);                 /* mode 14: fast PWM, TOP=ICR1 */
  TCCR1B = _BV(WGM13) | _BV(WGM12)
        | _BV(CS10);                  /* prescaler 1 */
  ICR1 = MAX_PWM_16_BITS;             /* TOP counter value (freeing OCR1A*/
}

/* 16-bit version of analogWrite(). Works only on pins 9 and 10. */
void analogWrite16(uint8_t pin, uint16_t val)
{
  switch (pin) {
    case  9: OCR1A = val; break;
    case 10: OCR1B = val; break;
  }
}
