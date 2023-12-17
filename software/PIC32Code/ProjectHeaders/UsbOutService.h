/****************************************************************************

  Header file for Test Harness Service0
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef UsbOutService_H
#define UsbOutService_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT
// Public Function Prototypes

bool InitUsbOutService(uint8_t Priority);
bool PostUsbOutService(ES_Event_t ThisEvent);
ES_Event_t RunUsbOutService(ES_Event_t ThisEvent);

#endif /* ServTemplate_H */

