/****************************************************************************
 Module
   UserButtonSM.c

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
#include "UserButtonSM.h"
#include "EventCheckers.h"
#include "dbprintf.h"
#include "DisplaySM.h"
/*----------------------------- Module Defines ----------------------------*/
#define DEBOUNCE_TIME 50

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static UserButtonState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitUserButtonSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitUserButtonSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_UserButton;
  
  // Set UserButton Pin to digital input
  TRISASET = _TRISA_TRISA11_MASK;
  ANSELACLR = _ANSELA_ANSA11_MASK;
  
  InitUserButton(); // Init the button event checker
  
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
     PostUserButtonSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostUserButtonSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunUserButtonSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunUserButtonSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_UserButton:
    {
      if (ThisEvent.EventType == ES_INIT) 
      {
        CurrentState = UserDebouncingWait;
      }
    }
    break;

    case UserDebouncingWait: 
    {
        switch (ThisEvent.EventType)
        {
            case EV_USER_BUTTON_DOWN: 
            {  
                ES_Timer_InitTimer(USER_DEBOUNCE_TIMER, DEBOUNCE_TIME);           
                CurrentState = UserDebouncingFall; 
            }
            break;
        
            case EV_USER_BUTTON_UP:
            {
                ES_Timer_InitTimer(USER_DEBOUNCE_TIMER, DEBOUNCE_TIME);           
                CurrentState = UserDebouncingRise; 
            }
            break;
        
            default:
            ;
        }
    }
    break;
    
    case UserDebouncingFall:
    {
        switch (ThisEvent.EventType)
        {
            case EV_USER_BUTTON_UP:
            {       
                CurrentState = UserDebouncingWait; 
            }
            break;
            
            case ES_TIMEOUT:
            {
//                DB_printf("User Button Pressed\r\n");
                ES_Event_t NewEvent = {EV_USER_BUTTON_PRESSED, 0};           
                PostDisplaySM(NewEvent);           
                CurrentState = UserDebouncingWait; 
            }
            break;
        }
    }
    break;
    
    case UserDebouncingRise:
    {
        switch (ThisEvent.EventType)
        {
            case EV_USER_BUTTON_DOWN:
            {       
                CurrentState = UserDebouncingWait; 
            }
            break;
            
            case ES_TIMEOUT:
            {
//                DB_printf("User Button Released\r\n");
                ES_Event_t NewEvent = {EV_USER_BUTTON_RELEASED, 0};           
                PostDisplaySM(NewEvent);         
                CurrentState = UserDebouncingWait; 
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
     QueryUserButtonSM

 Parameters
     None

 Returns
     UserButtonState_t The current state of the UserButton state machine

 Description
     returns the current state of the UserButton state machine
 Notes

****************************************************************************/
UserButtonState_t QueryUserButtonSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

