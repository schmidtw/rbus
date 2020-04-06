/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>
#include "../common/test_macros.h"

static int gDuration = 20;

static int vcCount = 1;

void rbusValueChange_SetPollingPeriod(int seconds);

int getDurationValueChange()
{
    return gDuration;
}

static void paramValueChangeHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    char valExpectNew[32];
    char valExpectOld[32];
    char const* valActualNew;
    char const* valActualOld;
    bool pass;

    PRINT_TEST_EVENT("test_ValueChange", event, subscription);

    snprintf(valExpectNew, 32, "value %d", vcCount);
    snprintf(valExpectOld, 32, "value %d", vcCount-1);

    valActualNew = rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL);
    valActualOld = rbusValue_GetString(rbusObject_GetValue(event->data, "oldValue"), NULL);

    pass = (strcmp(valExpectNew, valActualNew)==0 && strcmp(valExpectOld, valActualOld)==0);

    TALLY(pass);

    printf("_test_ValueChange %s: expect=[value:%s oldValue:%s] actual=[value:%s oldValue:%s]\n", 
        pass ? "PASS" : "FAIL", valExpectNew, valExpectOld, valActualNew, valActualOld);

    vcCount++;
}

void testValueChange(rbusHandle_t handle, int* countPass, int* countFail)
{
    int rc;
    rbusValueChange_SetPollingPeriod(1);

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.Param1", paramValueChangeHandler, NULL);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_ValueChange rbusEvent_Subscribe Param1 %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit0;

    sleep(gDuration);

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.Param1");
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_ValueChange rbusEvent_Unsubscribe Param1 %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);

exit0:
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_ValueChange");
}

