/****************************************************************************

  Header file for Temperature FSM

 ****************************************************************************/

#ifndef TemperatureSM_H
#define TemperatureSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_Temperature, PublishingTemp, NotPublishingTemp
}TemperatureState_t;

typedef enum
{
    Fahrenheit, Celsius
} TemperatureUnit_t;

// Public Function Prototypes

bool InitTemperatureSM(uint8_t Priority);
bool PostTemperatureSM(ES_Event_t ThisEvent);
ES_Event_t RunTemperatureSM(ES_Event_t ThisEvent);
TemperatureState_t QueryTemperatureSM(void);

void SetTemperatureUnit(uint8_t unit);
uint16_t GetCurrentTemp(void);
TemperatureUnit_t GetTempUnit(void);

#endif /* TemperatureSM_H */

