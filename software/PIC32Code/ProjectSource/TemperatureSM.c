/****************************************************************************
 Module
   TemperatureSM.c

 Revision
   1.0.1

 Description
   This is a file for implementing the Temperature state machine.
 
 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TemperatureSM.h"
#include "ADC_HAL.h"
#include "DisplaySM.h"
#include "dbprintf.h"
#include "WiFiSM.h"
#include <math.h>

/*----------------------------- Module Defines ----------------------------*/
#define UPDATE_TIME 500
#define BETA 3892 // For thermistor equation
#define R_25 10000 // 10k resistance at 25 C
#define R1 10000 // Resistance of fixed resistor in voltage divider
#define T_CALIBRATE 4

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static int16_t VoltageToTemperature(uint16_t Reading);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static TemperatureState_t CurrentState;
static uint16_t ADC_Results[2];
static TemperatureUnit_t TempUnit = Celsius;
static uint16_t CurrentTemp = 0;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemperatureSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitTemperatureSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_Temperature;
  
  // Set temperature sensor pin to analog input
  TRISASET = _TRISA_TRISA8_MASK;  // input
  ANSELASET = _ANSELA_ANSA8_MASK; // analog
  
  InitADC(); // Initialize the ADC
  ReadADC(ADC_Results);
 
  // Get the initial temperature and send to the display
  int16_t Temp = VoltageToTemperature(ADC_Results[0]);
  ES_Event_t NewEvent = {EV_UPDATE_TEMP, Temp};
  PostDisplaySM(NewEvent);
  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostTemperatureSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostTemperatureSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemperatureSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunTemperatureSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_Temperature: 
    {
      if (ThisEvent.EventType == ES_INIT)
      {
        CurrentState = PublishingTemp;
        ES_Timer_InitTimer(TEMPERATURE_UPDATE_TIMER, UPDATE_TIME);
      }
    }
    break;

    case PublishingTemp:
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT:
        {
            ReadADC(ADC_Results);
 
            // Get the temperature and send to the display
            int16_t Temp = VoltageToTemperature(ADC_Results[0]);
            ES_Event_t NewEvent = {EV_UPDATE_TEMP, Temp};
            PostDisplaySM(NewEvent);
            PostWiFiSM(NewEvent);
            
            CurrentTemp = Temp;
//            DB_printf("Temperature: %d\r\n", Temp);
            ES_Timer_InitTimer(TEMPERATURE_UPDATE_TIMER, UPDATE_TIME);
        }
        break;
        
        case EV_BEGIN_UNIT_SELECT:
        {
//            CurrentState = NotPublishingTemp;
        }
        break;

        default:
          ;
      }
    }
    break;
    
    case NotPublishingTemp:
    {
      switch (ThisEvent.EventType)
      {       
        case EV_END_UNIT_SELECT:
        {
            CurrentState = PublishingTemp;
            
            ReadADC(ADC_Results);
 
            // Get the temperature and send to the display
            int16_t Temp = VoltageToTemperature(ADC_Results[0]);
            ES_Event_t NewEvent = {EV_UPDATE_TEMP, Temp};
            PostDisplaySM(NewEvent);
            PostWiFiSM(NewEvent);
            
            CurrentTemp = Temp;
//            DB_printf("Temperature: %d\r\n", Temp);
            ES_Timer_InitTimer(TEMPERATURE_UPDATE_TIMER, UPDATE_TIME);
        }
        break;

        default:
          ;
      }
    }
    break;
    

    default:
      ;
  } 
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTemperatureSM

 Parameters
     None

 Returns
     TemperatureState_t The current state of the Temperature state machine

 Description
     returns the current state of the Temperature state machine
 Notes

****************************************************************************/
TemperatureState_t QueryTemperatureSM(void)
{
  return CurrentState;
}

/****************************************************************************
 Function
     SetTemperatureUnit

 Parameters
     uint8_t unit: the desired unit: 0 = Celsius, >=1 is fahrenheit

 Returns
     None

 Description
     Sets the units of temperature to read
 Notes

****************************************************************************/
void SetTemperatureUnit(uint8_t unit)
{
    if (unit) {
//        DB_printf("Switched to Fahrenheit\r\n");
        TempUnit = Fahrenheit;
    } else {
//        DB_printf("Switched to Celsius\r\n");
        TempUnit =  Celsius;
    }
}

/****************************************************************************
 Function
     GetCurrentTemp

 Parameters
     None

 Returns
     uint16_t: the stored current temperature

 Description
     Returns the latest stored temperature
 Notes

****************************************************************************/
uint16_t GetCurrentTemp(void)
{
    return CurrentTemp;
}

/****************************************************************************
 Function
     GetCurrentTemp

 Parameters
     None

 Returns
     TemperatureUnit_t: the current unit

 Description
     Returns the current temperature unit
 Notes

****************************************************************************/
TemperatureUnit_t GetTempUnit(void) {
    return TempUnit;
}
/***************************************************************************
 private functions
 ***************************************************************************/
static int16_t VoltageToTemperature(uint16_t Reading)
{
//    DB_printf("Reading: %d\r\n", Reading);
    double V_out = (double)Reading / 4095 * 3.3; // Calculate voltage corresponding to ADC reading
    double R_thermistor = R1 / (3.3 - V_out) * V_out;
    double T = BETA / (BETA/298.1 - log(R_25 / R_thermistor))-273.1; // In celsius
    
    T = T-T_CALIBRATE;
    
    // Convert to fahrenheit if needed
    if (TempUnit == Fahrenheit) {
        T = T * 9 / 5 + 32;
    }
    
    return roundf(T);
}