#include <stdio.h>

#ifndef __MTA_HAL_TR104__
#define __MTA_HAL_TR104__

int mta_hal_getTR104parameterNames(char ***parameterNamesList, int *parameterListLen);
int mta_hal_freeTR104parameterNames(char **parameterNamesList, int parameterListLen);

int mta_hal_setTR104parameterValues(char **parameterValueList, int *parameterListLen);

int mta_hal_getTR104parameterValues(char **parameterNamesList, int *parameterListLen, char ***parameterValuesList);
int mta_hal_freeTR104parameterValues(char **parameterValuesList, int  parameterListLen);

#endif /* __MTA_HAL_TR104__ */

