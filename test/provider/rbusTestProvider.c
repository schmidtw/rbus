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
#include "../common/runningParamHelper.h"

//TODO handle filter matching

int subscribed1 = 0;
int subscribed2 = 0;

rbus_errorCode_e eventSubHandler(
  rbus_eventSubAction_e action,
  const char* eventName, 
  rbusEventFilter_t* filter)
{
    (void)(filter);

    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp("Device.TestProvider.Event1!", eventName))
    {
        subscribed1 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp("Device.TestProvider.Event2!", eventName))
    {
        subscribed2 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e getHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    static uint32_t count = 0;
    static uint32_t value = 0;
    int i = 0;
    static char buff[100];

    (void)(context);
    (void)(requestingComponentName);

    snprintf(buff, 100, "value %d", value);

    //fake a value change every 3rd time the getHandler is called
    if( ((count++) % 3) == 0 )
    {
        printf("test:value-change=%s\n", buff);
        value++;
    }

    while(i < numTlvs)
    {
        if(strcmp(tlv[i].name, "Device.TestProvider.Param1") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[i].name);
            tlv[i].type = RBUS_STRING;
            tlv[i].length = strlen(buff)+1;
            tlv[i].value = (void *)malloc(tlv[i].length);
            memcpy(tlv[i].value, buff, tlv[i].length);
        }
        i++;
    }
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    char componentName[] = "TestProvider";
    int dataVal=0;

    #define numElements 3

    rbus_dataElement_t elements[numElements] = {
        {"Device.TestProvider.Event1!",{NULL,NULL,NULL,eventSubHandler}},
        {"Device.TestProvider.Event2!",{NULL,NULL,NULL,eventSubHandler}},
        {"Device.TestProvider.Param1", {getHandler,NULL,NULL,NULL}}
    };

    printf("provider: start\n");

    rc = rbus_open(&handle, componentName);
    printf("provider: rbus_open=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit2;

    rc = rbus_regDataElements(handle, numElements, elements);
    printf("provider: rbus_regDataElements=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit1;

    if(runningParamProvider_Init(handle, "Device.TestProvider.TestRunning") != RBUS_ERROR_SUCCESS)
    {
        printf("provider: failed to register running param\n");
        goto exit1;
    }
    printf("provider: waiting for consumer to start\n");
    while (!runningParamProvider_IsRunning())
    {
        usleep(10000);
    }

    printf("provider: running\n");
    while (runningParamProvider_IsRunning())
    {
        #define BUFF_LEN 100
        char buffer[BUFF_LEN];
        dataVal++;

        sleep(1);

        if(subscribed1)
        {
            printf("publishing Event1\n");

            snprintf(buffer, BUFF_LEN, "event 1 data %d\n", dataVal);

            rbus_Tlv_t tlv;
            tlv.name = elements[0].elementName;
            tlv.type = RBUS_STRING;
            tlv.value = buffer;
            tlv.length = strlen(tlv.value) + 1;

            rc = rbusEvent_Publish(handle, &tlv);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event1 failed: %d\n", rc);
        }

        if(subscribed2)
        {
            printf("publishing Event2\n");

            snprintf(buffer, BUFF_LEN, "event 2 data %d\n", dataVal);

            rbus_Tlv_t tlv;
            tlv.name = elements[1].elementName;
            tlv.type = RBUS_STRING;
            tlv.value = buffer;
            tlv.length = strlen(tlv.value) + 1;

            rc = rbusEvent_Publish(handle, &tlv);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event2 failed: %d\n", rc);
        }
    }

    printf("provider: finishing\n");

    rc = rbus_unregDataElements(handle, numElements, elements);
    printf("provider: rbus_unregDataElements=%d\n", rc);

exit1:
    rc = rbus_close(handle);
    printf("provider: rbus_close=%d\n", rc);

exit2:
    printf("provider: exit\n");
    return rc;
}
