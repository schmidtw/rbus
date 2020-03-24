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

#define     TotalParams   10

char            componentName[20] = "rbusSampleConsumer";
rbusHandle_t    handle;
char*           paramNames[TotalParams] = {
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

int main(int argc, char *argv[])
{
    int count = 0;

    if( argc == 2 ) {
        printf("The argument supplied is %s\n", argv[1]);
    }

    printf("Hello from consumer\n");
    int rbus_errorCode = RBUS_ERROR_BUS_ERROR;

    rbus_errorCode = rbus_open(&handle, componentName);
    printf("return code from rbus_open is %d\n", rbus_errorCode);

    while(count < TotalParams)
    {
        rbus_Tlv_t tlv;
        tlv.length = 0;
        tlv.value = NULL;

        tlv.name = strdup(paramNames[count]);

        switch(count)
        {
            case 0:
                //Manufacturer
                tlv.type = RBUS_STRING;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                if(tlv.value)
                {
                    printf("Manufacturer name = [%s]\n", (char*)tlv.value);
                    free(tlv.value);
                }
                break;

            case 1:
                //Model Name
                tlv.type = RBUS_STRING;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                if(tlv.value)
                {
                    printf("Model name = [%s]\n", (char*)tlv.value);
                    free(tlv.value);
                }
                break;

            case 2:
                //Software Version
                tlv.type = RBUS_FLOAT;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                if(tlv.value)
                {
                    printf("Software Version  = [%f]\n", *(float*)tlv.value);
                    free(tlv.value);
                }
                break;
            case 3:
                //Int Data
                tlv.type = RBUS_INT32;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                if(tlv.value)
                {
                    printf("The value is = [%d]\n", *(int*)tlv.value);
                    free(tlv.value);
                }
                /**************************/
                {
                    int tmp = 250;
                    tlv.type = RBUS_INT32;
                    printf("calling rbus set for [%s] with int value %d\n", tlv.name, tmp);
                    tlv.length = sizeof(tmp);
                    tlv.value = &tmp;
                    rbus_errorCode = rbus_set(handle, &tlv, 0, RBUS_TRUE);
                    if (rbus_errorCode != RBUS_ERROR_SUCCESS)
                        printf ("Set Failed for %s with error code as %d\n", tlv.name, rbus_errorCode);
                }
                /* -ve Test. it will fail */
                {
                    char *tmp = "2020";
                    tlv.type = RBUS_STRING;
                    printf("calling rbus set for [%s] with string value: %s\n", tlv.name, tmp);
                    tlv.length = strlen(tmp) + 1;
                    tlv.value = tmp;
                    rbus_errorCode = rbus_set(handle, &tlv, 0, RBUS_TRUE);
                    if (rbus_errorCode != RBUS_ERROR_SUCCESS)
                        printf ("Set Failed for %s with error code as %d\n", tlv.name, rbus_errorCode);
                }
                {
                    tlv.value = NULL;
                    printf("calling rbus get for [%s]\n", tlv.name);
                    rbus_get(handle, &tlv);
                    if(tlv.value)
                    {
                        printf("The value is = [%d]\n", *(int*)tlv.value);
                        free(tlv.value);
                    }
                }
                break;

            case 4:
                //Bool Data
                tlv.type = RBUS_BOOLEAN;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                if(tlv.value)
                {
                    printf("The value is = [%d]\n", *(rbusBool_t*)tlv.value);
                    free(tlv.value);
                }
                /**************************/
                {
                    rbusBool_t tmp = RBUS_TRUE;
                    tlv.type = RBUS_BOOLEAN;
                    printf("calling rbus set for [%s] with bool value %d\n", tlv.name, tmp);
                    tlv.length = sizeof(tmp);
                    tlv.value = &tmp;
                    rbus_errorCode = rbus_set(handle, &tlv, 0, RBUS_TRUE);
                    if (rbus_errorCode != RBUS_ERROR_SUCCESS)
                        printf ("Set Failed for %s with error code as %d\n", tlv.name, rbus_errorCode);
                }
                /* -ve Test. it will fail */
                {
                    char *tmp = "TRUE";
                    tlv.type = RBUS_STRING;
                    printf("calling rbus set for [%s] with string value: %s\n", tlv.name, tmp);
                    tlv.length = strlen(tmp) + 1;
                    tlv.value = tmp;
                    rbus_errorCode = rbus_set(handle, &tlv, 0, RBUS_TRUE);
                    if (rbus_errorCode != RBUS_ERROR_SUCCESS)
                        printf ("Set Failed for %s with error code as %d\n", tlv.name, rbus_errorCode);
                }
                {
                    tlv.value = NULL;
                    printf("calling rbus get for [%s]\n", tlv.name);
                    rbus_get(handle, &tlv);
                    if(tlv.value)
                    {
                        printf("The value is = [%d]\n", *(rbusBool_t*)tlv.value);
                        free(tlv.value);
                    }
                }
                break;

            case 5:
                //UInt Data
                tlv.type = RBUS_UINT32;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                if(tlv.value)
                {
                    printf("The value is = [%d]\n", *(unsigned int*)tlv.value);
                    free(tlv.value);
                }
                /**************************/
                {
                    unsigned int tmp = 0xFFFFFFFF;
                    tlv.type = RBUS_UINT32;
                    printf("calling rbus set for [%s] with uint32 value %u\n", tlv.name, tmp);
                    tlv.length = sizeof(tmp);
                    tlv.value = &tmp;
                    rbus_errorCode = rbus_set(handle, &tlv, 0, RBUS_TRUE);
                    if (rbus_errorCode != RBUS_ERROR_SUCCESS)
                        printf ("Set Failed for %s with error code as %d\n", tlv.name, rbus_errorCode);
                }
                /* -ve Test. it will fail */
                {
                    char *tmp = "25625";
                    tlv.type = RBUS_STRING;
                    printf("calling rbus set for [%s] with string value: %s\n", tlv.name, tmp);
                    tlv.length = strlen(tmp) + 1;
                    tlv.value = tmp;
                    rbus_errorCode = rbus_set(handle, &tlv, 0, RBUS_TRUE);
                    if (rbus_errorCode != RBUS_ERROR_SUCCESS)
                        printf ("Set Failed for %s with error code as %d\n", tlv.name, rbus_errorCode);
                }
                {
                    tlv.value = NULL;
                    printf("calling rbus get for [%s]\n", tlv.name);
                    rbus_get(handle, &tlv);
                    if(tlv.value)
                    {
                        printf("The value is = [%d]\n", *(unsigned int*)tlv.value);
                        free(tlv.value);
                    }
                }

                break;

                /*
                case 6:
                //Obj1-TestParam
                tlv.type = RBUS_STRING;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                break;

                case 7:
                //Obj1-AnotherTestParam
                tlv.type = RBUS_STRING;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                break;

                case 8:
                //Obj2-TestParam
                tlv.type = RBUS_STRING;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                break;

                case 9:
                //Obj2-AnotherTestParam
                tlv.type = RBUS_STRING;
                printf("calling rbus get for [%s]\n", tlv.name);
                rbus_get(handle, &tlv);
                break;
                 */
            default:
                break;
        }

        free(tlv.name);
        count++;
    }
    int numElements = 0;
    char** elementNames;
    rbus_errorCode_e rc;
    int i;
    rc =  rbus_discoverComponentDataElements(handle,"rbusSampleProvider",RBUS_FALSE,&numElements,&elementNames);
    if(RBUS_ERROR_SUCCESS == rc)
    {
        printf ("Discovered elements are,\n");
        for(i=0;i<numElements;i++)
        {
            printf("rbus_discoverComponentDataElements %d: %s\n", i,elementNames[i]);
            free(elementNames[i]);
        }
        free(elementNames);
    }
    else
    {
        printf ("Failed to discover element array. Error Code = %s\n", "");
    }
    char* elementNames1[] = {"Device.DeviceInfo.SampleProvider.Manufacturer","Device.DeviceInfo.SampleProvider.ModelName","Device.DeviceInfo.SampleProvider.SoftwareVersion","Device.SampleProvider.SampleData.IntData","Device.SampleProvider.SampleData.BoolData","Device.SampleProvider.SampleData.UIntData","Device.SampleProvider.NestedObject1.TestParam","Device.SampleProvider.NestedObject1.AnotherTestParam","Device.SampleProvider.NestedObject2.TestParam","Device.SampleProvider.NestedObject2.AnotherTestParam"};
    int numComponents = 0;
    char **componentName;
    rc = rbus_discoverComponentName(handle,10,elementNames1,&numComponents,&componentName);
    if(RBUS_ERROR_SUCCESS == rc)
    {
        printf ("Discovered components are,\n");
        for(i=0;i<numComponents;i++)
        {
            printf("rbus_discoverComponentName %s: %s\n", elementNames1[i],componentName[i]);
            free(componentName[i]);
        }
        free(componentName);
    }
    else
    {
        printf ("Failed to discover component array. Error Code = %s\n", "");
    }

    rbus_errorCode = rbus_close(handle);
    printf("return code from rbus_close is %d\n", rbus_errorCode);
    return 0;
}


