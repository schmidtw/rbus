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

#define UNUSED1(a)              (void)(a)
#define UNUSED2(a,b)            UNUSED1(a),UNUSED1(b)
#define UNUSED3(a,b,c)          UNUSED1(a),UNUSED2(b,c)
#define UNUSED4(a,b,c,d)        UNUSED1(a),UNUSED3(b,c,d)
#define UNUSED5(a,b,c,d,e)      UNUSED1(a),UNUSED4(b,c,d,e)
#define UNUSED6(a,b,c,d,e,f)    UNUSED1(a),UNUSED5(b,c,d,e,f)

#define TotalParams   10

rbus_dataElement_t* elements;
rbusHandle_t        rbusHandle;
int                 loopFor = 5;
char                componentName[20] = "rbusSampleProvider";
char*               paramNames[TotalParams] = {
                                            "Device.DeviceInfo.SampleProvider.Manufacturer",
                                            "Device.DeviceInfo.SampleProvider.ModelName",
                                            "Device.DeviceInfo.SampleProvider.SoftwareVersion",
                                            "Device.SampleProvider.SampleData.IntData",
                                            "Device.SampleProvider.SampleData.BoolData",
                                            "Device.SampleProvider.SampleData.UIntData",
                                            "Device.SampleProvider.NestedObject1.TestParam",
                                            "Device.SampleProvider.NestedObject1.AnotherTestParam",
                                            "Device.SampleProvider.NestedObject2.TestParam",
                                            "Device.SampleProvider.NestedObject2.AnotherTestParam"
                                        };

typedef struct _rbus_sample_data {
    int m_intData;
    rbusBool_t m_boolData;
    unsigned int m_unsignedIntData;
} rbus_sample_provider_data_t;

rbus_sample_provider_data_t gTestInfo = {0};

rbus_errorCode_e SampleProvider_DeviceGetHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    int count = 0;
    UNUSED2(context, requestingComponentName);
    while(count < numTlvs)
    {
        if(strcmp(tlv[count].name, "Device.DeviceInfo.SampleProvider.Manufacturer") == 0)
        {
            char* mf = "COMCAST";
            printf("Called get handler for [%s]\n", tlv[count].name);
            tlv[count].type = RBUS_STRING;
            tlv[count].length = strlen(mf)+1;
            tlv[count].value = strdup(mf);
        }
        else if(strcmp(tlv[count].name, "Device.DeviceInfo.SampleProvider.ModelName") == 0)
        {
            char *mn = "XB3";
            printf("Called get handler for [%s]\n", tlv[count].name);
            tlv[count].type = RBUS_STRING;
            tlv[count].length = strlen(mn)+1;
            tlv[count].value = strdup(mn);
        }
        else if (strcmp(tlv[count].name, "Device.DeviceInfo.SampleProvider.SoftwareVersion") == 0)
        {
            float sv = 2.14;

            printf("Called get handler for [%s]\n", tlv[count].name);

            tlv[count].type = RBUS_FLOAT;
            tlv[count].length = sizeof(float);
            tlv[count].value = (void *)malloc(tlv[count].length);
            memcpy(tlv[count].value, &sv, sizeof(float));
        }
        else
        {
            printf("Cant Handle [%s]\n", tlv[count].name);
            loopFor = 0;
        }
        count++;
    }
    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e SampleProvider_SampleDataSetHandler (int numTlvs, rbus_Tlv_t* tlv, int sessionId, rbusBool_t commit, char *requestingComponentName)
{
    int count = 0;
    UNUSED2(sessionId, commit);
    while(count < numTlvs)
    {
        if(strcmp(tlv[count].name, "Device.SampleProvider.SampleData.IntData") == 0)
        {
            if (tlv[count].type != RBUS_INT32)
            {
                printf("%s Called Set handler for [%s] with wrong data type\n", requestingComponentName, tlv[count].name);
                return RBUS_ERROR_INVALID_INPUT;
            }
            else
            {
                printf("%s Called Set handler for [%s] with value = %d\n", requestingComponentName, tlv[count].name, *(int*) tlv[count].value);
                memcpy (&gTestInfo.m_intData, tlv[count].value, sizeof(int));
            }
        }
        else if(strcmp(tlv[count].name, "Device.SampleProvider.SampleData.BoolData") == 0)
        {
            if (tlv[count].type != RBUS_BOOLEAN)
            {
                printf("%s Called Set handler for [%s] with wrong data type\n", requestingComponentName, tlv[count].name);
                return RBUS_ERROR_INVALID_INPUT;
            }
            else
            {
                printf("%s Called Set handler for [%s] with value = %d\n", requestingComponentName, tlv[count].name, *(int*) tlv[count].value);
                memcpy (&gTestInfo.m_boolData, tlv[count].value, sizeof(rbusBool_t));
            }
        }
        else if (strcmp(tlv[count].name, "Device.SampleProvider.SampleData.UIntData") == 0)
        {
            if (tlv[count].type != RBUS_UINT32)
            {
                printf("%s Called Set handler for [%s] with wrong data type\n", requestingComponentName, tlv[count].name);
                return RBUS_ERROR_INVALID_INPUT;
            }
            else
            {
                printf("%s Called Set handler for [%s] with value = %u\n", requestingComponentName, tlv[count].name, *(unsigned int*) tlv[count].value);
                memcpy (&gTestInfo.m_unsignedIntData, tlv[count].value, sizeof(unsigned int));
            }
        }
        count++;
    }
    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e SampleProvider_SampleDataGetHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    int count = 0;
    UNUSED2(context, requestingComponentName);
    while(count < numTlvs)
    {
        if(strcmp(tlv[count].name, "Device.SampleProvider.SampleData.IntData") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
            tlv[count].type = RBUS_INT32;
            tlv[count].length = sizeof(int);
            tlv[count].value = (void *)malloc(tlv[count].length);
            memcpy(tlv[count].value, &gTestInfo.m_intData, tlv[count].length);

        }
        else if(strcmp(tlv[count].name, "Device.SampleProvider.SampleData.BoolData") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
            tlv[count].type = RBUS_BOOLEAN;
            tlv[count].length = sizeof(rbusBool_t);
            tlv[count].value = (void *)malloc(tlv[count].length);
            memcpy(tlv[count].value, &gTestInfo.m_boolData, tlv[count].length);
        }
        else if (strcmp(tlv[count].name, "Device.SampleProvider.SampleData.UIntData") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
            tlv[count].type = RBUS_UINT32;;
            tlv[count].length = sizeof(unsigned int);
            tlv[count].value = (void *)malloc(tlv[count].length);
            memcpy(tlv[count].value, &gTestInfo.m_unsignedIntData, tlv[count].length);
            printf("Called get handler for [%d]\n", *(unsigned int*)tlv[count].value);
        }
        else
        {
            printf("Cant Handle [%s]\n", tlv[count].name);
            loopFor = 0;
        }
        count++;
    }
    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e SampleProvider_NestedObjectsGetHandler(void *context, int numTlvs, rbus_Tlv_t* tlv, char *requestingComponentName)
{
    int count = 0;
    UNUSED2(context, requestingComponentName);

    while(count < numTlvs)
    {
        if(strcmp(tlv[count].name, "Device.SampleProvider.NestedObject1.TestParam") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
        }
        else if(strcmp(tlv[count].name, "Device.SampleProvider.NestedObject1.AnotherTestParam") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
        }
        else if (strcmp(tlv[count].name, "Device.SampleProvider.NestedObject2.TestParam") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
        }
        else if(strcmp(tlv[count].name, "Device.SampleProvider.NestedObject2.AnotherTestParam") == 0)
        {
            printf("Called get handler for [%s]\n", tlv[count].name);
        }
        else
        {
            printf("Cant Handle [%s]\n", tlv[count].name);
            loopFor = 0;
        }
        count++;
    }
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    int count = 0;
    if( argc == 2 )
    {
        printf("The argument supplied is %s\n", argv[1]);
    }

    printf("Hello from provider\n");
    int rbus_errorCode = RBUS_ERROR_BUS_ERROR;

    rbus_errorCode = rbus_open(&rbusHandle, componentName);
    printf("return code from rbus_open is %d\n", rbus_errorCode);

    elements = (rbus_dataElement_t*)malloc(sizeof(rbus_dataElement_t)*TotalParams);

    while(count < TotalParams)
    {
        elements[count].elementName = strdup(paramNames[count]);

        elements[count].table.rbus_setHandler = NULL;
        elements[count].table.rbus_updateTableHandler = NULL;
        elements[count].table.rbus_eventSubHandler = NULL;

        if(count < 3)
        {
            elements[count].table.rbus_getHandler = SampleProvider_DeviceGetHandler;
        }
        else if(count >= 3 && count < 6)
        {
            elements[count].table.rbus_getHandler = SampleProvider_SampleDataGetHandler;
            elements[count].table.rbus_setHandler = SampleProvider_SampleDataSetHandler;
        }
        else
        {
            elements[count].table.rbus_getHandler = SampleProvider_NestedObjectsGetHandler;
        }

        count++;
    }

    rbus_regDataElements(rbusHandle, TotalParams, elements);

    //Wait here....
    while (1)
    {
        sleep(5);
    }

    rbus_unregDataElements(rbusHandle, TotalParams, elements);

    count = 0;
    //To DO: Call rebus unregister data elements here...
    while(count < TotalParams)
    {
        free(elements[count].elementName);
        count++;
    }
    free(elements);

    rbus_errorCode = rbus_close(rbusHandle);
    printf("return code from  rbus_close is %d\n", rbus_errorCode);
    return 0;
}
