/****************************************************************************

  Header file WiFi FSM

 ****************************************************************************/

#ifndef WiFiSM_H
#define WiFiSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_WiFi, WiFiWaiting, WiFiWriting
}WiFiState_t;

// Public Function Prototypes

bool InitWiFiSM(uint8_t Priority);
bool PostWiFiSM(ES_Event_t ThisEvent);
ES_Event_t RunWiFiSM(ES_Event_t ThisEvent);
WiFiState_t QueryTemplateSM(void);

#endif /* WiFiFSM_H */

