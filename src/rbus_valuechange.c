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

/*
    Value-Change Detection:
    Simple API that allows you to add/remove parameters you wish to check value-change for.
    Uses a single thread to poll parameter values across all rbus handles.
    The thread is started on first param added and stopped on last param removed.
    A poling period (default 30 seconds) helps to limit the cpu usage.
    Runs in the provider process, so the value are got with direct callbacks and not over the network.
    The technique is simple:
    1) when a param is added, get and cache its current value.
    2) on a background thread, periodically get the latest value and compare to cached value.
    3) if the value has change, publish an event.
*/

#define _GNU_SOURCE 1 //needed for pthread_mutexattr_settype

#include "rbus_valuechange.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <rtVector.h>
#include <assert.h>

#define VC_LOCK() {int rc = pthread_mutex_lock(&vcmutex); (void)rc;}
#define VC_UNLOCK() {pthread_mutex_unlock(&vcmutex);}

static int              vcinit      = 0;
static int              vcrunning   = 0;
static int              vcperiod    = 30;//seconds
static rtVector         vcparams    = NULL;
static pthread_mutex_t  vcmutex;
static pthread_t        vcthread;

typedef struct ValueChangeRecord
{
    rbusHandle_t handle;    //needed when calling rbus_getHandler and rbusEvent_Publish
    elementNode const* node;    //used to call the rbus_getHandler is contains
    rbusProperty_t property;    //the parameter with value that gets cached
} ValueChangeRecord;

static void rbusValueChange_Init()
{
    rtLog_Debug("%s", __FUNCTION__);

    if(vcinit)
        return;

    vcinit = 1;

    rtVector_Create(&vcparams);

    pthread_mutexattr_t attrib;
    pthread_mutexattr_init(&attrib);
    pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
    if(0 != pthread_mutex_init(&vcmutex, &attrib))
    {
        rtLog_Warn("%s: failed to initialize mutex", __FUNCTION__);
    }
}

static void vcParams_Free(void* p)
{
    ValueChangeRecord* rec = (ValueChangeRecord*)p;
    rbusProperty_Release(rec->property);
    free(rec);
}

static ValueChangeRecord* vcParams_Find(const elementNode* paramNode)
{
    size_t i;
    for(i=0; i < rtVector_Size(vcparams); ++i)
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(vcparams, i);
        if(rec && rec->node == paramNode)
            return rec;
    }
    return NULL;
}

static void* rbusValueChange_pollingThreadFunc(void *userData)
{
    (void)(userData);
    rtLog_Debug("%s: start", __FUNCTION__);
    while(vcrunning)
    {
        size_t i;
        
        sleep(vcperiod);
        
        if(!vcrunning)
            break;

        VC_LOCK();//############ LOCK ############
        //TODO: VC_LOCK around the whole for loop might not be efficient
        //      What if rbusEvent_Publish takes too long.  
        //      This could block the _callback_handler, which calls rbusValueChange_AddParameter, for too long

        for(i=0; i < rtVector_Size(vcparams); ++i)
        {
            rbusProperty_t property;

            ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(vcparams, i);
            if(!rec)
                continue;

            rbusProperty_Init(&property,rbusProperty_GetName(rec->property), NULL);

            rbusGetHandlerOptions_t opts;
            memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
            opts.requestingComponent = "valueChangePollThread";

            int result = rec->node->cbTable.getHandler(rec->handle, property, &opts);

            if(result != RBUS_ERROR_SUCCESS)
            {
                rtLog_Warn("%s: failed to get current value of %s", __FUNCTION__, rbusProperty_GetName(property));
                continue;
            }

            char* sValue = rbusValue_ToString(rbusProperty_GetValue(property), NULL, 0);
            rtLog_Debug("%s: %s=%s", __FUNCTION__, rbusProperty_GetName(property), sValue);
            free(sValue);
            
            if(rbusValue_Compare(rbusProperty_GetValue(property), rbusProperty_GetValue(rec->property)))
            {
                rbusEvent_t event;
                rbusObject_t data;

                rbusObject_Init(&data, NULL);
                rbusObject_SetValue(data, "value", rbusProperty_GetValue(property));
                rbusObject_SetValue(data, "oldValue", rbusProperty_GetValue(rec->property));

                rtLog_Info ("%s: value change detected for %s", __FUNCTION__, rbusProperty_GetName(rec->property));

                event.name = rbusProperty_GetName(rec->property);
                event.data = data;
                event.type = RBUS_EVENT_VALUE_CHANGED;

                result = rbusEvent_Publish(rec->handle, &event);

                rbusProperty_SetValue(rec->property, rbusProperty_GetValue(property));

                rbusObject_Release(data);
                rbusProperty_Release(property);

                if(result != RBUS_ERROR_SUCCESS)
                {
                    rtLog_Warn("%s: rbusEvent_Publish failed with result=%d", __FUNCTION__, result);
                }
            }
            else
            {
                rtLog_Info("%s: value change not detected for %s", __FUNCTION__, rbusProperty_GetName(rec->property));
                rbusProperty_Release(property);
            }
        }

        VC_UNLOCK();//############ UNLOCK ############
    }
    rtLog_Debug("%s: stop", __FUNCTION__);
    return NULL;
}

void rbusValueChange_SetPollingPeriod(int seconds)
{
    rtLog_Debug("%s: %d", __FUNCTION__, seconds);
    vcperiod = seconds;
}

void rbusValueChange_AddPropertyNode(rbusHandle_t handle, elementNode* propNode)
{
    ValueChangeRecord* rec;

    rtLog_Debug("%s: %s", __FUNCTION__, propNode->fullName);

    if(!vcinit)
    {
        rbusValueChange_Init();
    }

    /* basic sanity tests */    
    assert(propNode);
    if(!propNode)
    {
        rtLog_Warn("%s: propNode NULL error", __FUNCTION__);
        return;
    }
    assert(propNode->type == RBUS_ELEMENT_TYPE_PROPERTY);
    if(propNode->type != RBUS_ELEMENT_TYPE_PROPERTY)
    {
        rtLog_Warn("%s: propNode type %d error", __FUNCTION__, propNode->type);
        return;
    }
    assert(propNode->cbTable.getHandler);
    if(!propNode->cbTable.getHandler)
    {
        rtLog_Warn("%s: propNode getHandler NULL error", __FUNCTION__);
        return;
    }

    /* only add the property if its not already in the list */

    VC_LOCK();//############ LOCK ############

    rec = vcParams_Find(propNode);

    VC_UNLOCK();//############ UNLOCK ############

    if(!rec)
    {
        rec = (ValueChangeRecord*)malloc(sizeof(ValueChangeRecord));
        rec->handle = handle;
        rec->node = propNode;

        rbusProperty_Init(&rec->property, propNode->fullName, NULL);

        rbusGetHandlerOptions_t opts;
        memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
        opts.requestingComponent = "valueChangePollThread";
        /*get and cache the current value
          the polling thread will periodically re-get and compare to detect value changes*/
        int result = propNode->cbTable.getHandler(handle, rec->property, &opts);

        if(result != RBUS_ERROR_SUCCESS)
        {
            rtLog_Warn("%s: failed to get current value for %s", __FUNCTION__, propNode->fullName);
            vcParams_Free(rec);
            rec = NULL;
        }

        char* sValue;
        rtLog_Debug("%s: %s=%s", __FUNCTION__, propNode->fullName, (sValue = rbusValue_ToString(rbusProperty_GetValue(rec->property), NULL, 0)));
        free(sValue);

        VC_LOCK();//############ LOCK ############

        rtVector_PushBack(vcparams, rec);

        /* start polling thread if needed */

        if(!vcrunning)
        {
            vcrunning = 1;
            pthread_create(&vcthread, NULL, rbusValueChange_pollingThreadFunc, NULL);
        }

        VC_UNLOCK();//############ UNLOCK ############
    }
}

void rbusValueChange_RemovePropertyNode(rbusHandle_t handle, elementNode* propNode)
{
    ValueChangeRecord* rec;

    (void)(handle);

    rtLog_Debug("%s: %s", __FUNCTION__, propNode->fullName);

    if(!vcinit)
    {
        return;
    }

    VC_LOCK();//############ LOCK ############

    rec = vcParams_Find(propNode);

    VC_UNLOCK();//############ UNLOCK ############

    if(rec)
    {
        bool stopThread;

        VC_LOCK();//############ LOCK ############

        rtVector_RemoveItem(vcparams, rec, vcParams_Free);

        /* if there's nothing left to poll then shutdown the polling thread */

        if(vcrunning && rtVector_Size(vcparams) == 0)
        {
            stopThread = true;
            vcrunning = 0;
        }
        else 
        {
            stopThread = false;
        }

        VC_UNLOCK();//############ UNLOCK ############

        if(stopThread)
        {
            pthread_join(vcthread, NULL);
        }
    }
    else
    {
        rtLog_Warn("%s: value change param not found: %s", __FUNCTION__, propNode->fullName);
    }
}

void rbusValueChange_Close(rbusHandle_t handle)
{
    rtLog_Debug("%s", __FUNCTION__);

    if(!vcinit)
    {
        return;
    }

    //remove all params for this bus handle
    size_t i = 0;
    while(i < rtVector_Size(vcparams))
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(vcparams, i);
        if(rec && rec->handle == handle)
        {
            rtVector_RemoveItem(vcparams, rec, vcParams_Free);
        }
        else
        {
            //only i++ here because rtVector_RemoveItem does a right shift on all the elements after remove index
            i++; 
        }
    }

    //clean up everything once all params are removed
    //but check the size to ensure we do not clean up if params for other rbus handles exist
    if(rtVector_Size(vcparams) == 0)
    {
        if(vcrunning)
        {
            vcrunning = 0;
            pthread_join(vcthread, NULL);
        }
        pthread_mutex_destroy(&vcmutex);
        rtVector_Destroy(vcparams, NULL);
        vcparams = NULL;
        vcinit = 0;
    }
}

