/****************************************************************************
 Module
   PumpSM.c

 Revision
   1.0.1

 Description
   Implements the Pump flat state machine.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PumpSM.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static PumpState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPumpSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitPumpSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_Pump;
  
  // Set pin to be a digital output
  TRISCCLR = _TRISC_TRISC10_MASK;
  ANSELCCLR = _ANSELC_ANSC10_MASK;
  LATCbits.LATC10 = 0; // Set to be low initially
  
  // Set up PWM (output compare) if deemed to be necessary
  
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
     PostPumpSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostPumpSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPumpSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunPumpSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_Pump: 
    {
      if (ThisEvent.EventType == ES_INIT)
      {
        // now put the machine into the actual initial state
        CurrentState = WaitingPump;
      }
    }
    break;

    case WaitingPump:
    {
      switch (ThisEvent.EventType)
      {
        case EV_ADD_WATER: 
        {
          LATCbits.LATC10 = 1; // Start Pumping
          ES_Timer_InitTimer(PUMP_TIMER, ThisEvent.EventParam); // Timer for when to stop pumping
          CurrentState = Pumping;
        }
        break;
        
        case EV_WATER_PRESS:
        {
          LATCbits.LATC10 = 1; // Start Pumping
          ES_Timer_InitTimer(PUMP_TIMER, 2000); // Pump for 2 second
          CurrentState = Pumping;
        }
        break;

        default:
          ;
      }
    }
    break;
    
    case Pumping:
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT: 
        {
          LATCbits.LATC10 = 0; // Stop Pumping
          CurrentState = WaitingPump;
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
     QueryPumpSM

 Parameters
     None

 Returns
     PumpState_t The current state of the Pump state machine

 Description
     returns the current state of the Pump state machine
 Notes

****************************************************************************/
PumpState_t QueryPumpSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

