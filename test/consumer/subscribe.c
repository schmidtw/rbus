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

static int gDuration = 10;

int gEventCounts[2] = {0,0}; /* shared with subscribeEx.c*/

bool testSubscribeHandleEvent( /*also shared with subscribeEx.c*/
    char* label,
    int eventIndex,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusValue_t valBuff;
    rbusValue_t valIndex;
    char expectedBuff[32];
    bool pass;

    PRINT_TEST_EVENT(label, event, subscription);

    snprintf(expectedBuff, 32, "event %d data %d", eventIndex, gEventCounts[eventIndex]);

    valBuff = rbusObject_GetValue(event->data, "buffer");
    if(!valBuff)
    {
        printf("%s FAIL: value 'buffer' NULL\n", label);
        return false;
    }

    valIndex = rbusObject_GetValue(event->data, "index");
    if(!valIndex)
    {
        printf("%s FAIL: value 'index' NULL\n", label);
        return false;
    }

    pass = (strcmp(rbusValue_GetString(valBuff, NULL), expectedBuff) == 0 && 
            rbusValue_GetInt32(valIndex) == gEventCounts[eventIndex]);

    printf("%s %s: expect=[buffer:\"%s\" index:%d] actual=[buffer:\"%s\" index:%d]\n",
        label, pass ? "PASS" : "FAIL", expectedBuff, gEventCounts[eventIndex], 
        rbusValue_GetString(valBuff, NULL), rbusValue_GetInt32(valIndex));

    gEventCounts[eventIndex]++;

    return pass;
}

static void handler1(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    TALLY(testSubscribeHandleEvent("_test_Subscribe handle1", 0, event, subscription));
}

static void handler2(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    TALLY(testSubscribeHandleEvent("_test_Subscribe handle2", 1, event, subscription));
}

int getDurationSubscribe()
{
    return gDuration;
}

void testSubscribe(rbusHandle_t handle, int* countPass, int* countFail)
{
    int rc = RBUS_ERROR_SUCCESS;
    char* data[2] = { "My Data 1", "My Data2" };
    int countDown = gDuration;

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.Event1!", handler1, data[0]);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_Subscribe rbusEvent_Subscribe Event1 %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit0;

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.Event2!", handler2, data[1]);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_Subscribe rbusEvent_Subscribe Event2 %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit1;

    while(countDown > gDuration/2)
    {
        sleep(1);
        countDown--;
    }

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.Event2!");
    printf("_test_Subscribe rbusEvent_Unsubscribe Event2 %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL", rc);
    TALLY(rc == RBUS_ERROR_SUCCESS);

    while(countDown > 0)
    {
        sleep(1);
        countDown--;
    }
 
    /*test negative cases*/
    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.NonExistingEvent1!", handler1, data[0]);
    printf("_test_Subscribe rbusEvent_Subscribe NonExistingEvent1 %s rc=%d\n", rc != RBUS_ERROR_SUCCESS ? "PASS" : "FAIL", rc);
    TALLY(rc != RBUS_ERROR_SUCCESS);

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.NonExistingEvent1!");
    printf("_test_Subscribe rbusEvent_Unsubscribe NonExistingEvent1 %s rc=%d\n", rc != RBUS_ERROR_SUCCESS ? "PASS" : "FAIL", rc);
    TALLY(rc != RBUS_ERROR_SUCCESS);

exit1:
    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.Event1!");
    printf("_test_Subscribe rbusEvent_Unsubscribe Event1! %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL", rc);
    TALLY(rc == RBUS_ERROR_SUCCESS);

exit0:
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_Subscribe");
}
