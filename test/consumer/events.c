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
#include <rtList.h>
#include "../common/test_macros.h"

#define SLEEP_TABLE_ROWS 1
#define SLEEP_VALUE_CHANGE 3
#define NUM_ROW_TESTS 17
#define NUM_VC_TESTS 34

int getDurationEvents()
{
    return NUM_ROW_TESTS*SLEEP_TABLE_ROWS + NUM_VC_TESTS*SLEEP_VALUE_CHANGE;
}

void rbusValueChange_SetPollingPeriod(int seconds);

rtList gResultList;

typedef struct Result
{
    rbusEventType_t type;
    char* user;
    char* name;
    char* data;
    bool ok;
} Result;

void freeResult(void* p)
{
    Result* r = p;
    free(r->user);
    free(r->name);
    free(r->data);
    free(r);
}

static void eventHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);

    rtListItem item;
    Result* r;
    bool found = false;
    char const* data = "";

    if(event->type == RBUS_EVENT_OBJECT_CREATED ||
       event->type == RBUS_EVENT_OBJECT_DELETED)
    {
        data = rbusValue_GetString(rbusObject_GetValue(event->data, "rowName"), NULL);
    }
    else if(event->type == RBUS_EVENT_VALUE_CHANGED)
    {
        data = rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL);
    }
    else
    {
        printf("ERROR ! unexpected type\n");
    }

    PRINT_TEST_EVENT("test_Events", event, subscription);

    printf(
    "   data=%s\n"
    "   user=%s\n",
        data,
        (char*)subscription->userData);

    rtList_GetFront(gResultList, &item);
    while(item)
    {
        rtListItem_GetData(item, (void**)&r);

        if( r->type == event->type &&
            strcmp(r->user, (char*)subscription->userData) == 0 &&
            strcmp(r->name, event->name) == 0 &&
            strcmp(r->data, data) == 0)
        {
            r->ok = r->ok ? false : true; /*flip it so we can test unexpected events as well*/
            found = true;
        }

        rtListItem_GetNext(item, &item);
    }

    if(!found)
        printf("\n%s %s %s\n\n", "# FAIL NOT FOUND", event->name, data);

}

#define SUBSCRIBE(T) VERIFY(rbusEvent_Subscribe(handle, T, eventHandler, T))

static void addTestResult(int i, char const* user, char const* name, char const* data, bool ok)
{
    Result* r = malloc(sizeof(struct Result));
    r->type = i;
    r->user = strdup(user);
    r->name = strdup(name);
    r->data = strdup(data);
    r->ok = ok;
    rtList_PushBack(gResultList, r, NULL);
}

static void setTestResult(int i, char const* user, char const* name, char const* data, bool ok)
{
    rtList_Destroy(gResultList, freeResult);
    rtList_Create(&gResultList);

    addTestResult(i, user, name, data, ok);
}

static void checkTestResult(int wait)
{
    rtListItem item;
    Result* r;

    sleep(wait);

    rtList_GetFront(gResultList, &item);
    while(item)
    {
        rtListItem_GetData(item, (void**)&r);

        printf("\n%s %s %s\n\n", r->ok ? "# PASS" : "# FAIL", r->user, r->data);

        TALLY(r->ok);

        rtListItem_GetNext(item, &item);
    }
}

static int testAddRow(rbusHandle_t handle, char const* user, char const* name, char const* data, char const* alias)
{
    printf("\n# TEST ADD ROW %s %s %s %s #\n\n", user, name, data, alias);
    setTestResult(RBUS_EVENT_OBJECT_CREATED, user, name, data, false);
    VERIFY(rbusTable_addRow(handle, name, alias, NULL));
    checkTestResult(SLEEP_TABLE_ROWS);
    return RBUS_ERROR_SUCCESS;
}

static int testAddRowMulti(rbusHandle_t handle, char const* user1, char const* user2 , char const* user3, char const* name, char const* data, char const* alias)
{
    printf("\n# TEST ADD ROW %s %s %s %s %s %s #\n\n", user1, user2, user3 ? user3 : "NULL", name, data, alias);
    setTestResult(RBUS_EVENT_OBJECT_CREATED, user1, name, data, false);
    if(user2)
        addTestResult(RBUS_EVENT_OBJECT_CREATED, user2, name, data, false);
    if(user3)
        addTestResult(RBUS_EVENT_OBJECT_CREATED, user3, name, data, false);
    VERIFY(rbusTable_addRow(handle, name, alias, NULL));
    checkTestResult(SLEEP_TABLE_ROWS);
    return RBUS_ERROR_SUCCESS;
}

static int testRemoveRow(rbusHandle_t handle, char const* user, char const* name, char const* data)
{
    printf("\n# TEST REM ROW %s %s %s #\n\n", user, name, data);
    setTestResult(RBUS_EVENT_OBJECT_DELETED, user, name, data, false);
    VERIFY(rbusTable_removeRow(handle, data));
    checkTestResult(SLEEP_TABLE_ROWS);
    return RBUS_ERROR_SUCCESS;
}

static int testRemoveRowMulti(rbusHandle_t handle, char const* user1, char const* user2, char const* user3, char const* name, char const* data)
{
    printf("\n# TEST REM ROW %s %s %s %s %s #\n\n", user1, user2, user3  ? user3 : "NULL", name, data);
    setTestResult(RBUS_EVENT_OBJECT_DELETED, user1, name, data, false);
    if(user2)
        addTestResult(RBUS_EVENT_OBJECT_DELETED, user2, name, data, false);
    if(user3)
        addTestResult(RBUS_EVENT_OBJECT_DELETED, user3, name, data, false);
    VERIFY(rbusTable_removeRow(handle, data));
    checkTestResult(SLEEP_TABLE_ROWS);
    return RBUS_ERROR_SUCCESS;
}

static int testSetStr(rbusHandle_t handle, char const* user, char const* name, char const* data, bool ok)
{
    printf("\n# TEST SET STR %s %s %s #\n\n", user, name, data);
    setTestResult(RBUS_EVENT_VALUE_CHANGED, user, name, data, ok);
    VERIFY(rbus_setStr(handle, name, data));
    checkTestResult(SLEEP_VALUE_CHANGE);
    return RBUS_ERROR_SUCCESS;
}

static int testSetStrMulti(rbusHandle_t handle, char const* user1, char const* user2, char const* user3, char const* name, char const* data, bool ok)
{
    printf("\n# TEST SET STR %s %s %s %s %s #\n\n", user1, user2, user3, name, data);
    setTestResult(RBUS_EVENT_VALUE_CHANGED, user1, name, data, ok);
    if(user2)
        addTestResult(RBUS_EVENT_VALUE_CHANGED, user2, name, data, ok);
    if(user3)
        addTestResult(RBUS_EVENT_VALUE_CHANGED, user3, name, data, ok);
    VERIFY(rbus_setStr(handle, name, data));
    checkTestResult(SLEEP_VALUE_CHANGE);
    return RBUS_ERROR_SUCCESS;
}

static int testTableEvents(rbusHandle_t handle)
{
    SUBSCRIBE("Device.TestProvider.Table1.");
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.", "Device.TestProvider.Table1.", "Device.TestProvider.Table1.1", "t_1"));
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.", "Device.TestProvider.Table1.", "Device.TestProvider.Table1.2", "t_2"));

    SUBSCRIBE("Device.TestProvider.Table1.[t_1].Table2.");
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.[t_1].Table2.", "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.1.Table2.1", "t_1_1"));
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.[t_1].Table2.", "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.1.Table2.2", "t_1_2"));

    SUBSCRIBE("Device.TestProvider.Table1.*.Table2.");
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.*.Table2.", "Device.TestProvider.Table1.2.Table2.", "Device.TestProvider.Table1.2.Table2.1", "t_2_1"));
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.*.Table2.", "Device.TestProvider.Table1.2.Table2.", "Device.TestProvider.Table1.2.Table2.2", "t_2_2"));
    VERIFY(testAddRowMulti(handle, "Device.TestProvider.Table1.*.Table2.", "Device.TestProvider.Table1.[t_1].Table2.", NULL,
        "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.1.Table2.3", "t_1_3"));

    SUBSCRIBE("Device.TestProvider.Table1.*.Table2.1.Table3.");
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.*.Table2.1.Table3.", "Device.TestProvider.Table1.1.Table2.1.Table3.", "Device.TestProvider.Table1.1.Table2.1.Table3.1", "t_1_1_1"));
    VERIFY(testAddRow(handle, "Device.TestProvider.Table1.*.Table2.1.Table3.", "Device.TestProvider.Table1.1.Table2.1.Table3.", "Device.TestProvider.Table1.1.Table2.1.Table3.2", "t_1_1_2"));

    VERIFY(testRemoveRow(handle, "Device.TestProvider.Table1.*.Table2.1.Table3.", "Device.TestProvider.Table1.1.Table2.1.Table3.", "Device.TestProvider.Table1.1.Table2.1.Table3.1"));
    VERIFY(testRemoveRowMulti(handle, "Device.TestProvider.Table1.*.Table2.", "Device.TestProvider.Table1.[t_1].Table2.", NULL,
        "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.1.Table2.3"));

    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.[t_1].Table2."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.*.Table2."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.*.Table2.1.Table3."));

    return RBUS_ERROR_SUCCESS;
}

static int testTableRemoveAllEvents(rbusHandle_t handle)
{
    SUBSCRIBE("Device.TestProvider.Table1.");

    SUBSCRIBE("Device.TestProvider.Table1.1.Table2.");
    SUBSCRIBE("Device.TestProvider.Table1.[t_1].Table2.");
    VERIFY(testRemoveRowMulti(handle, "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.[t_1].Table2.", NULL, 
        "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.1.Table2.1"));
    VERIFY(testRemoveRowMulti(handle, "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.[t_1].Table2.", NULL, 
        "Device.TestProvider.Table1.1.Table2.", "Device.TestProvider.Table1.1.Table2.2"));

    SUBSCRIBE("Device.TestProvider.Table1.2.Table2.");
    SUBSCRIBE("Device.TestProvider.Table1.*.Table2.");
    VERIFY(testRemoveRowMulti(handle, "Device.TestProvider.Table1.2.Table2.", "Device.TestProvider.Table1.*.Table2.", NULL,
        "Device.TestProvider.Table1.2.Table2.", "Device.TestProvider.Table1.2.Table2.2"));
    VERIFY(testRemoveRowMulti(handle, "Device.TestProvider.Table1.2.Table2.", "Device.TestProvider.Table1.*.Table2.", NULL,
        "Device.TestProvider.Table1.2.Table2.", "Device.TestProvider.Table1.2.Table2.1"));

    VERIFY(testRemoveRow(handle, "Device.TestProvider.Table1.", "Device.TestProvider.Table1.", "Device.TestProvider.Table1.2"));
    VERIFY(testRemoveRow(handle, "Device.TestProvider.Table1.", "Device.TestProvider.Table1.", "Device.TestProvider.Table1.1"));

    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.1.Table2."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.[t_1].Table2."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.2.Table2."));
    VERIFY(rbusEvent_Unsubscribe(handle, "Device.TestProvider.Table1.*.Table2."));

    return RBUS_ERROR_SUCCESS;    
}

static int testSingleValueChange(rbusHandle_t handle, char const* user, char const* name, char const* value, bool ok)
{
    SUBSCRIBE((char*)user);
    VERIFY(testSetStr(handle, user, name, value, ok));
    VERIFY(rbusEvent_Unsubscribe(handle, user));
    return RBUS_ERROR_SUCCESS;
}

static int testMultiValueChange(rbusHandle_t handle, char const* user1, char const* user2, char const* user3, char const* name, char const* value, bool ok)
{
    SUBSCRIBE((char*)user1);
    if(user2)
        SUBSCRIBE((char*)user2);
    if(user3)
        SUBSCRIBE((char*)user3);
    VERIFY(testSetStrMulti(handle, user1, user2, user3, name, value, ok));
    VERIFY(rbusEvent_Unsubscribe(handle, user1));
    if(user2)
        VERIFY(rbusEvent_Unsubscribe(handle, user2));
    if(user3)
        VERIFY(rbusEvent_Unsubscribe(handle, user3));
    return RBUS_ERROR_SUCCESS;
}

static int testAllSingleValueChange(rbusHandle_t handle)
{
    /*cases where events should be received*/
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.1.Table2.1.data", "Device.TestProvider.Table1.1.Table2.1.data", "value1", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_1].Table2.[t_1_1].data", "Device.TestProvider.Table1.1.Table2.1.data", "value2", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.*.data", "Device.TestProvider.Table1.1.Table2.1.data", "value3", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.*.data", "Device.TestProvider.Table1.2.Table2.2.data", "value4", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.1.Table2.*.data", "Device.TestProvider.Table1.1.Table2.2.data", "value5", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.2.Table2.*.data", "Device.TestProvider.Table1.2.Table2.1.data", "value6", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.1.Table2.[t_1_1].data", "Device.TestProvider.Table1.1.Table2.1.data", "value7", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.2.Table2.[t_2_2].data", "Device.TestProvider.Table1.2.Table2.2.data", "value8", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.1.data", "Device.TestProvider.Table1.1.Table2.1.data", "value9", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.2.data", "Device.TestProvider.Table1.2.Table2.2.data", "value10", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.[t_1_2].data", "Device.TestProvider.Table1.1.Table2.2.data", "value11", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.[t_2_1].data", "Device.TestProvider.Table1.2.Table2.1.data", "value12", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_1].Table2.1.data", "Device.TestProvider.Table1.1.Table2.1.data", "value13", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_2].Table2.1.data", "Device.TestProvider.Table1.2.Table2.1.data", "value14", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_1].Table2.*.data", "Device.TestProvider.Table1.1.Table2.1.data", "value15", false));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_2].Table2.*.data", "Device.TestProvider.Table1.2.Table2.1.data", "value16", false));

    /*cases where events should not be received*/
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.1.Table2.1.data", "Device.TestProvider.Table1.1.Table2.2.data", "value21", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_1].Table2.[t_1_1].data", "Device.TestProvider.Table1.2.Table2.1.data", "value22", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.1.Table2.*.data", "Device.TestProvider.Table1.2.Table2.2.data", "value25", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.2.Table2.*.data", "Device.TestProvider.Table1.1.Table2.1.data", "value26", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.1.Table2.[t_1_1].data", "Device.TestProvider.Table1.1.Table2.2.data", "value27", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.2.Table2.[t_2_2].data", "Device.TestProvider.Table1.2.Table2.1.data", "value28", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.1.data", "Device.TestProvider.Table1.1.Table2.2.data", "value29", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.2.data", "Device.TestProvider.Table1.2.Table2.1.data", "value30", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.[t_1_2].data", "Device.TestProvider.Table1.1.Table2.1.data", "value31", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.*.Table2.[t_2_1].data", "Device.TestProvider.Table1.2.Table2.2.data", "value32", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_1].Table2.1.data", "Device.TestProvider.Table1.1.Table2.2.data", "value33", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_2].Table2.1.data", "Device.TestProvider.Table1.2.Table2.2.data", "value34", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_1].Table2.*.data", "Device.TestProvider.Table1.2.Table2.1.data", "value35", true));
    VERIFY(testSingleValueChange(handle, "Device.TestProvider.Table1.[t_2].Table2.*.data", "Device.TestProvider.Table1.1.Table2.1.data", "value36", true));

    return RBUS_ERROR_SUCCESS;
}

static int testAllMultiValueChange(rbusHandle_t handle)
{
    VERIFY(testMultiValueChange(handle, "Device.TestProvider.Table1.1.Table2.1.data", "Device.TestProvider.Table1.*.Table2.1.data", "Device.TestProvider.Table1.[t_1].Table2.[t_1_1].data",
        "Device.TestProvider.Table1.1.Table2.1.data", "value40", false));

    VERIFY(testMultiValueChange(handle, "Device.TestProvider.Table1.*.Table2.[t_1_2].data", "Device.TestProvider.Table1.[t_1].Table2.2.data", "Device.TestProvider.Table1.[t_1].Table2.*.data",
        "Device.TestProvider.Table1.1.Table2.2.data", "value41", false));


    VERIFY(testMultiValueChange(handle, "Device.TestProvider.Table1.*.Table2.[t_2_1].data", "Device.TestProvider.Table1.[t_2].Table2.1.data", "Device.TestProvider.Table1.[t_2].Table2.*.data",
        "Device.TestProvider.Table1.2.Table2.1.data", "value42", false));

    VERIFY(testMultiValueChange(handle, "Device.TestProvider.Table1.*.Table2.*.data", "Device.TestProvider.Table1.*.Table2.2.data", "Device.TestProvider.Table1.[t_2].Table2.*.data",
        "Device.TestProvider.Table1.2.Table2.2.data", "value43", false));

    return RBUS_ERROR_SUCCESS;
}

void testEvents(rbusHandle_t handle, int* countPass, int* countFail)
{
    rbusValue_t val;

    rbusValue_Init(&val);
    rbusValue_SetBoolean(val, true);
    rbus_set(handle, "Device.TestProvider.ResetTables", val, NULL);
    rbusValue_Release(val);

    rtList_Create(&gResultList);

    if(testTableEvents(handle) != RBUS_ERROR_SUCCESS
    || testAllSingleValueChange(handle) != RBUS_ERROR_SUCCESS
    || testAllMultiValueChange(handle) != RBUS_ERROR_SUCCESS
    || testTableRemoveAllEvents(handle) != RBUS_ERROR_SUCCESS)
        goto exit1;

exit1:
    rtList_Destroy(gResultList,freeResult);
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_Events");
}
