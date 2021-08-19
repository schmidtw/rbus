#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <endian.h>
#include <float.h>
#include <unistd.h>
#include <rbus.h>
#include "../../src/rbus_buffer.h"
#include <math.h>
#include "../common/test_macros.h"

int getDurationObjectAPI()
{
    return 1;
}

void testObjectName()
{
    rbusObject_t obj;

    printf("%s\n",__FUNCTION__);

    rbusObject_Init(&obj, NULL);
    TEST(rbusObject_GetName(obj) == NULL);
    rbusObject_Release(obj);

    rbusObject_Init(&obj, "Apple");
    TEST(strcmp(rbusObject_GetName(obj), "Apple") == 0);

    rbusObject_SetName(obj, "Orange");
    TEST(strcmp(rbusObject_GetName(obj), "Orange") == 0);
    rbusObject_Release(obj);
}

void testObjectProperties()
{
    rbusObject_t obj1, obj2;
    rbusProperty_t prop1, prop2, oprop1, oprop2;
    rbusValue_t val1, val2;

    rbusValue_Init(&val1);
    rbusValue_SetInt32(val1, 1001);

    rbusValue_Init(&val2);
    rbusValue_SetInt32(val2, 1002);

    rbusProperty_Init(&prop1, "prop1", val1);
    rbusProperty_Init(&prop2, "prop2", val2);

    printf("%s\n",__FUNCTION__);

    rbusObject_Init(&obj1, NULL);
    rbusObject_Init(&obj2, NULL);

    rbusObject_SetProperties(obj1, prop1);
    rbusObject_SetProperties(obj2, prop2);

    oprop1 = rbusObject_GetProperties(obj1);
    oprop2 = rbusObject_GetProperties(obj2);

    TEST(oprop1 == prop1);
    TEST(oprop2 == prop2);

    TEST(rbusValue_GetInt32(rbusProperty_GetValue(oprop1)) == 1001);
    TEST(rbusValue_GetInt32(rbusProperty_GetValue(oprop2)) == 1002);

    rbusObject_SetProperties(obj1, prop2);
    rbusObject_SetProperties(obj2, prop1);

    oprop1 = rbusObject_GetProperties(obj1);
    oprop2 = rbusObject_GetProperties(obj2);

    TEST(oprop1 == prop2);
    TEST(oprop2 == prop1);

    rbusObject_SetProperties(obj1, NULL);
    TEST(rbusObject_GetProperties(obj1) == NULL);

    rbusObject_Release(obj1);
    rbusObject_Release(obj2);

    TEST(rbusValue_GetInt32(rbusProperty_GetValue(prop1)) == 1001);
    TEST(rbusValue_GetInt32(rbusProperty_GetValue(prop2)) == 1002);

    rbusProperty_Release(prop1);
    rbusProperty_Release(prop2);

    rbusValue_Release(val1);
    rbusValue_Release(val2);
}

void testObjectProperty()
{
    rbusObject_t obj;
    rbusProperty_t prop1, prop2, prop3;
    rbusProperty_t replace1, replace2, replace3;

    printf("%s\n",__FUNCTION__);

    rbusProperty_Init(&prop1, "prop1", NULL);
    rbusProperty_Init(&prop2, "prop2", NULL);
    rbusProperty_Init(&prop3, "prop3", NULL);
    rbusProperty_Init(&replace1, "prop1", NULL);
    rbusProperty_Init(&replace2, "prop2", NULL);
    rbusProperty_Init(&replace3, "prop3", NULL);

    rbusProperty_PushBack(prop1, prop2);
    rbusProperty_PushBack(prop1, prop3);

    rbusObject_Init(&obj, NULL);

    rbusObject_SetProperties(obj, prop1);

    TEST(rbusObject_GetProperty(obj, "prop1") == prop1);
    TEST(rbusObject_GetProperty(obj, "prop2") == prop2);
    TEST(rbusObject_GetProperty(obj, "prop3") == prop3);

    rbusObject_SetProperty(obj, replace3);
    TEST(rbusObject_GetProperty(obj, "prop1") == prop1);
    TEST(rbusObject_GetProperty(obj, "prop2") == prop2);
    TEST(rbusObject_GetProperty(obj, "prop3") == replace3);

    rbusObject_SetProperty(obj, replace2);
    TEST(rbusObject_GetProperty(obj, "prop1") == prop1);
    TEST(rbusObject_GetProperty(obj, "prop2") == replace2);
    TEST(rbusObject_GetProperty(obj, "prop3") == replace3);

    rbusObject_SetProperty(obj, replace1);
    TEST(rbusObject_GetProperty(obj, "prop1") == replace1);
    TEST(rbusObject_GetProperty(obj, "prop2") == replace2);
    TEST(rbusObject_GetProperty(obj, "prop3") == replace3);

    rbusProperty_Release(prop1);
    rbusProperty_Release(prop2);
    rbusProperty_Release(prop3);
    rbusProperty_Release(replace1);
    rbusProperty_Release(replace2);
    rbusProperty_Release(replace3);

    rbusObject_Release(obj);
}

void testObjectAPI(rbusHandle_t handle, int* countPass, int* countFail)
{
    (void)handle;

    testObjectName();
    testObjectProperties();
    testObjectProperty();

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_ObjectAPI");
}