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
#include "../common/testValueHelper.h"
#include "../common/test_macros.h"

int getDurationValue()
{
    return 1;
}
void printSpace(int depth)
{
    while(depth--)printf("  ");
}

void getAllValues(rbusHandle_t handle, TestValueProperty* properties, int index)
{
    int rc = RBUS_ERROR_SUCCESS;
    rbusValue_t value;
    TestValueProperty* data;

    printf("#################### getAllValue ######################\n");

    data = properties;
    while(data->name)
    {
        printf("_test_Value rbus_get %s\n", data->name);

        rc = rbus_get(handle, data->name, &value);

        if(rc ==  RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");

            if( rbusValue_Compare(value, data->values[index]) != 0)
            {
                char* s1 = rbusValue_ToDebugString(data->values[index], NULL, 0);
                char* s2 = rbusValue_ToDebugString(value, NULL, 0);
                printf(">>>>>>>>>>>>>>>>>>>>>>>> test fail %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", data->name);
                printf("_test_Value value compare fail: expected:[%s] got:[%s]\n", s1, s2);
                TALLY(0);
            }
            else
            {
                printf(">>>>>>>>>>>>>>>>>>>>>>>> test success %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", data->name);
                TALLY(1);
            }

            rbusValue_Release(value);
        }
        else
        {
            printf("_test_Value rbus_get failed err:%d\n", rc); 
            TALLY(0);
        }

        data++;
    }
}

void setAllValues(rbusHandle_t handle, TestValueProperty* properties, int index)
{
    int rc = RBUS_ERROR_SUCCESS;
    TestValueProperty* data;

    printf("#################### setAllValues ######################\n");

    data = properties;
    while(data->name)
    {
        printf("_test_Value rbus_set %s\n", data->name);

        rc = rbus_set(handle, data->name, data->values[index], NULL);

        if(rc !=  RBUS_ERROR_SUCCESS)
        {
            printf("_test_Value rbus_get failed err:%d\n", rc); 
            TALLY(0);
        }

        data++;
    }
}

void testValue(rbusHandle_t handle, int* countPass, int* countFail)
{
    TestValueProperty* properties;
    TestValueProperties_Init(&properties);
    getAllValues(handle, properties, 0);
    setAllValues(handle, properties, 1);
    getAllValues(handle, properties, 1);
    setAllValues(handle, properties, 2);
    getAllValues(handle, properties, 2);
    TestValueProperties_Release(properties);
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("_test_Value");
}

