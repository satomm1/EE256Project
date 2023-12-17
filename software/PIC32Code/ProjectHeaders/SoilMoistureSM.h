/****************************************************************************

  Header file SoilMoisture SM

 ****************************************************************************/

#ifndef SoilMoistureSM_H
#define SoilMoistureSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState_SoilMoisture, SoilMoistureWaiting, SoilMoistureMeasuring, SoilMoistureAfterWatering, SoilMoistureMeasuringAfterWatering
}SoilMoistureState_t;

// Public Function Prototypes

bool InitSoilMoistureSM(uint8_t Priority);
bool PostSoilMoistureSM(ES_Event_t ThisEvent);
ES_Event_t RunSoilMoistureSM(ES_Event_t ThisEvent);
SoilMoistureState_t QuerySoilMoistureSM(void);
void ToggleThreshold(void);
void SetThreshold(bool NewThreshold);
uint16_t GetCurrentSoilMoisture(void);
bool GetCurrentThreshold(void);

#endif /* SoilMoistureSM_H */

