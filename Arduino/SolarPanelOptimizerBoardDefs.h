#ifndef __SOLAR_PANEL_OPTIMIZER_BOARD__
#define __SOLAR_PANEL_OPTIMIZER_BOARD__

// Digital IO
#define IO1			5
#define IO2			4
#define IO3			3
#define IO4			2

// PWMs
#define SCR_LED		9
#define T1			5
#define T2			10
#define T3			11

#define MAX_PWM_16_BITS		(65535L)
#define MAX_PWM_8_BITS		(255L)

// Current probes
#define CUR_SENSE1			0
#define CUR_SENSE2			1
#define CUR_SENSE3			2
#define CUR_SENSE_COUNT		3

// Mains voltage image
#define MAINS_IMAGE			3


// Functions

void board_setup(void);

long int getPWMMaxDuty(int pin);
void setPWM(int pin, unsigned int duty);
void setPWM(int pin, long int duty);
void setPWM(int pin, float duty);

#endif
