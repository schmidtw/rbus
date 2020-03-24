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

int reopened = 0;

static void eventHandler(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbus_Tlv_t const* eventData)
{
    (void)(handle);
    printf(
        "eventHandler called:\n"
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
    if(reopened && strcmp((char*)subscription->user_data, "MultiConsumer1")==0)
    {
        printf("_test_:eventHandler reopen ERROR: should not have been called user_data=[%s] value=[%s]\n", (char*)subscription->user_data, (char*)eventData->value);
    }
    else
    {
        printf("_test_:eventHandler user_data:'%s' value:'%s'\n", (char*)subscription->user_data, (char*)eventData->value);
    }
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handles[5] = {NULL};

    /*create 5 consumers*/
    char* componentNames[] = {
        "MultiConsumer1",
        "MultiConsumer2",
        "MultiConsumer3",
        "MultiConsumer4",
        "MultiConsumer5"
    };

    char* elements[5][2] = {
        {"Device.MultiProvider1.Param", 
         "Device.MultiProvider1.Event!"},
        {"Device.MultiProvider2.Param", 
         "Device.MultiProvider2.Event!"},
        {"Device.MultiProvider3.Param", 
         "Device.MultiProvider3.Event!"},
        {"Device.MultiProvider4.Param", 
         "Device.MultiProvider4.Event!"},
        {"Device.MultiProvider5.Param", 
         "Device.MultiProvider5.Event!"},
    };

    int rc = RBUS_ERROR_SUCCESS;
    int i;
    int loopFor = 8;
    int eventCount = 0;

    printf("consumer: opening 5 handles\n");
    for(i = 0; i < 5; ++i)
    {
        rc = rbus_open(&handles[i], componentNames[i]);
        if(rc == RBUS_ERROR_SUCCESS)
        {
            printf("_test_:rbus_open result:SUCCESS component:%s\n", componentNames[i]);
        }
        else
        {
            printf("_test_:rbus_open result:FAIL component:%s rc:%d\n", componentNames[i], rc);
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
    }

    /*tell provider we are starting*/
    if(runningParamConsumer_Set(handles[0], "Device.MultiProvider.TestRunning", RBUS_TRUE) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: provider didn't get ready in time\n");
        goto exit1;
    }

    printf("consumer: subscribing 5 handles\n");
    for(i = 0; i < 5; ++i)
    {
        if(handles[i])
        {
            rc = rbusEvent_Subscribe(handles[i], elements[i][1], eventHandler, componentNames[i]);
            if(rc == RBUS_ERROR_SUCCESS)
            {
                printf("_test_:rbusEvent_Subscribe result:SUCCESS event:%s\n", elements[i][1]);
            }
            else
            {
                printf("_test_:rbusEvent_Subscribe result:FAIL event:%s rc:%d\n", elements[i][1], rc);
            }
        }
    }

    while (loopFor--)
    {
        char buffer[64];
        eventCount++;
        sleep(2);

        printf("consumer: get param for 5 handles\n");
        for(i = 0; i < 5; ++i)
        {
            if(handles[i])
            {
                rbus_Tlv_t tlv;
                tlv.name = elements[i][0];
                rc = rbus_get(handles[i], &tlv);
                if(rc == RBUS_ERROR_SUCCESS)
                {
                    if(tlv.type == RBUS_STRING && tlv.value)
                    {
                        printf("_test_:rbus_get result:SUCCESS param:'%s' value:'%s'\n", elements[i][0], (char*)tlv.value);
                    }
                    else
                    {
                        printf("_test_ rbus_get result:FAIL param:'%s' error:'unexpected type %d'\n", elements[i][0], tlv.type);
                    }

                    if(tlv.value)
                        free(tlv.value);
                }
                else
                {
                    printf("_test_:rbus_get result:FAIL param:'%s' rc:%d\n", elements[i][0], rc);
                }
            }
        }

        if(loopFor == 2)
        {
            printf("consumer: reopening 1 handle\n");

            /*test that calling rbus_open on an already openend component will 
                close its previous handle and create a new working handle 
                and valgrind should not show leaks.
                handle 0 should not be subscribed and should no longer get eventHandler calls for it
                however, rbus_get above on the next loops should work */
            i = 0;
            rc = rbus_open(&handles[i], componentNames[i]);
            if(rc == RBUS_ERROR_SUCCESS)
            {
                printf("_test_:rbus_open-reopen result:SUCCESS component:%s\n", componentNames[i]);
            }
            else
            {
                printf("_test_:rbus_open-reopen result:FAIL component:%s rc:%d\n", componentNames[i], rc);
            }
        }
    }

    printf("consumer: unsubscribing 5 handles\n");
    for(i = 0; i < 5; ++i)
    {
        if(handles[i])
        {
            rc = rbusEvent_Unsubscribe(handles[i], elements[i][1]);
            if(rc == RBUS_ERROR_SUCCESS)
            {
                printf("_test_:rbusEvent_Unsubscribe result:SUCCESS event:'%s'\n", elements[i][1]);
            }
            else
            {
                printf("_test_:rbusEvent_Unsubscribe result:FAIL event:'%s' rc:%d\n", elements[i][1], rc);
            }
        }
    }

    /*tell provider we are done (after we unsubcribe to avoid race condition)*/
    runningParamConsumer_Set(handles[0], "Device.MultiProvider.TestRunning", RBUS_FALSE);

exit1:

    printf("consumer: closing 5 handles\n");
    for(i = 0; i < 5; ++i)
    {
        if(handles[i])
        {
            rc = rbus_close(handles[i]);
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
