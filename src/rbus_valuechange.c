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

#include <rbus_valuechange.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <rtVector.h>

#define VC_LOCK() {pthread_mutex_lock(&vcmutex);}
#define VC_UNLOCK() {pthread_mutex_unlock(&vcmutex);}

static int              vcinit      = 0;
static int              vcrunning   = 0;
static int              vcperiod    = 30;//seconds
static rtVector         vcparams    = NULL;
static pthread_mutex_t  vcmutex;
static pthread_t        vcthread;

typedef struct ValueChangeRecord
{
    rbusHandle_t rbusHandle;    //needed when calling rbus_getHandler and rbusEvent_Publish
    elementNode const* node;    //used to call the rbus_getHandler is contains
    rbus_Tlv_t tlv;             //the parameter with value that gets cached
} ValueChangeRecord;

static void rbusValueChange_Init()
{
    printf("%s\n", __FUNCTION__);

    if(vcinit)
        return;

    vcinit = 1;

    rtVector_Create(&vcparams);

    pthread_mutexattr_t attrib;
    pthread_mutexattr_init(&attrib);
    pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
    if(0 != pthread_mutex_init(&vcmutex, &attrib))
    {
        printf("%s: failed to initialize mutex\n", __FUNCTION__);
    }
}

static void vcParams_Free(void* p)
{
    ValueChangeRecord* rec = (ValueChangeRecord*)p;
    free((void*)rec->tlv.name);
    free(rec->tlv.value);
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

static ValueChangeRecord* vcParams_FindByName(const char* name)
{
    size_t i;
    for(i=0; i < rtVector_Size(vcparams); ++i)
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(vcparams, i);
        if(rec && !strcmp(rec->tlv.name, name))
            return rec;
    }
    return NULL;
}

static int rbusTlv_CompareValue(rbus_Tlv_t* t1, rbus_Tlv_t* t2)
{
    //This is just an assertion and can be macro or removed later
    if(strcmp(t1->name, t2->name))
    {
        printf("%s: name should match names but it does not\n", __FUNCTION__);
        return 1;
    }

    //This is just an assertion and can be macro or removed later
    if(t1->type != t2->type)
    {
        printf("%s: type should match names but it does not\n", __FUNCTION__);
        return 1;
    }

    //Valid test for value change
    if(t1->length != t2->length)
    {
        printf("%s: value has changed because length has changed\n", __FUNCTION__);
        return 1;
    }

    //Valid test for value change
    if(memcmp(t1->value, t2->value, t1->length))
    {
        printf("%s: value has changed because the data has changed\n", __FUNCTION__);
        return 1;
    }
    
    printf("%s: value has not changed\n", __FUNCTION__);
    return 0;
}

static void* rbusValueChange_pollingThreadFunc(void *data)
{
    (void)(data);
    printf("%s: start\n", __FUNCTION__);
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
            ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(vcparams, i);
            if(!rec)
                continue;

            rbus_Tlv_t tlv;
            tlv.name = rec->tlv.name; /*TODO: can i reuse the cached name pointer like this ?*/

            int result = rec->node->cbTable->rbus_getHandler(rec->rbusHandle, 1, &tlv, "Self"/*FIXME*/);  

            if(result != RBUS_ERROR_SUCCESS)
            {
                printf("%s: failed to get current value of %s\n", __FUNCTION__, rec->tlv.name);
                continue;
            }

            if(rec->tlv.type == RBUS_STRING)
                printf("%s: %s=%s\n", __FUNCTION__, rec->tlv.name, (const char*)rec->tlv.value);
            else
                printf("%s: %s=%p\n", __FUNCTION__, rec->tlv.name, rec->tlv.value);

            if(rbusTlv_CompareValue(&tlv, &rec->tlv))
            {
                printf("%s: value change detected for %s\n", __FUNCTION__, rec->tlv.name);

                free(rec->tlv.value);
                rec->tlv.length = tlv.length;
                rec->tlv.value = malloc(tlv.length);
                memcpy(rec->tlv.value, tlv.value, tlv.length);

                result = rbusEvent_Publish(rec->rbusHandle, &rec->tlv);

                if(result != RBUS_ERROR_SUCCESS)
                {
                    printf("%s: rbusEvent_Publish failed with result=%d\n", __FUNCTION__, result);
                }
            }
            else
            {
                printf("%s: value change not detected for %s\n", __FUNCTION__, rec->tlv.name);
            }

            if(tlv.value)
            {
                free(tlv.value); 
            }
        }

        VC_UNLOCK();//############ UNLOCK ############
    }
    printf("%s: stop\n", __FUNCTION__);
    return NULL;
}

void rbusValueChange_SetPollingPeriod(int seconds)
{
    printf("%s: %d\n", __FUNCTION__, seconds);
    vcperiod = seconds;
}

void rbusValueChange_AddParameter(rbusHandle_t rbusHandle, const elementNode* paramNode, const char* fullName)
{
    if(!fullName)
    {
        printf("%s: fullName NULL error\n", __FUNCTION__);
        return;
    }
    if(!paramNode)
    {
        printf("%s: paramNode NULL error\n", __FUNCTION__);
        return;
    }
    if(paramNode->type != ELEMENT_TYPE_PARAMETER)
    {
        printf("%s: paramNode type %d error\n", __FUNCTION__, paramNode->type);
        return;
    }
    if(!paramNode->cbTable->rbus_getHandler)
    {
        printf("%s: paramNode getHandler NULL error\n", __FUNCTION__);
        return;
    }

    ValueChangeRecord* rec = (ValueChangeRecord*)malloc(sizeof(ValueChangeRecord));
    rec->rbusHandle = rbusHandle;
    rec->node = paramNode;
    rec->tlv.name = strdup(fullName);

    /*get and cache the current value
      the polling thread will periodically re-get and compare to detect value changes*/
    int result = paramNode->cbTable->rbus_getHandler(rbusHandle, 1, &rec->tlv, "Self"/*FIXME*/);  

    if(result != RBUS_ERROR_SUCCESS)
    {
        printf("%s: failed to get current value for %s\n", __FUNCTION__, fullName);
        vcParams_Free(rec);
        return;
    }

    if(rec->tlv.type == RBUS_STRING)
        printf("%s: %s=%s\n", __FUNCTION__, fullName, (const char*)rec->tlv.value);
    else
        printf("%s: %s=%p\n", __FUNCTION__, fullName, rec->tlv.value);

    if(!vcinit)
    {
        rbusValueChange_Init();
    }

    VC_LOCK();//############ LOCK ############

    rtVector_PushBack(vcparams, rec);

    if(!vcrunning)
    {
        vcrunning = 1;
        pthread_create(&vcthread, NULL, rbusValueChange_pollingThreadFunc, NULL);
    }

    VC_UNLOCK();//############ UNLOCK ############
}

void rbusValueChange_RemoveParameter(rbusHandle_t rbusHandle, const elementNode* paramNode, const char* fullName)
{
    (void)(rbusHandle);
    printf("%s\n", __FUNCTION__);

    if(!vcinit)
    {
        printf("%s: called before init\n", __FUNCTION__);
        return;
    }

    ValueChangeRecord* rec;

    VC_LOCK();//############ LOCK ############

    if(paramNode)
        rec = vcParams_Find(paramNode);
    else
        rec = vcParams_FindByName(fullName);

    if(!rec)
    {
        printf("%s: value change param not found: %s\n", __FUNCTION__, fullName);

        VC_UNLOCK();//############ UNLOCK ############

        return;
    }

    rtVector_RemoveItem(vcparams, rec, vcParams_Free);
    size_t sz = rtVector_Size(vcparams);

    VC_UNLOCK();//############ UNLOCK ############

    if(sz == 0)
    {
        if(vcrunning)
        {
            vcrunning = 0;
            pthread_join(vcthread, NULL);
        }
    }
}

void rbusValueChange_Close(rbusHandle_t rbusHandle)
{
    printf("%s\n", __FUNCTION__);

    if(!vcinit)
    {
        return;
    }

    //remove all params for this bus handle
    size_t i = 0;
    while(i < rtVector_Size(vcparams))
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(vcparams, i);
        if(rec && rec->rbusHandle == rbusHandle)
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
    //but check the size to ensure we do not clean up if params for othe rbus handles exist
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

