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

int getDurationSubscribe();
int getDurationSubscribeEx();
int getDurationValueChange();

int testSubscribe(rbusHandle_t handle);
int testSubscribeEx(rbusHandle_t handle);
int testValueChange(rbusHandle_t handle);

typedef int (*getDurationFunc_t)();
typedef int (*runTestFunc_t)(rbusHandle_t handle);

typedef struct testInfo_t
{
    int enabled;
    char name[50];
    getDurationFunc_t getDuration;
    runTestFunc_t runTest;
}testInfo_t;

typedef enum testType_t
{
    TestSubscribe,
    TestSubscribeEx,
    TestValueChange,
    TestTypeMax
}testType_t;

testInfo_t testList[TestTypeMax] = {
    { 0, "Subscribe", getDurationSubscribe, testSubscribe },
    { 0, "SubscribeEx", getDurationSubscribeEx, testSubscribeEx },
    { 0, "ValueChange", getDurationValueChange, testValueChange }
};

void printUsage()
{
    int i;
    printf("rbusTestConsumer [OPTIONS] [TESTS]\n");
    printf(" OPTIONS:\n");
    printf(" -d : get the estimated duration in seconds the test will take (must include desired test flags)\n");
    printf(" -a : run all tests (ignores indiviual test flags)\n");
    printf(" TESTS: (if -a is not set then specify which tests to run)\n");
    for(i=0; i<TestTypeMax; ++i)
    {
        printf(" --%s\n", testList[i].name);
    }
}

int main(int argc, char *argv[])
{
    int i,a;
    int duration = -1;

    for(a=1; a<argc; ++a)
    {
        if(strcmp(argv[a], "--help")==0)
        {
            printUsage();
            return 0;
        }

        /*get estimated time to run all tests*/
        if(strcmp(argv[a], "-d")==0)
        {
            duration = 0;
        }
        else if(strcmp(argv[a], "-a")==0)
        {
            for(i=0; i<TestTypeMax; ++i)
            {
                testList[i].enabled = 1;
            }
        }
        else
        {
            int argKnown = 0;
            if(strlen(argv[a]) > 2)
            {
                for(i=0; i<TestTypeMax; ++i)
                {
                    if(strcmp(testList[i].name, argv[a]+2)==0)
                    {
                        testList[i].enabled = 1;
                        argKnown = 1;
                        break;
                    }
                }
            }
            if(!argKnown)
            {
                printf("Invalid arg %s\n", argv[a]);
                printUsage();   
                return 0;
            }
        }
    }

    if(duration==0)
    {
        for(i=0; i<TestTypeMax; ++i)
        {
            if(testList[i].enabled)
                duration += testList[i].getDuration();
        }
        printf("%d\n", duration);
        return 0;
    }
    
    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;

    printf("consumer: start\n");

    rc = rbus_open(&handle, "EventConsumer");
    printf("consumer: rbus_open=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit1;


    /*tell provider we are starting*/
    if(runningParamConsumer_Set(handle, "Device.TestProvider.TestRunning", RBUS_TRUE) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: provider didn't get ready in time\n");
        goto exit2;
    }

    /*run all enabled tests*/
    for(i=0; i<TestTypeMax; ++i)
    {
        if(testList[i].enabled)
        {
            printf("running test %s\n", testList[i].name);
            testList[i].runTest(handle);
        }
    }

    /*tell provider we are done*/
    runningParamConsumer_Set(handle, "Device.TestProvider.TestRunning", RBUS_FALSE);

exit2:
    /*close*/
    rc = rbus_close(handle);
    printf("consumer: rbus_close=%d\n", rc);

exit1:
    printf("consumer: exit\n");
    return rc;
}


