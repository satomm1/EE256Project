/****************************************************************************

  Header file UserButton SM

 ****************************************************************************/

#ifndef UserButtonSM_H
#define UserButtonSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_UserButton, UserDebouncingWait, UserDebouncingFall, UserDebouncingRise
}UserButtonState_t;

// Public Function Prototypes

bool InitUserButtonSM(uint8_t Priority);
bool PostUserButtonSM(ES_Event_t ThisEvent);
ES_Event_t RunUserButtonSM(ES_Event_t ThisEvent);
UserButtonState_t QueryUserButtonSM(void);

#endif /* UserButtonSM_H */

