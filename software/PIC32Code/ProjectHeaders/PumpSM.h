/****************************************************************************

  Header file Pump SM

 ****************************************************************************/

#ifndef PumpSM_H
#define PumpSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_Pump, WaitingPump, Pumping
}PumpState_t;

// Public Function Prototypes

bool InitPumpSM(uint8_t Priority);
bool PostPumpSM(ES_Event_t ThisEvent);
ES_Event_t RunPumpSM(ES_Event_t ThisEvent);
PumpState_t QueryPumpSM(void);

#endif /* PumpSM_H */

