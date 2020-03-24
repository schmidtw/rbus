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

static int duration = 10;

void rbusValueChange_SetPollingPeriod(int seconds);

static void paramValueChangeHandler(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    (void)(handle);

    printf(
        "paramValueChange called:\n"
        "\tevent=%s\n"
        "\tvalue=%s\n"
        "\ttype=%d\n"
        "\tlength=%d\n"
        "\tdata=%p\n",
        eventData->name,
        (char*)eventData->value,
        eventData->length,
        eventData->type,
        subscription->user_data);

    printf("_test_ValueChange_handler=%s\n", (char*)eventData->value);
}

int getDurationValueChange()
{
    return duration;
}

int testValueChange(rbusHandle_t handle)
{
    int rc = RBUS_ERROR_SUCCESS;

    rbusValueChange_SetPollingPeriod(1);

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.Param1", paramValueChangeHandler, NULL);
    printf("_test_ValueChange_rbusEvent_Subscribe=%d\n", rc);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        return 1;
    }

    sleep(duration);

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.Param1");
    printf("_test_ValueChange_rbusEvent_Unsubscribe=%d\n", rc);

    return 0;
}

