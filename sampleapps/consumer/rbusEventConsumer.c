/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
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

#define TEST_VALUE_CHANGE   1
#define TEST_SUBSCRIBE      1
#define TEST_SUBSCRIBE_EX   1

int loopFor = 20;

void eventReceiveHandler1(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    printf(
        "eventReceiveHandler1 called:\n"
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
}

void eventReceiveHandler2(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    printf(
        "eventReceiveHandler2 called:\n"
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
}

void eventReceiveHandler3(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    printf(
        "eventReceiveHandler3 called:\n"
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
}

int main(int argc, char *argv[])
{
    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    char* data[2] = { "My Data 1", "My Data2" };
    rbusEventSubscription_t subscriptions[2] = {
        {"Device.Provider1.Event1!", NULL, 0, eventReceiveHandler1, data[0], 0},
        {"Device.Provider1.Event2!", NULL, 0, eventReceiveHandler2, data[1], 0}
    };

    printf("constumer: start\n");

    rc = rbus_open(&handle, "EventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit4;
    }

#if TEST_VALUE_CHANGE
#if 0
    rbus_Tlv_t tlv;
    tlv.name = "Device.Provider1.Param1";
    if(rbus_get(handle, &tlv) == RBUS_ERROR_SUCCESS)
    {
        if(tlv.type == RBUS_STRING)
        {
            printf("get %s: %s\n", tlv.name, (char*)tlv.value);
        }
    }
#endif

    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Param1",
        eventReceiveHandler3,
        NULL);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        goto exit3;
    }

    sleep(loopFor);
    
#endif

#if TEST_SUBSCRIBE
    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Event1!",
        eventReceiveHandler1,
        data[0]);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit3;
    }

    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Event2!",
        eventReceiveHandler2,
        data[1]);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 2 failed: %d\n", rc);
        goto exit2;
    }

    sleep(loopFor/2);
    printf("Unsubscribing from Event2\n");
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event2!");
    sleep(loopFor/2);
    printf("Unsubscribing from Event1\n");
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event1!");
#endif

#if TEST_SUBSCRIBE_EX
    rc = rbusEvent_SubscribeEx(handle, subscriptions, 2);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit3;
    }

    sleep(loopFor);

    rbusEvent_UnsubscribeEx(handle, subscriptions, 2);
#endif
    goto exit3;

exit2:
#if TEST_SUBSCRIBE
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event1");
#endif

exit3:
    rbus_close(handle);

exit4:
    printf("consumer: exit\n");
    return rc;
}


