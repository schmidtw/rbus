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
#include "../common/test_macros.h"

static int gDuration = 60;

static int vcCount = 1;

typedef struct IntResult
{
    int32_t newVal;
    int32_t oldVal;
    int32_t filter;
} IntResult;

typedef struct StrResult
{
    char* newVal;
    char* oldVal;
    int32_t filter;
} StrResult;

static IntResult intResults[6][5] = {
    { { 4, 3, 1 }, { 3, 4, 0 }, {0}, {0}, {0} },
    { { 3, 2, 1 }, { 2, 3, 0 }, {0}, {0}, {0} },
    { { 3, 2, 0 }, { 2, 3, 1 }, {0}, {0}, {0} },
    { { 4, 3, 0 }, { 3, 4, 1 }, {0}, {0}, {0} },
    { { 3, 2, 1 }, { 4, 3, 0 }, { 3, 4, 1 }, { 2, 3, 0 }, {0} },
    { { 3, 2, 0 }, { 4, 3, 1 }, { 3, 4, 0 }, { 2, 3, 1 }, {0} }
};
    
static StrResult strResults[6][5] = {
    { { "eeee", "dddd", 1 }, { "dddd", "eeee", 0 }, {0}, {0}, {0} },
    { { "dddd", "cccc", 1 }, { "cccc", "dddd", 0 }, {0}, {0}, {0} },
    { { "dddd", "cccc", 0 }, { "cccc", "dddd", 1 }, {0}, {0}, {0} },
    { { "eeee", "dddd", 0 }, { "dddd", "eeee", 1 }, {0}, {0}, {0} },
    { { "dddd", "cccc", 1 }, { "eeee", "dddd", 0 }, { "dddd", "eeee", 1 }, { "cccc", "dddd", 0 }, {0} },
    { { "dddd", "cccc", 0 }, { "eeee", "dddd", 1 }, { "dddd", "eeee", 0 }, { "cccc", "dddd", 1 }, {0} }
};

int intCounters[6] = {0};
int strCounters[6] = {0};

void rbusValueChange_SetPollingPeriod(int seconds);

int getDurationValueChange()
{
    return gDuration;
}

static void simpleVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    char valExpectNew[32];
    char valExpectOld[32];
    char const* valActualNew;
    char const* valActualOld;
    bool pass;

    PRINT_TEST_EVENT("test_ValueChange", event, subscription);

    snprintf(valExpectNew, 32, "value %d", vcCount);
    snprintf(valExpectOld, 32, "value %d", vcCount-1);

    valActualNew = rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL);
    valActualOld = rbusValue_GetString(rbusObject_GetValue(event->data, "oldValue"), NULL);

    pass = (strcmp(valExpectNew, valActualNew)==0 && strcmp(valExpectOld, valActualOld)==0);

    TALLY(pass);

    printf("_test_ValueChange %s: expect=[value:%s oldValue:%s] actual=[value:%s oldValue:%s]\n", 
        pass ? "PASS" : "FAIL", valExpectNew, valExpectOld, valActualNew, valActualOld);

    vcCount++;
}

static void intVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    int index = atoi(&event->name[strlen(event->name)-1]);
    int count = intCounters[index];    

    PRINT_TEST_EVENT("test_ValueChange_intVCHandler", event, subscription);

    int pass =  intResults[index][count].newVal == rbusValue_GetInt32(rbusObject_GetValue(event->data, "value")) &&
                intResults[index][count].oldVal == rbusValue_GetInt32(rbusObject_GetValue(event->data, "oldValue")) &&
                intResults[index][count].filter == rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter"));
    TALLY(pass);

    printf("_test_ValueChange_intVCHandler %s: expect=[value:%d oldValue:%d filter:%d] actual=[value:%d oldValue:%d filter:%d]\n", 
            pass ? "PASS" : "FAIL", 
            intResults[index][count].newVal, 
            intResults[index][count].oldVal, 
            intResults[index][count].filter,
            rbusValue_GetInt32(rbusObject_GetValue(event->data, "value")), 
            rbusValue_GetInt32(rbusObject_GetValue(event->data, "oldValue")),
            rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")));

    intCounters[index]++;
}

static void stringVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    int index = atoi(&event->name[strlen(event->name)-1]);
    int count = strCounters[index];    
    PRINT_TEST_EVENT("test_ValueChange_stringVCHandler", event, subscription);
    int pass =  strcmp(strResults[index][count].newVal, rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL))==0 &&
                strcmp(strResults[index][count].oldVal, rbusValue_GetString(rbusObject_GetValue(event->data, "oldValue"), NULL))==0 &&
                strResults[index][count].filter == rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter"));

    TALLY(pass);

    printf("_test_ValueChange_intVCHandler %s: expect=[value:%s oldValue:%s filter:%d] actual=[value:%s oldValue:%s filter:%d]\n", 
            pass ? "PASS" : "FAIL", 
            strResults[index][count].newVal, 
            strResults[index][count].oldVal, 
            strResults[index][count].filter,
            rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL), 
            rbusValue_GetString(rbusObject_GetValue(event->data, "oldValue"), NULL),
            rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")));

    strCounters[index]++;
}

void testValueChange(rbusHandle_t handle, int* countPass, int* countFail)
{
    int rc;
    int i;
    rbusValue_t intVal, strVal;
    rbusFilter_t filter[12];

    rbusValueChange_SetPollingPeriod(1);

    /*
     *test simply subscribe
     */

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.VCParam", simpleVCHandler, NULL, 0);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_ValueChange rbusEvent_Subscribe %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit0;

    sleep(12);

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.VCParam");
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_ValueChange rbusEvent_Unsubscribe %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);

    /*
     * test subscribeEx with all filter types
     * currently testing only ints and strings
     */
    rbusValue_Init(&intVal);
    rbusValue_SetInt32(intVal, 3);

    rbusValue_Init(&strVal);
    rbusValue_SetString(strVal, "dddd");

    rbusFilter_InitRelation(&filter[0], RBUS_FILTER_OPERATOR_GREATER_THAN, intVal);
    rbusFilter_InitRelation(&filter[1], RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL, intVal);
    rbusFilter_InitRelation(&filter[2], RBUS_FILTER_OPERATOR_LESS_THAN, intVal);
    rbusFilter_InitRelation(&filter[3], RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL, intVal);
    rbusFilter_InitRelation(&filter[4], RBUS_FILTER_OPERATOR_EQUAL, intVal);
    rbusFilter_InitRelation(&filter[5], RBUS_FILTER_OPERATOR_NOT_EQUAL, intVal);

    rbusFilter_InitRelation(&filter[6], RBUS_FILTER_OPERATOR_GREATER_THAN, strVal);
    rbusFilter_InitRelation(&filter[7], RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL, strVal);
    rbusFilter_InitRelation(&filter[8], RBUS_FILTER_OPERATOR_LESS_THAN, strVal);
    rbusFilter_InitRelation(&filter[9], RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL, strVal);
    rbusFilter_InitRelation(&filter[10], RBUS_FILTER_OPERATOR_EQUAL, strVal);
    rbusFilter_InitRelation(&filter[11], RBUS_FILTER_OPERATOR_NOT_EQUAL, strVal);

    rbusEventSubscription_t subscription[12] = {
        {"Device.TestProvider.VCParamInt0", filter[0], 0, 0, intVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamInt1", filter[1], 0, 0, intVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamInt2", filter[2], 0, 0, intVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamInt3", filter[3], 0, 0, intVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamInt4", filter[4], 0, 0, intVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamInt5", filter[5], 0, 0, intVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamStr0", filter[6], 0, 0, stringVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamStr1", filter[7], 0, 0, stringVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamStr2", filter[8], 0, 0, stringVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamStr3", filter[9], 0, 0, stringVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamStr4", filter[10], 0, 0, stringVCHandler, NULL, NULL, NULL},
        {"Device.TestProvider.VCParamStr5", filter[11], 0, 0, stringVCHandler, NULL, NULL, NULL}
    };

    rc = rbusEvent_SubscribeEx(handle, subscription, 12, 0);

    rbusValue_Release(intVal);
    rbusValue_Release(strVal);
    for(i = 0; i < 12; ++i)
        rbusFilter_Release(filter[i]);

    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_ValueChange rbusEvent_SubscribeEx %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit0;

    sleep(gDuration-12);

    rc = rbusEvent_UnsubscribeEx(handle, subscription, 12);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_ValueChange rbusEvent_UnsubscribeEx %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);

    /*test filters on strings*/
exit0:
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_ValueChange");
}

