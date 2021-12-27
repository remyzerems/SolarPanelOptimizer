

// If defined, outputs some more debug data onto the Serial Console
//#define SERIAL_DEBUG

#include <EmonLib.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include "RTClib.h"
#include "SunriseSunset.h"

// Useful definitions of the Solar Panel Optimizer PCB
#include "SolarPanelOptimizerBoardDefs.h"

#include "DebouncedButton.h"
#include "PI.h"
#include "ControlToolbox.h"

/* PROBES ALIAS */
#define PROBE_BOILER    CUR_SENSE1
#define PROBE_MAIN      CUR_SENSE2
#define PROBE_PANELS    CUR_SENSE3
/**********/

#define LCD_DEFAULT_ADDRESS    0x27
#define LCD_CHARS_PER_LINE    16
#define LCD_LINE_COUNT      2
LiquidCrystal_I2C lcd(LCD_DEFAULT_ADDRESS, LCD_CHARS_PER_LINE, LCD_LINE_COUNT); // LCD initialization

/* Location data for sunset/sunrise determination. Customization : change it to your own location */
#define LOCAL_LATTITUDE     43.836699
#define LOCAL_LONGITUDE     4.360054
#define LOCAL_TIMEZONE      GetTimezoneFranceFixe
/**********/

/* DEBUG stuff */
int dayMins, dayHours, dayD, dayM;
DateTime LastForcedHeatDate;
/**************/

/* RTC */
RTC_PCF8523 rtc;
bool rtcInitialized = false;
DateTime now; // Date/time of the day (updated frequently)
SunriseSunsetDates sunrise_sunset_dates;  // Sunset/sunrise times
#define DATE_REFRESH_RATE_S   60  // Date/time refresh rate in seconds
void UpdateDateTime();
/**********/

// Energy monitors (i.e. current probes)
EnergyMonitor emons[CUR_SENSE_COUNT];
#define emonMain     emons[0]
#define emonBoiler   emons[1]
#define emonPanels   emons[2]
void ManageEmons();

int emonAvg[CUR_SENSE_COUNT];
int emonAvgTmp[CUR_SENSE_COUNT];
int AvgCount = 0;

// Buttons
#define MAX_DEBOUNCED_BUTTONS   10
#define BUTTON_DEBOUNCE_TIME_MS    5
DebouncedButton button1(IO1);
DebouncedButton button2(IO2);
DebouncedButton button3(IO3);
DebouncedButton button4(IO4);
void ManageButtonsDebouncing();

// Operating modes
enum Modes { MODES_IDLE, MODES_DAY_HEAT, MODES_STARTING_FORCED_HEAT, MODES_TESTING_BOILER, MODES_FORCE_HEAT, MODES_OFF };
enum Modes currentMode = MODES_IDLE;
Modes ManageModes(Modes modeToManage);
String CurrentModeStr = "";

// Home hardware specific functions 
bool IsBoilerON();
#define MAX_BOILER_POWER          (4700.0)  // Customization : set your boiler max power rating in Watts
#define BOILER_TESTING_DELAY_S    (10)      // Time in seconds to wait before testing if the boiler is consuming power or not

// Control stuff
unsigned long int dT = 0;
unsigned long int prevDTmillis = 0;

float TargetPower = 0.0;
float BoilerPowerCmd = 0.0;

#define MainPower     emonMain.realPower
#define BoilerPower   emonBoiler.realPower
#define PanelPower    emonPanels.realPower

unsigned int led_intensity = 0;
#define MAX_LED_INTENSITY (getPWMMaxDuty(SCR_LED))

//              MAX_LED_INTENSITY / 3.0 => from 0 to max led intensity in 3s
RateLimiter on_off_rate_lim((float)MAX_LED_INTENSITY / 3.0); // Rate limiter for smoothing ON=>OFF and OFF=>ON transients

Limiter lim_BoilerPowerCmd(0, MAX_BOILER_POWER);     // Limiter for the boiler power command (avoid sudden changes in the command)
// PID controller to manage boiler power control
RegPI pi(0.04, 0.000, 50);  // Notice : no Integral term


// Application parameters
typedef struct
{
  float CurrentProbeOffsets[CUR_SENSE_COUNT];
}AppParameters;

// Display menu definitions.
enum Menus { MENU_OFF, MENU_1, MENU_2, MENU_3, MENU_4, MENU_PARAMETERS, MENU_FORCE_HEAT, MAX_MENU };
enum Menus currentMenu = MENU_OFF;
enum Menus ManageMenus( enum Menus menuToDisplay );
#define MENUS_REFRESH_TIME_S  2   // Force refreshing the LCD every ... seconds
#define LCD_LINE_EX(line_str, line_size, string, ...) snprintf(line_str, line_size, string, ## __VA_ARGS__)  // Helper function for LCD text with extended parameters
#define LCD_LINE(line_str, string, ...) LCD_LINE_EX(line_str, sizeof(line_str), string, ## __VA_ARGS__)  // Simplified helper function for LCD text


typedef enum
{
  DVT_FLOAT,
  DVT_DATE_TIME
}DisplayableVariableType;

typedef struct
{
  char VarName[12];
  void* VarPtr;
  char Unit[4];
  DisplayableVariableType Type;
}DisplayableVariable;
#define NEW_DISP_VAR_DATE(text_name, variable)  {text_name, (void*)&variable, "", DVT_DATE_TIME}
#define NEW_DISP_VAR(text_name, variable, unit)  {text_name, (void*)&variable, unit, DVT_FLOAT}

void ComputeLCDText(char* line_str_out, DisplayableVariable displayableVar)
{
  switch(displayableVar.Type)
  {
    case DVT_DATE_TIME:
      LCD_LINE_EX(line_str_out, LCD_CHARS_PER_LINE, "%s=%02d:%02d", displayableVar.VarName, (int)(((DateTime*)displayableVar.VarPtr)->month()), (int)(((DateTime*)displayableVar.VarPtr)->day()));
    break;
    default:
      LCD_LINE_EX(line_str_out, LCD_CHARS_PER_LINE, "%s=%d%s", displayableVar.VarName, (int)(*((float*)displayableVar.VarPtr)), displayableVar.Unit);
    break;
  }
}

// Beware. The DisplayableVariable variables must be placed after the variables you want to display so that they are declared when initializing the menu variable

// Creating a table for variables to display on menu 1
DisplayableVariable Menu1_Vars[] = {
  NEW_DISP_VAR("MainPow", MainPower, "W"),
  NEW_DISP_VAR("HeatPow", BoilerPower, "W"),
  NEW_DISP_VAR("PanelPow", PanelPower, "W"),
  NEW_DISP_VAR("BoilerCmd", BoilerPowerCmd, "W"),
  NEW_DISP_VAR("Led", led_intensity, ""),
  NEW_DISP_VAR_DATE("LastFHeat", LastForcedHeatDate)
};

// Creating a table for variables to display on menu "parameters"
DisplayableVariable MenuParam_Vars[] = {
  NEW_DISP_VAR("Offs. main", emonAvg[0], "W"),
  NEW_DISP_VAR("Offs. heat", emonAvg[1], "W"),
  NEW_DISP_VAR("Offs. panl", emonAvg[2], "W"),
};

// Attention : Utiliser cette fonction en tant LOCAL_TIMEZONE ne fonctionne que si on met la RTC à l'heure aux changements d'heure été/hiver
// Defining the France timezone (i.e. when do we change from summer time to winter time)
// Important note : if you set LOCAL_TIMEZONE to use this function, it will only work if you change the RTC time according to the summer/winter time.
//                  otherwise, you'll get a one hour offset (maybe that's not so important for our application though...
unsigned int GetTimezoneFrance(DateTime d)
{
  unsigned int timezone = 1;
  
  if( d.month() > 3 || (d.month() == 3 && d.day() >= 28) )
  {
    if( d.month() < 10 || (d.month() == 10 && d.day() < 31) )
    {
      timezone = 2;
    }
  }

  return timezone;
}

// This function gets the current time zone at compile time. This has some side effects but may not be a too big issue for our application...
unsigned int GetTimezoneFranceFixe(DateTime d)
{
  DateTime compile_date(F(__DATE__), F(__TIME__));
  unsigned int timezone = GetTimezoneFrance(compile_date);

  return timezone;
}

// Sunrise/sunset variables for displaying
int riseSecs, SunSecs;
int riseMins, SunMins;
int riseHs, SunHs;
String riseTime;
String SunTime;


/*************************************************************************************/
/*           A R D U I N O   S E T U P    F U N C T I O N                            */
/*************************************************************************************/
void setup()
{
  lcd.init(); // LCD initialization

  // Solar Panel Optimizer PCB initialization
  board_setup();

  // Current probes initialization
  emonMain.current(PROBE_MAIN, 29.7);
  emonMain.voltage(MAINS_IMAGE, 218, 1.7);

  emonBoiler.current(PROBE_BOILER, 29.7);
  emonBoiler.voltage(MAINS_IMAGE, 218, 1.7);

  emonPanels.current(PROBE_PANELS, 29.5);
  emonPanels.voltage(MAINS_IMAGE, 218, 1.7);

  // Buttons initialization
  button1.Setup();
  button2.Setup();
  button3.Setup();
  button4.Setup();

  // Average initialization
  for(int i = 0; i < CUR_SENSE_COUNT;i++)
  {
     emonAvg[i] = 0;
     emonAvgTmp[i] = 0;
  }

  // RTC initialization
  if ( !rtc.begin() )
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  } else {
    if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    rtc.start();

    now = rtc.now();

    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" (");
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();

    rtcInitialized = true;
  }

  currentMenu = MENU_1; // Starting with MENU_1

  #ifdef SERIAL_DEBUG
    Serial.println("MainPower;PanelPower;BoilerPower;intensite led");
  #endif
}

/*************************************************************************************/
/*             A R D U I N O   L O O P    F U N C T I O N                            */
/*************************************************************************************/
void loop()
{
 
  // Calculate time base (elapsed time from the previous time)
  unsigned long int curMillis = millis();
  dT = curMillis - prevDTmillis;
  prevDTmillis = curMillis;
  
  // Update probe sensor measurements
  ManageEmons();

  // Serial output debug text
  #ifdef SERIAL_DEBUG
    Serial.print(MainPower);
    Serial.print(";");
    Serial.print(PanelPower);
    Serial.print(";");
    Serial.print(BoilerPower);
    Serial.print(";");
  #endif
  
  // Manage modes of operation
  currentMode = ManageModes(currentMode);

  // If button 1 is pressed,
  if(button1.IsButtonReleased())
  {
    currentMenu = (enum Menus)(currentMenu + 1);  // Move to next menu
  }


  // Getting probe average (could be used for probe offset calibration or debug...)
  for(int i = 0;i < CUR_SENSE_COUNT;i++)
  { 
    emonAvgTmp[i] += emons[i].realPower;
  }
  AvgCount++;
  if(AvgCount >= 20)
  {
    for(int i = 0;i < CUR_SENSE_COUNT;i++)
    {
      emonAvg[i] = emonAvgTmp[i]/AvgCount;
      emonAvgTmp[i] = 0;
    }
    AvgCount = 0;
  }

  // Manage menus (i.e. what's displayed on the screen)
  currentMenu = ManageMenus(currentMenu);

  // Manage buttons
  ManageButtonsDebouncing();

  // Update date and time from RTC
  UpdateDateTime();

  // Finish printing debug log data
  #ifdef SERIAL_DEBUG
    Serial.println(led_intensity);
  #endif

  // Setting the led intensity (i.e. resulting from the control loop)
  setPWM(SCR_LED, led_intensity);
}

// Function to manage operating modes (day mode, night mode and so on)
Modes ManageModes(Modes modeToManage)
{
  Modes newMode = modeToManage;

  switch(modeToManage)
  {
    case MODES_DAY_HEAT :
    // This Day heat mode is where we drive the boiler power so that it is using the panel power that would be sent outside to the network otherwise

    CurrentModeStr = "DAY_HEAT";

    // Calculating the amount of power to reach the TargetPower (the target) from the current MainPower value (power consumed by our house) and feeding this to our PID
    pi.Update(PI_SETPOINT(TargetPower, MainPower), dT);

    // Getting the output of the previous step (the PID) and adding it to the current boiler power setpoint
    BoilerPowerCmd += pi.GetOutput();

    // Limiting the new setpoint to the [0 MAX_BOILER_POWER] range (yeh... unfortunately, our boiler is not producing power !! and we cannot over power it)
    BoilerPowerCmd = lim_BoilerPowerCmd.Update(BoilerPowerCmd);

    // Applying the linear scaling to transform boiler power (Watts) into led intensity
    led_intensity = (unsigned int)( (BoilerPowerCmd / MAX_BOILER_POWER) * (float)MAX_LED_INTENSITY );

    break;
    case MODES_STARTING_FORCED_HEAT :
    // This Starting forced heat mode is a state in which we're just passing once to initialize stuff before making a forced heat (mostly when leaving day mode and entering night mode)

    CurrentModeStr = "START_F_HEAT";

    // Resetting the boiler power setpoint to 0 for next time (mostly for the next day)
    BoilerPowerCmd = 0.0;

    // Setting the rate limiter to the current setpoint (i.e. led_intensity) so that the rate limiter is in sync with the current setpoint (i.e. avoid discontinuity in the setpoint)
    on_off_rate_lim.SetCurrentState(led_intensity);

    // Moving on to the next step
    newMode = MODES_TESTING_BOILER;
    break;
    case MODES_TESTING_BOILER :
    // The goal of this mode is to test if the boiler is hot enough (i.e. had enough time in the day to heat to its standard temperature)

    static unsigned long int heatTimeDelay = 0;

    CurrentModeStr = "TEST_BOILER";

    // Progressively powering the boiler using the rate limiter to gently increase the setpoint
    on_off_rate_lim.Update((float)MAX_LED_INTENSITY, dT);
    led_intensity = on_off_rate_lim.GetOutput();

    // Waiting for the setpoint to reach a rather high value (i.e. nearly 100% ON)
    if(led_intensity >= (MAX_LED_INTENSITY * 0.9))
    {
      // Wait for a given delay in order to let the power measurements to settle
      if(heatTimeDelay < ((unsigned long)BOILER_TESTING_DELAY_S * 1000UL))
      {
        heatTimeDelay += dT;
      } else {
        // When the delay has been executed, check if the boiler is consuming power...
        if ( IsBoilerON() )
        {
          // It means it did not reach the desired temperature so we need to power it during the night for heating up
          newMode = MODES_FORCE_HEAT;
          heatTimeDelay = 0;
        }
        else 
        {
          // Otherwise we had enough sun during the day to get the required temperature in the boiler, so switching everything off for the whole night !
          newMode = MODES_OFF;
          heatTimeDelay = 0;
        }
      }
    }
    break;
    case MODES_FORCE_HEAT :
    // This Force heat mode is to force the boiler to heat (mostly during the night because it did not have enough time/power to heat up to the required temperature, but can also be activated manually)

    CurrentModeStr = "FORCE_HEAT";

    // Switching the boiler ON to max power (still using the rate limiter to genlty switch it ON)
    on_off_rate_lim.Update(MAX_LED_INTENSITY, dT);
    led_intensity = on_off_rate_lim.GetOutput();

    // Smart mode : once the boiler is no longer consuming power it means it's hot enough, so...
    if ( BoilerPower < MAX_BOILER_POWER / 3.0 )
    {
      // ...let's switch it OFF, and go to sleep
      newMode = MODES_OFF;

      // Remember when we had to do a forced heat (for display purpose)
      LastForcedHeatDate = now;
    }
    break;
    case MODES_OFF :
    // This Off mode is a mode in which we switch the boiler off (mostly when at night)

    CurrentModeStr = "OFF";

    // Switch the boiler OFF using the rate limiter to switch it off progressively
    on_off_rate_lim.Update(0, dT);
    led_intensity = on_off_rate_lim.GetOutput();
    break;
    default :
    // In IDLE mode and all unknown modes, switch the power off
    on_off_rate_lim.Update(0, dT);
    led_intensity = on_off_rate_lim.GetOutput();
    break;
  }

  return newMode;
}

// This function is to control what are the possible actions and information displayed in the menus
enum Menus ManageMenus( enum Menus menuToDisplay )
{
    static Menus prevMenu = MAX_MENU;
    static unsigned long int lastDisplayTime = millis();
    bool switchOffLCD = false;
    char line1[20];
    char line2[20];

    static int curVars = 0;
    
    unsigned long int curMillis = millis();

    bool buttonChanged = button1.IsButtonStateChanged() || button2.IsButtonStateChanged() || button3.IsButtonStateChanged() || button4.IsButtonStateChanged();

    // Should we refresh the screen ?
    if(menuToDisplay != prevMenu || (lastDisplayTime + (MENUS_REFRESH_TIME_S * 1000) <= curMillis) || buttonChanged )
    {
        // Clear LCD
        lcd.clear();
        
        // Depending on the menu to display...
        switch(menuToDisplay)
        {
            case MENU_1 :
        // In this menu we successively display the variables of the Menu1_Vars
        // They will scroll so that we can see them all
        
        // If reaching the end of the list, restart to the beginning
        if((curVars + 1) >= sizeof(Menu1_Vars) / sizeof(Menu1_Vars[0]))
        {
          curVars = 0;
        }

        // Define what's displayed in the two LCD lines
        ComputeLCDText(line1, Menu1_Vars[curVars]);
        ComputeLCDText(line2, Menu1_Vars[curVars+1]);
        
        // Move on to the next variables to display for the next refresh event
        curVars++;
            break;
            case MENU_2 :
        // Display the current mode
        LCD_LINE(line1, "CurrentModeStr ");
        LCD_LINE(line2, "%s", CurrentModeStr.c_str());
            break;
            case MENU_3 :
        // Display sunrise and sunset times
        riseSecs = sunrise_sunset_dates.SunriseDate.second();
        riseMins = sunrise_sunset_dates.SunriseDate.minute();
        riseHs = sunrise_sunset_dates.SunriseDate.hour();
        LCD_LINE(line1, "sunrise:%02d:%02d", riseHs, riseMins);

        SunSecs = sunrise_sunset_dates.SunsetDate.second();
        SunMins = sunrise_sunset_dates.SunsetDate.minute();
        SunHs = sunrise_sunset_dates.SunsetDate.hour();
        SunTime = SunHs + ":"+ SunMins;
        LCD_LINE(line2, "sunset:%02d:%02d", SunHs, SunMins );

            break;

            case MENU_4 :
            LCD_LINE(line1, "MENU4");
            LCD_LINE(line2, "Nothing here...");
        //LCD_LINE(line1, "passé par  ");
        //LCD_LINE(line2, "%s", ModeAff.c_str());  
        //dayMins = now.minute();
        //dayHours = now.hour();
        //dayD = now.day();
        //dayM = now.month();
        
        /*LCD_LINE(line1, "end night heat on");
        LCD_LINE(line2, "%u/%u  %u:%u", dayD, dayM,dayHours,dayMins );*/
            break;
            
                
      case MENU_PARAMETERS :

        // If reaching the end of the list, restart to the beginning
        if(curVars >= sizeof(MenuParam_Vars) / sizeof(MenuParam_Vars[0]))
        {
          curVars = 0;
        }

        // Displaying the probes average
        LCD_LINE(line1, "Parameters");
        ComputeLCDText(line2, MenuParam_Vars[curVars]);

        // Move on to the next variables to display for the next refresh event
        curVars++;
      break;

      case MENU_FORCE_HEAT:
        // This menu is to allow the user to manually force the boiler to heat
        static unsigned long int buttonPressedTime = 0;

        // If coming here from another menu...
        if(menuToDisplay != prevMenu)
        {
          // Reset some variables
          buttonPressedTime = 0;
        }
      
        LCD_LINE(line1, "Force heating ?");

        if(button1.IsButtonPressed())
        {
          buttonPressedTime = curMillis;
        }

        if(buttonPressedTime == 0)
        {
          LCD_LINE(line2, "Long press if OK");
        } else {
          if((curMillis - buttonPressedTime) < 5000U)
          {
            LCD_LINE(line2, "Pressed...");
          } else {
            LCD_LINE(line2, "Going to forced");
            currentMode = MODES_STARTING_FORCED_HEAT;
          }
        }
      
      break;
            // For all other menus...
            default :
        // Switch of the LCD
        switchOffLCD = true;
        menuToDisplay = MENU_OFF;

        // Resetting the probes average
        for(int i = 0;i < CUR_SENSE_COUNT;i++)
        {
          emonAvg[i] = 0;
          emonAvgTmp[i] = 0;
        }
        AvgCount = 0;
        break;
    }

    if(switchOffLCD == true)
    {
      lcd.noBacklight(); // Switch the LCD backlight OFF
    } else {
      // Finally print the computed text to the LCD
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(line1);
      lcd.setCursor(0, 1);
      lcd.print(line2);
        }

      lastDisplayTime = curMillis;
    }
    
  // Keep track of the previous menu
    prevMenu = menuToDisplay;

  return menuToDisplay;
}

// Helper function that determines if the boiler is consuming power
bool IsBoilerON()
{
  bool res = false;
  // Assuming that a power consumption of more than 15W means boiler is ON (comparison to -15W is to deal with the probe mounting direction that would lead to negative readings)
  if(emonBoiler.realPower > 15.0 || emonBoiler.realPower < -15.0)
  {
    res = true;
  }
  return res;
}

// Manage current probes
void ManageEmons()
{
  // Updating the probe measurements
  emonMain.calcVI(20,2000);
  emonBoiler.calcVI(20,2000);
  emonPanels.calcVI(20,2000);
}

// Managing the buttons and the debouncing function
void ManageButtonsDebouncing()
{
  static unsigned long int elapsedTime = 0;
  
  if ( elapsedTime >= BUTTON_DEBOUNCE_TIME_MS  ) 
    {
        button1.Debounce();
        button2.Debounce();
        button3.Debounce();
        button4.Debounce();

    elapsedTime = 0;
  }

  elapsedTime += dT;
}

// Update date and time thanks to the RTC
void UpdateDateTime()
{
  static unsigned long int elapsedTime = 0;
  static bool firstPass = true;

  // Should we update date and time ?
  if ( elapsedTime >= ((unsigned long)DATE_REFRESH_RATE_S * 1000UL) || firstPass  ) 
  {
    if(rtcInitialized == true)
    {
      DateTime prevDate = DateTime(now);
      now = rtc.now();
      
      #ifdef SERIAL_DEBUG
      Serial.println("Updating time");
      #endif

      // Did the day change ?
      if(now.day() != prevDate.day() || firstPass)
      {
    // Update sunrise/sunset dates
        sunrise_sunset_dates = CalculateSunriseSunset(LOCAL_LATTITUDE, LOCAL_LONGITUDE, now, LOCAL_TIMEZONE(now));
      }

    // Computing Unix time
      uint32_t sunriseUnixTime = sunrise_sunset_dates.SunriseDate.unixtime();
      uint32_t sunsetUnixTime = sunrise_sunset_dates.SunsetDate.unixtime();

      // If we just passed sunrise time
      if(now.unixtime() >= sunriseUnixTime && (prevDate.unixtime() < sunriseUnixTime || (firstPass && now.unixtime() < sunsetUnixTime)) )
      {
        // Wake up ! Go to day heat mode
        currentMode = MODES_DAY_HEAT;
        #ifdef SERIAL_DEBUG
        Serial.println("MODES_DAY_HEAT");
        #endif
      } else {
        // If we just passed the sunset time
        if((now.unixtime() >= sunsetUnixTime && prevDate.unixtime() < sunsetUnixTime) || firstPass)
        {
          // Go to starting night mode (i.e. check if the boiler is hot enough and if not, force heating)
          currentMode = MODES_STARTING_FORCED_HEAT;
          #ifdef SERIAL_DEBUG
          Serial.println("MODES_STARTING_FORCED_HEAT");
          #endif
        }
      }
    } else {
      if(firstPass)
      {
        // If RTC is not there, failed to start or so, go to day heat mode
        currentMode = MODES_DAY_HEAT;
      }
    }

    elapsedTime = 0;
    firstPass = false;
  }

  elapsedTime += dT;
}
