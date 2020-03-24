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

static int count1 = 0;
static int count2 = 0;
static int duration = 10;

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
    printf("_test_SubscribeEx_handler1=%s\n", (char*)eventData->value);
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
    printf("_test_SubscribeEx_handler2=%s\n", (char*)eventData->value);
}

int getDurationSubscribeEx()
{
    return duration;
}

int testSubscribeEx(rbusHandle_t handle)
{
    int rc = RBUS_ERROR_SUCCESS;
    char* data[2] = { "My Data 1", "My Data2" };

    rbusEventSubscription_t subscriptions[2] = {
        {"Device.TestProvider.Event1!", NULL, 0, handler1, data[0], 0},
        {"Device.TestProvider.Event2!", NULL, 0, handler2, data[1], 0}
    };

    count1 = count2 = 0;

    rc = rbusEvent_SubscribeEx(handle, subscriptions, 2);
    printf("_test_SubscribeEx_rbusEvent_SubscribeEx=%d\n", rc);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer %s: rbusEvent_SubscribeEx failed: %d\n", __FUNCTION__, rc);
        return 1;
    }

    sleep(duration);

    rc = rbusEvent_UnsubscribeEx(handle, subscriptions, 2);
    printf("_test_SubscribeEx_rbusEvent_UnsubscribeEx=%d\n", rc);

    return 0;
}
