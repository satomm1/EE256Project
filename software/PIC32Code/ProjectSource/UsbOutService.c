/****************************************************************************
 Module
   UsbOutService.c

 Revision
   1.0.1

 Description
   This is the service for providing updates to the terminal via usb. Can also
   take keystroke inputs from the user to do useful stuff

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "UsbOutService.h"

// Hardware
#include <xc.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"
#include "DisplaySM.h"
#include "PumpSM.h"
#include "SoilMoistureSM.h"
#include "TemperatureSM.h"
#include "WiFiSM.h"
#include "WaterButtonSM.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 10.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

//#define DEBUGGING

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitUsbOutService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

****************************************************************************/
bool InitUsbOutService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  // When doing testing, it is useful to announce just which program
  // is running.
  clrScrn();
  puts("\rSmart Pot \r");
  DB_printf( "\n\r\n");
  DB_printf( "Press 'f' to switch to Fahrenheit \n\r");
  DB_printf( "Press 'c' to switch to Celsius \n\r");
  DB_printf( "Press 'w' to water the plant \n\r");
  DB_printf( "Press 't' to switch water level threshold \n\r");

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
     PostUsbOutService

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostUsbOutService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunUsbOutService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

****************************************************************************/
ES_Event_t RunUsbOutService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (ThisEvent.EventType)
  {
    case ES_INIT:
    {
        ES_Timer_InitTimer(USB_UPDATE_TIMER, TWO_SEC);
    }
    break;
    
    case ES_TIMEOUT:   // re-start timer & announce
    {
        ES_Timer_InitTimer(USB_UPDATE_TIMER, TWO_SEC);
        clrScrn();
        puts("\rSmart Pot, Matthew Sato, EE256 Final Project \r");
        DB_printf( "\n\r\n");
        DB_printf( "Press 'f' to switch to Fahrenheit \n\r");
        DB_printf( "Press 'c' to switch to Celsius \n\r");
        DB_printf( "Press 'w' to water the plant \n\r");
        DB_printf( "Press 't' to switch water level threshold \n\r");
        
        TemperatureUnit_t TempUnit = GetTempUnit();
        if (TempUnit == Celsius) {
            DB_printf( "Temperature: %d C \n\r", GetCurrentTemp());
        } else {
            DB_printf( "Temperature: %d F \n\r", GetCurrentTemp());
        }
        DB_printf( "Soil Moisture: %d%% \n\r", GetCurrentSoilMoisture());
        
        bool threshold = GetCurrentThreshold();
        if (threshold) {
            DB_printf("Threshold = 30%% \n\r");
        } else {
            DB_printf("Threshold = 20%% \n\r");
        }
        
        if (GetWaterStatus()) {
            DB_printf("\r\n**************************\r\n");
            DB_printf("WATER LOW!!!\r\nRefill Water\r\n");
            DB_printf("**************************\r\n");
        }
        // TODO: Print current temp, water level, etc...
    }
    break;
    
    case ES_NEW_KEY:
    {
        #ifdef DEBUGGING
        // Announce the key press (debugging only)
        DB_printf("ES_NEW_KEY received with -> %c <- in Service 0\r\n",
          (char)ThisEvent.EventParam);
        #endif

        if (('f' == ThisEvent.EventParam) || ('F' == ThisEvent.EventParam))
        {
          SetTemperatureUnit(1);
          ES_Event_t NewEvent = {EV_SEND_WIFI_UNIT_UPDATE, 1};
          PostWiFiSM(NewEvent);
          PostWiFiSM(NewEvent);
        }

        if (('c' == ThisEvent.EventParam) || ('C' == ThisEvent.EventParam))
        {
          SetTemperatureUnit(0);
          ES_Event_t NewEvent = {EV_SEND_WIFI_UNIT_UPDATE, 0};
          PostWiFiSM(NewEvent);
          PostWiFiSM(NewEvent);
        }
        
        if (('w' == ThisEvent.EventParam) || ('W' == ThisEvent.EventParam))
        {
            ThisEvent.EventType = EV_WATER_PRESS;
            ThisEvent.EventParam = 0;
            PostPumpSM(ThisEvent);
        }
        
        if (('t' == ThisEvent.EventParam) || ('T' == ThisEvent.EventParam))
        {
            ToggleThreshold();
        }        
    }
    break;
    
    default:
    {}
     break;
  }

  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

