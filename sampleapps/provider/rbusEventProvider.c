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

//TODO handle filter matching

int loopFor = 60;
int subscribed1 = 0;
int subscribed2 = 0;

rbus_errorCode_e eventSubHandler(
  rbus_eventSubAction_e action,
  const char* eventName, 
  rbusEventFilter_t* filter)
{
    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp("Device.Provider1.Event1!", eventName))
    {
        subscribed1 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp("Device.Provider1.Event2!", eventName))
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
    static uint32_t value = 1;
    int i = 0;

    (void)(context);
    (void)(requestingComponentName);

    //fake a value change every 3rd time the getHandler is called
    if( ((count++) % 3) == 0 )
    {
        value++;
    }

    while(i < numTlvs)
    {
        if(strcmp(tlv[i].name, "Device.Provider1.Param1") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[i].name);
            char buff[10];
            sprintf(buff, "v%d", value);
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
    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    char componentName[] = "EventProvider";
    #define numElements 3
    rbus_dataElement_t elements[numElements] = {
        {"Device.Provider1.Event1!",{NULL,NULL,NULL,eventSubHandler}},
        {"Device.Provider1.Event2!",{NULL,NULL,NULL,eventSubHandler}},
        {"Device.Provider1.Param1", {getHandler,NULL,NULL,NULL}},
    };
    char* data[2] = { "Hello Earth", "Hello Mars" };

    printf("provider: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(handle, numElements, elements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements failed: %d\n", rc);
        goto exit1;
    }

    while (loopFor != 0)
    {
        printf("provider: exiting in %d seconds\n", loopFor);
        sleep(1);
        loopFor--;

        if(subscribed1)
        {
            printf("publishing Event1\n");

            rbus_Tlv_t tlv;
            tlv.name = elements[0].elementName;
            tlv.type = RBUS_STRING;
            tlv.value = data[0];
            tlv.length = strlen(data[0]) + 1;

            rc = rbusEvent_Publish(handle, &tlv);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event1 failed: %d\n", rc);
        }
        if(subscribed2)
        {
            printf("publishing Event2\n");

            rbus_Tlv_t tlv;
            tlv.name = elements[1].elementName;
            tlv.type = RBUS_STRING;
            tlv.value = data[1];
            tlv.length = strlen(data[1]) + 1;

            rc = rbusEvent_Publish(handle, &tlv);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event2 failed: %d\n", rc);
        }
    }

    rbus_unregDataElements(handle, numElements, elements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
