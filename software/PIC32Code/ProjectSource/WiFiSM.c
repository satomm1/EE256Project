/****************************************************************************
 Module
   WiFiSM.c

 Revision
   1.0.1

 Description
   Implements the WiFi flat state machine.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "WiFiSM.h"
#include "dbprintf.h"
#include "PumpSM.h"
#include "TemperatureSM.h"
#include "SoilMoistureSM.h"

/*----------------------------- Module Defines ----------------------------*/
#define SPI_BRG_DIVISOR 1000
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static WiFiState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static uint8_t LastReceivedMessage = 0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitWiFiSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

****************************************************************************/
bool InitWiFiSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState_WiFi;
  
  ////////////////////// Set Up SPI1 /////////////////////////////////////////
  // SDI1 Digital Input (RC7)
  TRISCSET = _TRISC_TRISC7_MASK;
  SDI1R = 0b0101; // Map SDI1 to C7
  
  // SDO1 (RB5), SS1 (RB6), SSCK1 (RB7) Digital Output
  TRISBCLR = _TRISB_TRISB5_MASK | _TRISB_TRISB6_MASK | _TRISB_TRISB7_MASK;
  ANSELBCLR = _ANSELB_ANSB7_MASK;
  RPB5R = 0b00011; // Map RB5 to SDO1
  RPB6R = 0b00011; // Map RPB6 to SS1
  
  SPI1CON = 0; // Turn SPI off and reset all settings
  SPI1CONbits.MSSEN = 1; // Automatically drive SS pin
  SPI1CONbits.MCLKSEL = 0; // Use PBCLK2 for Baud Rate Generator
  SPI1CONbits.ENHBUF = 1; // Disable enhanced buffer
  SPI1CONbits.DISSDO = 0; // SDO1 controlled by SPI
  SPI1CONbits.MODE32 = 0; // 8 bit mode
  SPI1CONbits.MODE16 = 0; // 8 bit mode
  SPI1CONbits.SMP = 0; // Sample data at middle of data output time
  SPI1CONbits.CKE = 0; // Serial output data changes on transition from Idle clock state to active clock state
  SPI1CONbits.CKP = 0; // Idle state for clock is a low level; active state is a high level
  SPI1CONbits.MSTEN = 1; // Host Mode
  SPI1CONbits.DISSDI = 0; // SDI1 Pin controlled by SPI module
  SPI1CONbits.STXISEL = 0b00; // Interrupt generated when last transfer shifted out of SPISR and transmit operations are complete
  SPI1CONbits.SRXISEL = 0b01; // Interrupt generated when buffer is not empty
  
  SPI1CON2 = 0; // Clear the register
  SPI1BRG = SPI_BRG_DIVISOR;
    
  SPI1CONbits.ON = 1; // Turn SPI1 On
  
  // Initialize the Interrupts   
  PRISSbits.PRI7SS = 0b0111; // Assign shadow set for IPL 7
  
  IFS1CLR = _IFS1_SPI1RXIF_MASK; // Clear SPI1 RX flag
  IPC9bits.SPI1RXIP = 7; // Write SPI1 RX interrupt priority
  IEC1SET = _IEC1_SPI1RXIE_MASK; // Set SPI1 RX interrupt enable
  
  INTCONbits.MVEC = 1; // set up for multiple interrupt vectors   
  __builtin_enable_interrupts(); // enable global interrupts   
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
     PostWiFiSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

****************************************************************************/
bool PostWiFiSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunWiFiSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
****************************************************************************/
ES_Event_t RunWiFiSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState_WiFi:
    {
      if (ThisEvent.EventType == ES_INIT) 
      {
        CurrentState = WiFiWaiting;
      }
    }
    break;

    case WiFiWaiting:
    {
      switch (ThisEvent.EventType)
      {
        case EV_UPDATE_TEMP: 
        {
          uint8_t Temp = ThisEvent.EventParam;
          SPI1BUF = Temp;
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Temp;
          SPI1BUF = 0;
          SPI1BUF = 0;
        }
        break;
        
        case EV_SEND_WIFI_MOISTURE_UPDATE:
        {
//            DB_printf("Sending Moisture Update\r\n");
            uint8_t Percent = ThisEvent.EventParam;
            SPI1BUF = 0;
            SPI1BUF = Percent;
            SPI1BUF = 0;
            SPI1BUF = 0;
            SPI1BUF = Percent;
            SPI1BUF = 0;
        }
        
        case EV_SEND_WIFI_THRESHOLD_UPDATE:
        {
          uint8_t Update = 1;
          if (ThisEvent.EventParam == 1) {
            Update = Update | 1<<1;   
          }
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Update;
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Update;
        }   
        break;
        
        case EV_SEND_WIFI_UNIT_UPDATE:
        {
          uint8_t Update = 0b10000000;
          if (ThisEvent.EventParam == 1) {
            Update = Update | 0b01000000;   
          }
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Update;
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Update;
        }
        
        case EV_SEND_WATER_LOW_UPDATE:
        {
          uint8_t Update = 0b00100000;
          if (ThisEvent.EventParam ==1) {
              Update = Update | 0b00010000;
          }
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Update;
          SPI1BUF = 0;
          SPI1BUF = 0;
          SPI1BUF = Update;
        }
            
        case EV_SPI1_RX_RECEIVED: 
        {
            if (ThisEvent.EventParam != LastReceivedMessage) {
                LastReceivedMessage = ThisEvent.EventParam;
//                DB_printf("Received WiFi Data: %b\r\n", ThisEvent.EventParam);
                
                if (0b00000100 & ThisEvent.EventParam) {
                    // Water Press
                    ES_Event_t NewEvent = {EV_WATER_PRESS, 0};
                    PostPumpSM(NewEvent);
                }
                
                if (0b00000010 & ThisEvent.EventParam) {
                    // Unit Update
                    if (0b00000001 & ThisEvent.EventParam) {
                        SetTemperatureUnit(1); // Fahrenheit
                    } else {
                        SetTemperatureUnit(0); // Celsius
                    }
                }
                
                if (0b00010000 & ThisEvent.EventParam) {
                    // Threshold update
                    if (0b00001000 & ThisEvent.EventParam) {
                        SetThreshold(true); // Fahrenheit
                    } else {
                        SetThreshold(false); // Celsius
                    }
                }
            }
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
     QueryWiFiSM

 Parameters
     None

 Returns
     WiFiState_t The current state of the WiFi state machine

 Description
     returns the current state of the WiFi state machine
 Notes

****************************************************************************/
WiFiState_t QueryWiFiSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void __attribute__((interrupt(ipl7srs), at_vector(_SPI1_RX_VECTOR), aligned(16))) SPI1_RX_ISR()
{  
    IFS1CLR = _IFS1_SPI1RXIF_MASK; // clear SPI1 interrupt flag
    uint8_t ReceivedData = SPI1BUF;
    ES_Event_t NewEvent = {EV_SPI1_RX_RECEIVED, ReceivedData};
    if (ReceivedData > 0) {
      PostWiFiSM(NewEvent);
    }
}