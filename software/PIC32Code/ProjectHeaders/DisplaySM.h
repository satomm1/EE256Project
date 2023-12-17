/****************************************************************************

  Header file Display SM

 ****************************************************************************/

#ifndef DisplaySM_H
#define DisplaySM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_Display, DisplayWaiting, ButtonDown, ButtonUp, F_UnitSelect, C_UnitSelect, F_UnitButtonDown, C_UnitButtonDown, SoilMoistureSelect, SoilMoistureButtonDown, DisplayWaitingSoilMoisture
}DisplayState_t;

// Public Function Prototypes

bool InitDisplaySM(uint8_t Priority);
bool PostDisplaySM(ES_Event_t ThisEvent);
ES_Event_t RunDisplaySM(ES_Event_t ThisEvent);
DisplayState_t QueryDisplaySM(void);

#endif /* DisplaySM_H */

