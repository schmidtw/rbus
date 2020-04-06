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

int runtime = 24;

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");

    printf("Consumer receiver ValueChange event for param %s\n", event->name);

    if(newValue)
        printf("  New Value: %d\n", rbusValue_GetInt32(newValue));

    if(oldValue)
        printf("  Old Value: %d\n", rbusValue_GetInt32(oldValue));

    printf("My user data: %s\n", (char*)subscription->userData);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;

    rc = rbus_open(&handle, "EventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }

    // subscribe to ValueChanged on "Device.Provider1.Param1"
    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Param1",
        eventReceiveHandler,
        "My User Data");

    sleep(runtime);

    rbus_close(handle);
    return rc;
}


