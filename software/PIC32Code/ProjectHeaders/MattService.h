/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ServMatt_H
#define ServMatt_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitMattService(uint8_t Priority);
bool PostMattService(ES_Event_t ThisEvent);
ES_Event_t RunMattService(ES_Event_t ThisEvent);

#endif /* ServMatt_H */

