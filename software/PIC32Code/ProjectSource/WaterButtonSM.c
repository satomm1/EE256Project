/****************************************************************************
 Module
   WaterButtonSM.c

 Revision
   1.0.1

 Description
   Implements the button flat state machine. Provides software debounce for the
   button

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "WaterButtonSM.h"
#include "EventCheckers.h"
#include "dbprintf.h"
#include "DisplaySM.h"
#include "WiFiSM.h"

/*----------------------------- Module Defines ----------------------------*/
#define DEBOUNCE_TIME 50
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static WaterButtonState_t CurrentState;
static bool WaterStatus;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitWaterButtonSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitWaterButtonSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_WaterButton;
  
  // Set WaterButton Pin to digital input
  TRISDSET = _TRISD_TRISD8_MASK;
  
  InitWaterButton(); // Init the button event checker
  
  // Set water low pin to Digital Output
  TRISACLR = _TRISA_TRISA12_MASK;
  ANSELACLR = _ANSELA_ANSA12_MASK;
  
  if (PORTDbits.RD8) {
    LATAbits.LATA12 = 0; // Start off with water low light off
    
    ES_Event_t NewEvent = {EV_SEND_WATER_LOW_UPDATE, 0};
    PostWiFiSM(NewEvent);
    PostWiFiSM(NewEvent);
    
    WaterStatus = false;
  } else {
    LATAbits.LATA12 = 1; // Start off with water low light on  
    
    ES_Event_t NewEvent = {EV_SEND_WATER_LOW_UPDATE, 1};
    PostWiFiSM(NewEvent);
    PostWiFiSM(NewEvent);
    
    WaterStatus = true;
  }
  
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
     PostWaterButtonSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostWaterButtonSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunWaterButtonSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunWaterButtonSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_WaterButton:
    {
      if (ThisEvent.EventType == ES_INIT) 
      {
        CurrentState = WaterDebouncingWait;
      }
    }
    break;

    case WaterDebouncingWait: 
    {
      switch (ThisEvent.EventType)
      {
        case EV_WATER_BUTTON_DOWN: 
            {  
                ES_Timer_InitTimer(WATER_DEBOUNCE_TIMER, DEBOUNCE_TIME);           
                CurrentState = WaterDebouncingFall; 
            }
            break;
        
            case EV_WATER_BUTTON_UP:
            {
                ES_Timer_InitTimer(WATER_DEBOUNCE_TIMER, DEBOUNCE_TIME);           
                CurrentState = WaterDebouncingRise; 
            }
            break;
        
            default:
            ;
      }
    }
    break;
    
    case WaterDebouncingFall:
    {
        switch (ThisEvent.EventType)
        {
            case EV_WATER_BUTTON_UP:
            {       
                CurrentState = WaterDebouncingWait; 
            }
            break;
            
            case ES_TIMEOUT:
            {
//                DB_printf("Water Button Pressed\r\n");
                LATAbits.LATA12 = 0; // Turn water low light off    
                
                ES_Event_t NewEvent = {EV_SEND_WATER_LOW_UPDATE, 0};
                PostWiFiSM(NewEvent);
                PostWiFiSM(NewEvent);
                
                WaterStatus = false;
                CurrentState = WaterDebouncingWait; 
            }
            break;
        }
    }
    break;
    
    case WaterDebouncingRise:
    {
        switch (ThisEvent.EventType)
        {
            case EV_WATER_BUTTON_DOWN:
            {       
                CurrentState = WaterDebouncingWait; 
            }
            break;
            
            case ES_TIMEOUT:
            {
//                DB_printf("Water Button Released\r\n");
                LATAbits.LATA12 = 1; // Turn water low light on
                ES_Event_t NewEvent = {EV_SEND_WATER_LOW_UPDATE, 1};
                PostWiFiSM(NewEvent);
                PostWiFiSM(NewEvent);
                
                WaterStatus = true;
                CurrentState = WaterDebouncingWait; 
            }
            break;
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
     QueryWaterButtonSM

 Parameters
     None

 Returns
     WaterButtonState_t The current state of the WaterButton state machine

 Description
     returns the current state of the WaterButton state machine
 Notes

****************************************************************************/
WaterButtonState_t QueryWaterButtonSM(void)
{
  return CurrentState;
}

/****************************************************************************
 Function
     GetWaterStatus
 Parameters
     None

 Returns
     bool: the water status, true if low on water, false if OK

 Description
     returns the status of the water reservoir level
 Notes

****************************************************************************/
bool GetWaterStatus(void)
{
    return WaterStatus;
}
/***************************************************************************
 private functions
 ***************************************************************************/

