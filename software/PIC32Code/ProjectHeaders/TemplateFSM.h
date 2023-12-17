/****************************************************************************

  Header file __ FSM

 ****************************************************************************/

#ifndef TemplateFSM_H
#define TemplateFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState, UnlockWaiting, _1UnlockPress,
  _2UnlockPresses, Locked
}TemplateState_t;

// Public Function Prototypes

bool InitTemplateFSM(uint8_t Priority);
bool PostTemplateFSM(ES_Event_t ThisEvent);
ES_Event_t RunTemplateFSM(ES_Event_t ThisEvent);
TemplateState_t QueryTemplateFSM(void);

#endif /* TemplateFSM_H */

