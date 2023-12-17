/****************************************************************************
 Module
   DisplaySM.c

 Revision
   1.0.1

 Description
   Implements the ___ flat state machine.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "DisplaySM.h"
#include <sys/attribs.h>
#include "PumpSM.h"
#include "TemperatureSM.h"
#include "SoilMoistureSM.h"
#include "WiFiSM.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define SPI2_BRG_DIVISOR 10000
#define UPDATE_TIME     1000 // How often to update the display
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static DisplayState_t CurrentState;
static uint16_t CurrentTemp = 0;

const uint16_t ones_digit_map[10] = {     
    0b0011111100000000, // 0
    0b0000011000000000, // 1
    0b0101101100000000, // 2
    0b0100111100000000, // 3
    0b0110011000000000, // 4
    0b0110110100000000, // 5
    0b0111110100000000, // 6
    0b0000011100000000, // 7
    0b0111111100000000, // 8
    0b0110011100000000, // 9
};

const uint16_t tens_digit_map[10] = {     
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01100111, // 9
};

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitDisplaySM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitDisplaySM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_Display;
  
    ////////////////////// Set Up SPI2 /////////////////////////////////////////
  // SDO2 (RB15), SS2 (RB14), SCK2 (RA7) Digital Output
  TRISACLR = _TRISA_TRISA7_MASK;
  TRISBCLR = _TRISB_TRISB14_MASK | _TRISB_TRISB15_MASK;
  RPA7R = 0b00100; // Map RA7 to SCK2
  RPB14R = 0b00100; // Map RB14 to SS2
  RPB15R = 0b00100; // Map RPB15 to SDO2
  
  SPI2CON = 0; // Turn SPI off and reset all settings
  SPI2CONbits.MSSEN = 1; // Automatically drive SS pin
  SPI2CONbits.MCLKSEL = 0; // Use PBCLK2 for Baud Rate Generator
  SPI2CONbits.ENHBUF = 0; // Disable enhanced buffer
  SPI2CONbits.DISSDO = 0; // SDO2 controlled by SPI
  SPI2CONbits.MODE32 = 0; // 16 bit mode
  SPI2CONbits.MODE16 = 1; // 16 bit mode
  SPI2CONbits.SMP = 0; // Sample data at middle of data output time
  SPI2CONbits.CKE = 1; // Serial output data changes on transition from active clock state to Idle clock state
  SPI2CONbits.CKP = 0; // Idle state for clock is a low level; active state is a high level
  SPI2CONbits.MSTEN = 1; // Host Mode
  SPI2CONbits.DISSDI = 1; // SDI2 Pin not controlled by SPI module
  SPI2CONbits.STXISEL = 0b00; // Interrupt generated when last transfer shifted out of SPISR and transmit operations are complete
  SPI2CONbits.SRXISEL = 0b01; // Interrupt generated when buffer is not empty
  
  SPI2CON2 = 0; // Clear the register
  SPI2BRG = SPI2_BRG_DIVISOR;
    
  SPI2CONbits.ON = 1; // Turn SPI2 On 
  
  
  //////////////////////////// Finish SPI Setup ////////////////////////////////
    
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
     PostDisplaySM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostDisplaySM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunDisplaySM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunDisplaySM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_Display: 
    {
      if (ThisEvent.EventType == ES_INIT)
      {
        CurrentState = DisplayWaiting;
        SPI2BUF = 0;
      }
    }
    break;

    case DisplayWaiting:
    {
      switch (ThisEvent.EventType)
      {
        case EV_UPDATE_TEMP: 
        {         
          uint16_t Data2Send = 0;
          uint16_t Digit2Send = ThisEvent.EventParam;
          if (Digit2Send == CurrentTemp) {
              // Don't Need to do anything here
          } else {
              CurrentTemp = Digit2Send;
              uint8_t ones_digit = Digit2Send % 10;
              uint8_t tens_digit = (Digit2Send / 10) % 10 ;
              Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
              SPI2BUF = Data2Send;
          }
          
        }
        break;
        
        case EV_USER_BUTTON_PRESSED:
        {
            CurrentState = ButtonDown;
            ES_Timer_InitTimer(DISPLAY_TIMER, 1000); // Set 1 second timer to see if long press
        }
        break;
        
        default:
          ;
      }
    }
    break;
        
    case ButtonDown: 
    {         
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT: 
        {         
            ES_Event_t NewEvent = {EV_WATER_PRESS, 0};
            PostPumpSM(NewEvent);
            CurrentState = DisplayWaiting;
        }
        break;
        
        case EV_USER_BUTTON_RELEASED: 
        {         
            CurrentState = ButtonUp;
            ES_Timer_InitTimer(DISPLAY_TIMER, 250); // Set 0.5 Second timer to see if single click or double click
        }
        break;
        
        default:
          ;
      }
    }
    break;

    case ButtonUp: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_PRESSED: 
        {         
          CurrentState = F_UnitSelect;
          
          ES_Event_t NewEvent = {EV_BEGIN_UNIT_SELECT, 0};
          PostTemperatureSM(NewEvent); // Tell Temp SM to not publish anything while in unit select
          
          ES_Timer_InitTimer(DISPLAY_TIMER, 5000); // Set 5 second timer to timeout of unit select mode

          SPI2BUF = 0b0111000100000000; // 'F'
        }
        break;
        
        case ES_TIMEOUT: 
        {         
          CurrentState = DisplayWaiting;
          ToggleThreshold();
        }
        break;
        
        default:
          ;
      }
    }
    break;

    case F_UnitSelect: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_PRESSED: 
        {         
            CurrentState = F_UnitButtonDown;
            ES_Timer_InitTimer(DISPLAY_TIMER, 1000); // 1 sec timer to see if long press to exit unit select mode          
        }
        break;
        
        case ES_TIMEOUT: 
        {         
            CurrentState = DisplayWaiting;
            
            SetTemperatureUnit(1); // Set Temperature to Fahrenheit mode
            
            ES_Event_t NewEvent = {EV_END_UNIT_SELECT, 0};
            PostTemperatureSM(NewEvent); // Tell Temp SM to resume publishing
            
            // Tell WiFi we are now in Fahrenheit
            ES_Event_t NewEvent1 = {EV_SEND_WIFI_UNIT_UPDATE, 1};
            PostWiFiSM(NewEvent1);
            PostWiFiSM(NewEvent1);
            
            CurrentTemp = GetCurrentTemp();
            uint8_t ones_digit = CurrentTemp % 10;
            uint8_t tens_digit = (CurrentTemp / 10) % 10 ;
            uint16_t Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
            SPI2BUF = Data2Send;
        }
        break;
        
        default:
          ;
      }
    }
    break;

    case F_UnitButtonDown: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_RELEASED: 
        {         
            CurrentState = C_UnitSelect;
            ES_Timer_InitTimer(DISPLAY_TIMER, 5000);
                        
            SPI2BUF = 0b0011100100000000; // 'C'
        }
        break;
        
        case ES_TIMEOUT: 
        {         
            CurrentState = DisplayWaiting;
            
            SetTemperatureUnit(1); // Set Temperature to fahrenheit mode
            
            ES_Event_t NewEvent = {EV_END_UNIT_SELECT, 0};
            PostTemperatureSM(NewEvent); // Tell Temp SM to resume publishing
            
            // Tell WiFi we are now in Fahrenheit
            ES_Event_t NewEvent1 = {EV_SEND_WIFI_UNIT_UPDATE, 1};
            PostWiFiSM(NewEvent1);
            PostWiFiSM(NewEvent1);
            
            CurrentTemp = GetCurrentTemp();
            uint8_t ones_digit = CurrentTemp % 10;
            uint8_t tens_digit = (CurrentTemp / 10) % 10 ;
            uint16_t Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
            SPI2BUF = Data2Send;
        }
        break;
        
        default:
          ;
      }
    }
    break;

    case C_UnitSelect: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_PRESSED: 
        {         
            CurrentState = C_UnitButtonDown;
            ES_Timer_InitTimer(DISPLAY_TIMER, 1000); // Timer to see if long hold to exit unit select mode
        }
        break;
        
        case ES_TIMEOUT: 
        {         
            CurrentState = DisplayWaiting;
            
            SetTemperatureUnit(0); // Set Temperature to Celsius mode
            
            ES_Event_t NewEvent = {EV_END_UNIT_SELECT, 0};
            PostTemperatureSM(NewEvent); // Tell Temp SM to resume publishing
            
            ES_Event_t NewEvent1 = {EV_SEND_WIFI_UNIT_UPDATE, 0};
            PostWiFiSM(NewEvent1);
            PostWiFiSM(NewEvent1);
            
            CurrentTemp = GetCurrentTemp();
            uint8_t ones_digit = CurrentTemp % 10;
            uint8_t tens_digit = (CurrentTemp / 10) % 10 ;
            uint16_t Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
            SPI2BUF = Data2Send;
        }
        break;        
        
        default:
          ;
      }
    }
    break;

    case C_UnitButtonDown: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_RELEASED: 
        {         
            CurrentState = SoilMoistureSelect;
            ES_Timer_InitTimer(DISPLAY_TIMER, 5000);
                        
            SPI2BUF = 0b0111001100000000; // 'P'
        }
        break;
        
        case ES_TIMEOUT: 
        {         
            CurrentState = DisplayWaiting;
            
            SetTemperatureUnit(0); // Set Temperature to Celsius mode
            
            ES_Event_t NewEvent1 = {EV_SEND_WIFI_UNIT_UPDATE, 0};
            PostWiFiSM(NewEvent1);
            PostWiFiSM(NewEvent1);
            
            
            CurrentTemp = GetCurrentTemp();
            uint8_t ones_digit = CurrentTemp % 10;
            uint8_t tens_digit = (CurrentTemp / 10) % 10 ;
            uint16_t Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
            SPI2BUF = Data2Send;
        }
        break;
        
        default:
          ;
      }
    }
    break;
    
    case SoilMoistureSelect: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_PRESSED: 
        {         
            CurrentState = SoilMoistureButtonDown;
            ES_Timer_InitTimer(DISPLAY_TIMER, 1000); // Timer to see if long hold to exit unit select mode
        }
        break;
        
        case ES_TIMEOUT: 
        {         
            uint16_t Data2Send = 0;
            uint16_t Digit2Send = GetCurrentSoilMoisture();
            if (Digit2Send > 99) {
              Digit2Send = 99;
            }
            uint8_t ones_digit = Digit2Send % 10;
            uint8_t tens_digit = (Digit2Send / 10) % 10 ;
            Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
            SPI2BUF = Data2Send;
            CurrentState = DisplayWaitingSoilMoisture;
        }
        break;        
        
        default:
          ;
      }
    }
    break;
    
    case SoilMoistureButtonDown: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_USER_BUTTON_RELEASED: 
        {         
            CurrentState = F_UnitSelect;
            ES_Timer_InitTimer(DISPLAY_TIMER, 5000);
                        
            SPI2BUF = 0b0111000100000000; // 'F'
        }
        break;
        
        case ES_TIMEOUT: 
        {         
            uint16_t Data2Send = 0;
            uint16_t Digit2Send = GetCurrentSoilMoisture();
            if (Digit2Send > 99) {
              Digit2Send = 99;
            }
            uint8_t ones_digit = Digit2Send % 10;
            uint8_t tens_digit = (Digit2Send / 10) % 10 ;
            Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
            SPI2BUF = Data2Send;
            CurrentState = DisplayWaitingSoilMoisture;
        }
        break;
        
        default:
          ;
      }
    }
    break;
    
    case DisplayWaitingSoilMoisture: 
    {         
      switch (ThisEvent.EventType)
      {
        case EV_SEND_WIFI_MOISTURE_UPDATE: 
        {         
          uint16_t Data2Send = 0;
          uint16_t Digit2Send = ThisEvent.EventParam;
          if (Digit2Send > 99) {
              Digit2Send = 99;
          }
          uint8_t ones_digit = Digit2Send % 10;
          uint8_t tens_digit = (Digit2Send / 10) % 10 ;
          Data2Send = tens_digit_map[tens_digit] | ones_digit_map[ones_digit];
          SPI2BUF = Data2Send;
        }
        break;
        
        case EV_USER_BUTTON_PRESSED:
        {
            CurrentState = ButtonDown;
            ES_Timer_InitTimer(DISPLAY_TIMER, 1000); // Set 1 second timer to see if long press
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
     QueryDisplaySM

 Parameters
     None

 Returns
     DisplayState_t The current state of the Display state machine

 Description
     returns the current state of the Display state machine
 Notes

****************************************************************************/
DisplayState_t QueryDisplaySM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
