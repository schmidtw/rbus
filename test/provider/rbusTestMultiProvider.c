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

rbusBool_t testRunning = RBUS_FALSE;
int subscribes[5] = {-1,-1,-1,-1,-1};

rbus_errorCode_e getTestRunningHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    int i = 0;
    (void)(context);
    (void)(requestingComponentName);

    while(i < numTlvs)
    {
        if(strcmp(tlv[i].name, "Device.TestProvider.TestRunning") == 0)
        {
            tlv[i].type = RBUS_BOOLEAN;
            tlv[i].length = sizeof(rbusBool_t);
            tlv[i].value = (void *)malloc(tlv[i].length);
            memcpy(tlv[i].value, &testRunning, tlv[i].length);
        }
        i++;
    }
    return RBUS_ERROR_SUCCESS;
}
rbus_errorCode_e setTestRunningHandler(int numTlvs, rbus_Tlv_t* tlv, int sessionId, rbusBool_t commit, char *requestingComponentName)
{
    int i = 0;
    (void)(sessionId);
    (void)(commit);
    (void)(requestingComponentName);

    while(i < numTlvs)
    {
        if(strcmp(tlv[i].name, "Device.TestProvider.TestRunning") == 0)
        {
            if(tlv[i].type == RBUS_BOOLEAN)
            {
                memcpy(&testRunning, tlv[i].value, sizeof(testRunning));
                printf("provider:TestRunning set %s\n", testRunning ? "TRUE" : "FALSE");
            }
            else
            {
                printf("provider:setTestRunningHandler error: expected RBUS_BOOL but got type %d\n", tlv[i].type);
            }
        }
        i++;
    }

    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e getHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    int i = 0;
    int found = 0;
    static int counter = 0;

    (void)(context);
    (void)(requestingComponentName);

    while(i < numTlvs)
    {
        if( strcmp(tlv[i].name, "Device.MultiProvider1.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider2.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider3.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider4.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider5.Param") == 0)
        {
            char value[256];
            snprintf(value, 256, "param=%s caller=%s counter=%d", tlv[i].name, requestingComponentName, ++counter);
            tlv[i].type = RBUS_STRING;
            tlv[i].length = strlen(value)+1;
            tlv[i].value = strdup(value);

            found = 1;
            printf("_test_:getHandler result:SUCCESS value:'%s'\n", value);
        }
        else
        {
            printf("_test_:getHandler result:FAIL error:'unexpected param' param:'%s' caller:%s\n", tlv[i].name, requestingComponentName);
        }
        i++;
    }

    if(!found)
    {
        printf("_test_:getHandler result:FAIL error:'no param matched' caller:%s\n", requestingComponentName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e setHandler(int numTlvs, rbus_Tlv_t* tlv, int sessionId, rbusBool_t commit, char *requestingComponentName)
{
    int i = 0;
    int found = 0;

    (void)(sessionId);
    (void)(commit);
    (void)(requestingComponentName);

    while(i < numTlvs)
    {
        if( strcmp(tlv[i].name, "Device.MultiProvider1.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider2.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider3.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider4.Param") == 0 ||
            strcmp(tlv[i].name, "Device.MultiProvider5.Param") == 0)
        {
            if(tlv[i].type == RBUS_STRING)
            {
                printf("_test_:setHandler result:SUCCESS param='%s' value='%s' caller=%s\n", tlv[i].name, (char*)tlv[i].value, requestingComponentName);
            }
            else
            {
                printf("_test_:setHandler result:FAIL error:'unexpected type %d' caller=%s \n", tlv[i].type, requestingComponentName);
            }
            found = 1;
        }
        i++;
    }

    if(!found)
    {
        printf("_test_:getHandler result:FAIL error:'no param matched' caller=%s\n", requestingComponentName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e eventSubHandler(
  rbus_eventSubAction_e action,
  const char* eventName, 
  rbusEventFilter_t* filter)
{
    (void)(filter);

    if(strcmp(eventName, "Device.MultiProvider1.Event!") == 0)
    {   
        subscribes[0] = action;
    }
    else
    if(strcmp(eventName, "Device.MultiProvider2.Event!") == 0)
    {
        subscribes[1] = action;
    }
    else
    if(strcmp(eventName, "Device.MultiProvider3.Event!") == 0)
    {
        subscribes[2] = action;
    }
    else
    if(strcmp(eventName, "Device.MultiProvider4.Event!") == 0)
    {
        subscribes[3] = action;
    }
    else
    if(strcmp(eventName, "Device.MultiProvider5.Event!") == 0)
    {
        subscribes[4] = action;
    }

    printf("_test_:eventSubHandler event=%s action=%s\n", eventName, action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe");

    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handles[5] = {NULL};

    /*create 5 components which act as provider for 1 param and 1 event each*/
    char* componentNames[] = {
        "MultiProvider1",
        "MultiProvider2",
        "MultiProvider3",
        "MultiProvider4",
        "MultiProvider5"
    };

    rbus_dataElement_t elements[5][2] = {
        {   {"Device.MultiProvider1.Param", {getHandler,setHandler,NULL,NULL}},
            {"Device.MultiProvider1.Event!",{NULL,NULL,NULL,eventSubHandler}}
        },
        {   {"Device.MultiProvider2.Param", {getHandler,setHandler,NULL,NULL}},
            {"Device.MultiProvider2.Event!",{NULL,NULL,NULL,eventSubHandler}}
        },
        {   {"Device.MultiProvider3.Param", {getHandler,setHandler,NULL,NULL}},
            {"Device.MultiProvider3.Event!",{NULL,NULL,NULL,eventSubHandler}}
        },
        {   {"Device.MultiProvider4.Param", {getHandler,setHandler,NULL,NULL}},
            {"Device.MultiProvider4.Event!",{NULL,NULL,NULL,eventSubHandler}}
        },
        {   {"Device.MultiProvider5.Param", {getHandler,setHandler,NULL,NULL}},
            {"Device.MultiProvider5.Event!",{NULL,NULL,NULL,eventSubHandler}}
        }
    };

    int rc = RBUS_ERROR_SUCCESS;
    int i;
    int loopFor = 20;
    int eventCount = 0;

    printf("provider: opening 5 handles\n");

    for(i = 0; i < 5; ++i)
    {
        rc = rbus_open(&handles[i], componentNames[i]);
        printf("provider: rbus_open %s=%d\n", componentNames[i], rc);
        if(rc == RBUS_ERROR_SUCCESS)
        {
            printf("_test_:rbus_open result:SUCCESS component:%s\n", componentNames[i]);
        }
        else
        {
            printf("_test_:rbus_open result:FAIL component:%s rc:%d\n", componentNames[i], rc);
            handles[i] = NULL;
            if(i == 0)
            {
                printf("provider: failed to open 1st handle\n");
                return 1;
            }
            else
            {
                continue;
            }
        }

        rc = rbus_regDataElements(handles[i], 2, elements[i]);
        printf("provider: rbus_regDataElements %s=%d\n", componentNames[i], rc);
        if(rc == RBUS_ERROR_SUCCESS)
        {
            printf("_test_:rbus_regDataElements result:SUCCESS component:%s\n", componentNames[i]);
        }
        else
        {
            printf("_test_:rbus_regDataElements result:FAIL component:%s rc:%d\n", componentNames[i], rc);
        }
    }

    if(runningParamProvider_Init(handles[0], "Device.MultiProvider.TestRunning") != RBUS_ERROR_SUCCESS)
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
        char buffer[64];
        eventCount++;
        sleep(1);

        printf("provider: publishing events for 5 handles\n");
        for(i = 0; i < 5; ++i)
        {
            if(handles[i])
            {
                snprintf(buffer, 64, "from=%s count=%d\n", componentNames[i], eventCount);

                rbus_Tlv_t tlv;
                tlv.name = elements[i][1].elementName;
                tlv.type = RBUS_STRING;
                tlv.value = buffer;
                tlv.length = strlen(tlv.value) + 1;

                rc = rbusEvent_Publish(handles[i], &tlv);
                printf("provider: rbusEvent_Publish %s=%d\n", elements[i][1].elementName, rc);
                if(rc == RBUS_ERROR_SUCCESS)
                {
                    printf("_test_:rbusEvent_Publish result:SUCCESS event:%s\n", elements[i][1].elementName);
                }
                else
                {
                    printf("_test_:rbusEvent_Publish result:FAIL event:%s rc=%d\n", elements[i][1].elementName, rc);
                }
            }
        }
    }

exit1:

    printf("provider: closing 5 handles\n");

    for(i = 0; i < 5; ++i)
    {
        if(handles[i])
        {
            rc = rbus_unregDataElements(handles[i], 2, elements[i]);
            printf("provider: rbus_unregDataElements %s=%d\n", componentNames[i], rc);
            if(rc == RBUS_ERROR_SUCCESS)
            {
                printf("_test_:rbus_unregDataElements result:SUCCESS component:%s\n", componentNames[i]);
            }
            else
            {
                printf("_test_:rbus_unregDataElements result:FAIL component:%s rc:%d\n", componentNames[i], rc);
            }

            rc = rbus_close(handles[i]);
            printf("provider: rbus_close %s=%d\n", componentNames[i], rc);
            if(rc == RBUS_ERROR_SUCCESS)
            {
                printf("_test_:rbus_close result:SUCCESS component:%s\n", componentNames[i]);
            }
            else
            {
                printf("_test_:rbus_close result:FAIL component:%s rc:%d\n", componentNames[i], rc);
            }
        }
    }

    return rc;
}
