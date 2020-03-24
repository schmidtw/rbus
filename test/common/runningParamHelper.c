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

#define MAXPATH 100
static char gParamName[MAXPATH+1] = {0};
static rbusBool_t gIsRunning = RBUS_FALSE;

static rbus_errorCode_e getRunningParamHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    int i = 0;
    (void)(context);
    (void)(requestingComponentName);

    while(i < numTlvs)
    {
        if(strcmp(tlv[i].name, gParamName) == 0)
        {
            tlv[i].type = RBUS_BOOLEAN;
            tlv[i].length = sizeof(rbusBool_t);
            tlv[i].value = (void *)malloc(tlv[i].length);
            memcpy(tlv[i].value, &gIsRunning, tlv[i].length);
        }
        i++;
    }
    return RBUS_ERROR_SUCCESS;
}

static rbus_errorCode_e setRunningParamHandler(int numTlvs, rbus_Tlv_t* tlv, int sessionId, rbusBool_t commit, char *requestingComponentName)
{
    int i = 0;
    (void)(sessionId);
    (void)(commit);
    (void)(requestingComponentName);

    while(i < numTlvs)
    {
        if(strcmp(tlv[i].name, gParamName) == 0)
        {
            if(tlv[i].type == RBUS_BOOLEAN)
            {
                memcpy(&gIsRunning, tlv[i].value, sizeof(gIsRunning));
                printf("_test_:setRunningParamHandler result:SUCCESS value:%s\n", gIsRunning ? "TRUE" : "FALSE");
            }
            else
            {
                printf("_test_:setRunningParamHandler result:FAIL msg:'unexpected type %d'\n", tlv[i].type);
            }
        }
        i++;
    }

    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e runningParamProvider_Init(rbusHandle_t handle, char* paramName)
{
    int rc;
    rbus_dataElement_t el = {paramName, {getRunningParamHandler,setRunningParamHandler,NULL,NULL}};

    strncpy(gParamName, paramName, MAXPATH);

    rc = rbus_regDataElements(handle, 1, &el);
    if(rc == RBUS_ERROR_SUCCESS)
    {
        printf("_test_:runningParamProvider_Init result:SUCCESS\n");
    }
    else
    {
        printf("_test_:runningParamProvider_Init result:FAIL msg:'rbus_regDataElements returned %d'\n", rc);
    }
    return rc;
}

int runningParamProvider_IsRunning()
{
    return gIsRunning;
}

int runningParamConsumer_Set(rbusHandle_t handle, char* paramName, rbusBool_t running)
{
    rbus_Tlv_t runTlv;
    runTlv.name = paramName;
    runTlv.type = RBUS_BOOLEAN;
    runTlv.length = sizeof(rbusBool_t);
    runTlv.value = &running;
    int i;
    int loopFor = 1;

    if(running)
        loopFor = 5;/*give provider time to get ready*/

    for(i=0; i<loopFor; ++i)
    {
        int rc;
        rc = rbus_set(handle, &runTlv, 0, RBUS_FALSE);
        printf("runningParamConsumer_Set rbus_set=%d\n", rc);
        if(rc == RBUS_ERROR_SUCCESS)
            break;
        if(i<loopFor-1)
            sleep(1);
    }
    if(i==loopFor)
    {
        printf("_test_:runningParamConsumer_Set result:FAIL\n");
        return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        printf("_test_:runningParamConsumer_Set result:SUCCESS\n");
        return RBUS_ERROR_SUCCESS;
    }
}
