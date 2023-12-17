/****************************************************************************
 Module
   SoilMoistureSM.c

 Revision
   1.0.1

 Description
   Implements the SoilMoisture flat state machine.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "SoilMoistureSM.h"
#include "ADC_HAL.h"
#include "PumpSM.h"
#include "dbprintf.h"
#include "WiFiSM.h"
#include "DisplaySM.h"

/*----------------------------- Module Defines ----------------------------*/
#define MEASURE_TIME 4500
#define WAIT_TIME   500
#define LOW_THRESHOLD 20
#define HIGH_THRESHOLD 30
#define WATERING_TIMEOUT 10000
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static SoilMoistureState_t CurrentState;
static uint16_t ADC_Results[2];
static uint16_t Threshold;
static uint16_t CurrentSoilMoisture;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitSoilMoistureSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitSoilMoistureSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_SoilMoisture;
  
  // Moisture Threshold Light Indicators
  // Digital Outputs
  TRISBCLR = _TRISB_TRISB9_MASK;
  TRISCCLR = _TRISC_TRISC6_MASK;
  ANSELBCLR = _ANSELB_ANSB9_MASK;
  
  LATBbits.LATB9 = 1; // Start in Low Threshold Mode
  LATCbits.LATC6 = 0; // Start in Low Threshold Mode
  
  
  // Soil Moisture Activate Pin to Digital Output
  TRISBCLR = _TRISB_TRISB4_MASK;
  LATBbits.LATB4 = 0; // Set low initially
  
  // Soil Moisture Sensor Pin to Analog Input
  TRISASET = _TRISA_TRISA4_MASK;
  ANSELASET = _ANSELA_ANSA4_MASK;
  
  InitADC(); // Initialize the ADC
  
  Threshold = LOW_THRESHOLD; // Start with the low threshold
  
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
     PostSoilMoistureSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostSoilMoistureSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunSoilMoistureSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunSoilMoistureSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_SoilMoisture:
    {
      if (ThisEvent.EventType == ES_INIT)
      {
        CurrentState = SoilMoistureMeasuring;
        LATBbits.LATB4 = 1; // Activate the sensor
        ES_Timer_InitTimer(SOIL_MOISTURE_TIMER, WAIT_TIME); // Wait a bit before reading the measurement
      }
    }
    break;

    case SoilMoistureWaiting:
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT:
        {  
          LATBbits.LATB4 = 1; // Activate the sensor
          ES_Timer_InitTimer(SOIL_MOISTURE_TIMER, WAIT_TIME); // Wait a bit before reading the measurement
          CurrentState = SoilMoistureMeasuring;  
        }
        break;

        default:
          ;
      }
    }
    break;
    
    case SoilMoistureMeasuring:
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT:
        {  
          ReadADC(ADC_Results); // Read the sensor
          LATBbits.LATB4 = 0; // Turn off sensor
          
//          DB_printf("Soil Moisture: %d\r\n", ADC_Results[1]);
          uint8_t soil_moisture_percent = ADC_Results[1] * 100 / 4095;
//          DB_printf("Soil Moisture Percent: %d%%\r\n",  soil_moisture_percent);
          
          CurrentSoilMoisture = soil_moisture_percent;
          
          ES_Event_t NewEvent1 = {EV_SEND_WIFI_MOISTURE_UPDATE, soil_moisture_percent};
          PostWiFiSM(NewEvent1);
          PostDisplaySM(NewEvent1);
          
          if (soil_moisture_percent < Threshold  && soil_moisture_percent >= 5) {
              // percent < 5%  indicates probes not in pot
              ES_Event_t NewEvent = {EV_ADD_WATER, 2000};
              PostPumpSM(NewEvent);
              DB_printf("Hit Threshold: Begin Pumping\r\n");
        
          } 
          CurrentState = SoilMoistureWaiting;
     
          ES_Timer_InitTimer(SOIL_MOISTURE_TIMER, MEASURE_TIME); // Set timer to measure again     
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
     QuerySoilMoistureSM

 Parameters
     None

 Returns
     SoilMoistureState_t The current state of the SoilMoisture state machine

 Description
     returns the current state of the SoilMoisture state machine
 Notes

****************************************************************************/
SoilMoistureState_t QuerySoilMoistureSM(void)
{
  return CurrentState;
}

/****************************************************************************
 Function
     ToggleThreshold

 Parameters
     None

 Returns
     None

 Description
     Changes the threshold level to begin pumping
 Notes

****************************************************************************/
void ToggleThreshold(void) 
{
    if (Threshold == LOW_THRESHOLD) {
        Threshold = HIGH_THRESHOLD;
        LATBbits.LATB9 = 0; // Now in High Threshold Mode
        LATCbits.LATC6 = 1; // Now in High Threshold Mode
        
        ES_Event_t NewEvent = {EV_SEND_WIFI_THRESHOLD_UPDATE, 1};
        PostWiFiSM(NewEvent);
        PostWiFiSM(NewEvent);
    } else {
        Threshold = LOW_THRESHOLD;
        LATBbits.LATB9 = 1; // Now in Low Threshold Mode
        LATCbits.LATC6 = 0; // Now in Low Threshold Mode
        
        ES_Event_t NewEvent = {EV_SEND_WIFI_THRESHOLD_UPDATE, 0};
        PostWiFiSM(NewEvent);
        PostWiFiSM(NewEvent);
    }
}

void SetThreshold(bool NewThreshold)
{
    if (NewThreshold) {
        Threshold = HIGH_THRESHOLD;
        LATBbits.LATB9 = 0; // Now in High Threshold Mode
        LATCbits.LATC6 = 1; // Now in High Threshold Mode
    } else {
        Threshold = LOW_THRESHOLD;
        LATBbits.LATB9 = 1; // Now in Low Threshold Mode
        LATCbits.LATC6 = 0; // Now in Low Threshold Mode
    }
}

/****************************************************************************
 Function
     GetCurrentSoilMoisture

 Parameters
     None

 Returns
     uint16_t: the stored current soil moisture percent

 Description
     Returns the latest stored soil moisture percentage
 Notes

****************************************************************************/
uint16_t GetCurrentSoilMoisture(void) {
    return CurrentSoilMoisture;
}

/****************************************************************************
 Function
     GetCurrentThreshold

 Parameters
     None

 Returns
     bool: the current threshold, false if low, true if high

 Description
     Returns the latest stored soil moisture threshold
 Notes

****************************************************************************/
bool GetCurrentThreshold(void)
{
    if (LOW_THRESHOLD == Threshold) {
        return false;
    } else {
        return true;
    }
}

/***************************************************************************
 private functions
 ***************************************************************************/

