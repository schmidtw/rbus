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

#define UNUSED1(a)              (void)(a)
#define UNUSED2(a,b)            UNUSED1(a),UNUSED1(b)
#define UNUSED3(a,b,c)          UNUSED1(a),UNUSED2(b,c)
#define UNUSED4(a,b,c,d)        UNUSED1(a),UNUSED3(b,c,d)
#define UNUSED5(a,b,c,d,e)      UNUSED1(a),UNUSED4(b,c,d,e)
#define UNUSED6(a,b,c,d,e,f)    UNUSED1(a),UNUSED5(b,c,d,e,f)

#define TotalParams   13

rbusHandle_t        rbusHandle;
int                 loopFor = 60;
char                componentName[20] = "rbusSampleProvider";

rbusError_t SampleProvider_DeviceGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t SampleProvider_SampleDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);
rbusError_t SampleProvider_SampleDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t SampleProvider_NestedObjectsGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t SampleProvider_BuildResponseDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);

rbusDataElement_t dataElements[TotalParams] = {
    {"Device.DeviceInfo.SampleProvider.Manufacturer", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_DeviceGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.DeviceInfo.SampleProvider.ModelName", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_DeviceGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.DeviceInfo.SampleProvider.SoftwareVersion", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_DeviceGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.SampleData.IntData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_SampleDataGetHandler, SampleProvider_SampleDataSetHandler, NULL, NULL, NULL}},
    {"Device.SampleProvider.SampleData.BoolData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_SampleDataGetHandler, SampleProvider_SampleDataSetHandler, NULL, NULL, NULL}},
    {"Device.SampleProvider.SampleData.UIntData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_SampleDataGetHandler, SampleProvider_SampleDataSetHandler, NULL, NULL, NULL}},
    {"Device.SampleProvider.NestedObject1.TestParam", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_NestedObjectsGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.NestedObject1.AnotherTestParam", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_NestedObjectsGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.NestedObject2.TestParam", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_NestedObjectsGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.NestedObject2.AnotherTestParam", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_NestedObjectsGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.TestData.IntData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_BuildResponseDataGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.TestData.BoolData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_BuildResponseDataGetHandler, NULL, NULL, NULL, NULL}},
    {"Device.SampleProvider.TestData.UIntData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_BuildResponseDataGetHandler, NULL, NULL, NULL, NULL}}
};

typedef struct _rbus_sample_data {
    int m_intData;
    bool m_boolData;
    unsigned int m_unsignedIntData;
} rbus_sample_provider_data_t;

rbus_sample_provider_data_t gTestInfo = {0};

rbusError_t SampleProvider_DeviceGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    char const* name;

    rbusValue_Init(&value);

    name = rbusProperty_GetName(property);

    if(strcmp(name, "Device.DeviceInfo.SampleProvider.Manufacturer") == 0)
    {
        printf("Called get handler for [%s]\n", name);
        rbusValue_SetString(value, "COMCAST");
    }
    else if(strcmp(name, "Device.DeviceInfo.SampleProvider.ModelName") == 0)
    {
        printf("Called get handler for [%s]\n", name);
        rbusValue_SetString(value, "XB3");

    }
    else if (strcmp(name, "Device.DeviceInfo.SampleProvider.SoftwareVersion") == 0)
    {
        printf("Called get handler for [%s]\n", name);
        rbusValue_SetSingle(value, 2.14f);
    }
    else
    {
        printf("Cant Handle [%s]\n", name);
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

rbusObject_t gTestObject;
static bool isFirstTime = true;

void _prepare_object_for_future_query(void)
{
    /* This flag is also a sample; as we can always release the object and get a new one.. */
    if (isFirstTime)
    {
        isFirstTime = false;
        rbusObject_Init (&gTestObject, "SampleTestData");

        rbusValue_t val1;
        rbusValue_t val2;
        rbusValue_t val3;
        rbusProperty_t prop1;
        rbusProperty_t prop2;
        rbusProperty_t prop3;

        rbusValue_Init(&val1);
        rbusValue_Init(&val2);
        rbusValue_Init(&val3);

        rbusValue_SetInt32(val1, gTestInfo.m_intData);
        rbusValue_SetBoolean(val2, gTestInfo.m_boolData);
        rbusValue_SetUInt32(val3, gTestInfo.m_unsignedIntData);

        rbusProperty_Init (&prop1, "Device.SampleProvider.TestData.IntData", val1);
        rbusProperty_Init (&prop2, "Device.SampleProvider.TestData.BoolData", val2);
        rbusProperty_Init (&prop3, "Device.SampleProvider.TestData.UIntData", val3);

        rbusObject_SetProperty(gTestObject, prop1);
        rbusObject_SetProperty(gTestObject, prop2);
        rbusObject_SetProperty(gTestObject, prop3);

        rbusValue_Release(val1);
        rbusValue_Release(val2);
        rbusValue_Release(val3);
        rbusProperty_Release(prop1);
        rbusProperty_Release(prop2);
        rbusProperty_Release(prop3);
    }
    else
    {
        rbusValue_t val1;
        rbusValue_t val2;
        rbusValue_t val3;

        rbusValue_Init(&val1);
        rbusValue_Init(&val2);
        rbusValue_Init(&val3);

        rbusValue_SetInt32(val1, gTestInfo.m_intData);
        rbusValue_SetBoolean(val2, gTestInfo.m_boolData);
        rbusValue_SetUInt32(val3, gTestInfo.m_unsignedIntData);

        rbusObject_SetValue(gTestObject, "Device.SampleProvider.TestData.IntData", val1);
        rbusObject_SetValue(gTestObject, "Device.SampleProvider.TestData.BoolData", val2);
        rbusObject_SetValue(gTestObject, "Device.SampleProvider.TestData.UIntData", val3);

        rbusValue_Release(val1);
        rbusValue_Release(val2);
        rbusValue_Release(val3);
    }
    return;
}

void _release_object_for_future_query()
{
    rbusObject_Release(gTestObject);
}

rbusError_t SampleProvider_BuildResponseDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    char const* name;

    name = rbusProperty_GetName(property);
    value = rbusObject_GetValue(gTestObject, name);
    rbusProperty_SetValue(property, value);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t SampleProvider_SampleDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* name = rbusProperty_GetName(prop);
    rbusValue_t value = rbusProperty_GetValue(prop);
    rbusValueType_t type = rbusValue_GetType(value);

    if(strcmp(name, "Device.SampleProvider.SampleData.IntData") == 0)
    {
        if (type != RBUS_INT32)
        {
            printf("%s Called Set handler with wrong data type\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
        else
        {
            printf("%s Called Set handler with value = %d\n", name, rbusValue_GetInt32(value));
            gTestInfo.m_intData = rbusValue_GetInt32(value);
        }
    }
    else if(strcmp(name, "Device.SampleProvider.SampleData.BoolData") == 0)
    {
        if (type != RBUS_BOOLEAN)
        {
            printf("%s Called Set handler with wrong data type\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
        else
        {
            printf("%s Called Set handler with value = %d\n", name, rbusValue_GetBoolean(value));
            gTestInfo.m_boolData = rbusValue_GetBoolean(value);
        }
    }
    else if (strcmp(name, "Device.SampleProvider.SampleData.UIntData") == 0)
    {
        if (type != RBUS_UINT32)
        {
            printf("%s Called Set handler with wrong data type\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
        else
        {
            printf("%s Called Set handler with value = %u\n", name, rbusValue_GetUInt32(value));
            gTestInfo.m_unsignedIntData = rbusValue_GetUInt32(value);
        }
    }

    /* Sample Case for Build Response APIs that are proposed */
    _prepare_object_for_future_query();

    return RBUS_ERROR_SUCCESS;
}

rbusError_t SampleProvider_SampleDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    char const* name;

    rbusValue_Init(&value);
    name = rbusProperty_GetName(property);

    if(strcmp(name, "Device.SampleProvider.SampleData.IntData") == 0)
    {
        printf("Called get handler for [%s]\n", name);
        rbusValue_SetInt32(value, gTestInfo.m_intData);
    }
    else if(strcmp(name, "Device.SampleProvider.SampleData.BoolData") == 0)
    {
        printf("Called get handler for [%s]\n", name);
        rbusValue_SetBoolean(value, gTestInfo.m_boolData);

    }
    else if (strcmp(name, "Device.SampleProvider.SampleData.UIntData") == 0)
    {
        printf("Called get handler for [%s]\n", name);
        rbusValue_SetUInt32(value, gTestInfo.m_unsignedIntData);
    }
    else
    {
        printf("Cant Handle [%s]\n", name);
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t SampleProvider_NestedObjectsGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    printf("SampleProvider_NestedObjectsGetHandler called for %s\n", rbusProperty_GetName(property));

    rbusValue_t value;
    rbusValue_Init(&value);

    rbusValue_SetString(value, "Testing Rbus Nested Object");
    rbusProperty_SetValue(property, value);

    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    rbusError_t rc;

    (void)argc;
    (void)argv;

    printf("provider: start\n");

    rc = rbus_open(&rbusHandle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(rbusHandle, TotalParams, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements failed: %d\n", rc);
        goto exit1;
    }

    /* Sample Case for Build Response APIs that are proposed */
    _prepare_object_for_future_query();

    while(loopFor--)
    {
        printf("provider: exiting in %d seconds\n", loopFor);
        sleep(1);
    }

    rbus_unregDataElements(rbusHandle, TotalParams, dataElements);

    /* Sample Case for freeing the Response that was prebuilt */
    _release_object_for_future_query();
exit1:
    rbus_close(rbusHandle);

exit2:
    printf("provider: exit\n");
    return 0;
}
