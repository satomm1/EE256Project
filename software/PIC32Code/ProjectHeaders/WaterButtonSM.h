/****************************************************************************

  Header file WaterButton SM

 ****************************************************************************/

#ifndef WaterButtonSM_H
#define WaterButtonSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_WaterButton, WaterDebouncingWait, WaterDebouncingFall, WaterDebouncingRise
}WaterButtonState_t;

// Public Function Prototypes

bool InitWaterButtonSM(uint8_t Priority);
bool PostWaterButtonSM(ES_Event_t ThisEvent);
ES_Event_t RunWaterButtonSM(ES_Event_t ThisEvent);
WaterButtonState_t QueryWaterButtonSM(void);
bool GetWaterStatus(void);
#endif /* WaterButtonSM_H */

