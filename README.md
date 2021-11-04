# Solar Panel Optimizer
A project to get the most out of your solar panel of your house by intelligently controlling your boiler !

### Project context
The goal of an (electric) boiler is to transform electrical power into heat to make some hot water inside a tank.
If your home has no specific installation, the boiler switches on as soon as the water inside the tank falls down to a given temperature threshold.
If your home has a specific installation (i.e. day/night rates) this only occurs at night when the rates are lower.

Now if you have solar panels, the energy they are producing from the sun is either injected into your house to power some of your devices (oven, computer...) or either injected to the mains network (i.e. outside of your house, to your neighbourhood).
In the latter case, your electric provider will pay you a fraction (I mean less than one uh !) of what you pay for each kWh you consume.

As a result each solar panel produced kWh that does not power something in your house is not efficently used...

There comes our project !

### Project goal
The idea of the project is mainly inspired from [Barnabé Chaillot project](https://github.com/sgalvagno/Chaillot_Barnabe_Autoconsommation_photovoltaique).
He described a way to control the power that is sent to his boiler based on the power produced by the solar panels.
To be more precise, it is based on the available power at the house level.
Indeed the house electrical devices are consuming a given amount of energy that may or may not be absorbing all what the solar panels are producing.

The goal of the system is to only power the boiler on the exceeding power from the house.
The system has to carefully control the boiler power to make the house power balance null.
This way the energy produced by our solar panels is optimally used and than we only sell the exceeding energy that we do not need.

So we've been inspired by Barnabé project and decided to push it a little bit further and to have fun developping it.

### System features
The system we developped has the following features :
* Control of the electrical power injected to the boiler based on available solar panel excess power
* Measurement of the following currents:
  * House global
  * Solar panel production
  * Boiler consumption
* Hot water guard (switching the boiler ON at night if the solar panel production had not been sufficient to heat the water)
* Dedicated PCB board with some extension capacities (see board features for details)

### PCB board features
* Arduino Nano based
* Clean and user friendly design
* Inputs
  * 3x current probes
  * 1x mains signal image
  * 4x digital IOs (e.g. push button inputs)
* Outputs
  * Galvanicaly isolated variable resistor (i.e. to control a SCR that powers the boiler)
  * 3x transistor controllable loads, 0-24V, 5A (e.g. relays)
* Communication busses
  * 3x I2C connectors (LCD display, RTC and a spare connector)
* Power inputs
  * Arduino power through USB connector
  * External power input to power stuff (relays mainly)
