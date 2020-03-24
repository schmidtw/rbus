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
static int count1 = 0;
static int count2 = 0;

static void handler1(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    (void)(handle);
    printf(
        "handler1 called:\n"
        "\tevent=%s\n"
        "\tvalue=%s\n"
        "\ttype=%d\n"
        "\tlength=%d\n"
        "\tdata=%s\n",
        eventData->name,
        (char*)eventData->value,
        eventData->length,
        eventData->type,
        (char*)subscription->user_data);
    printf("_test_Subscribe_handler1=%s\n", (char*)eventData->value);
    count1++;
}

static void handler2(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    (void)(handle);
    printf(
        "handler2 called:\n"
        "\tevent=%s\n"
        "\tvalue=%s\n"
        "\ttype=%d\n"
        "\tlength=%d\n"
        "\tdata=%s\n",
        eventData->name,
        (char*)eventData->value,
        eventData->length,
        eventData->type,
        (char*)subscription->user_data);
    printf("_test_Subscribe_handler2=%s\n", (char*)eventData->value);
    count2++;
}

int getDurationSubscribe()
{
    return duration;
}

int testSubscribe(rbusHandle_t handle)
{
    int rc = RBUS_ERROR_SUCCESS;
    char* data[2] = { "My Data 1", "My Data2" };
    int countDown = duration;

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.Event1!", handler1, data[0]);
    printf("_test_Subscribe_rbusEvent_Subscribe_1=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        return 1;
    }

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.Event2!", handler2, data[1]);
    printf("_test_Subscribe_rbusEvent_Subscribe_2=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 2 failed: %d\n", rc);
        goto exit1;
    }

    while(countDown > duration/2)
    {
        sleep(1);
        countDown--;
    }

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.Event2!");
    printf("_test_Subscribe_rbusEvent_Unsubscribe_1=%d\n", rc);

    while(countDown > 0)
    {
        sleep(1);
        countDown--;
    }
 
    rbusEvent_Unsubscribe(handle, "Device.TestProvider.Event1!");
    printf("_test_Subscribe_rbusEvent_Unsubscribe_2=%d\n", rc);

    /*test negative cases*/
    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.NonExistingEvent1!", handler1, data[0]);
    printf("_test_Subscribe_rbusEvent_Subscribe_noexist=%d\n", rc);

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.NonExistingEvent1!");
    printf("_test_Subscribe_rbusEvent_Unsubscribe_noexist=%d\n", rc);

    return 0;

exit1:
    rbusEvent_Unsubscribe(handle, "Device.TestProvider.Event1");
    return 1;
}
