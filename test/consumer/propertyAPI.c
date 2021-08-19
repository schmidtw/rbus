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

int getDurationPropertyAPI()
{
    return 1;
}

void testPropertyName()
{
    rbusProperty_t prop;

    printf("%s\n",__FUNCTION__);

    rbusProperty_Init(&prop, NULL, NULL);
    TEST(rbusProperty_GetName(prop)==NULL);

    rbusProperty_SetName(prop, "Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength");
    TEST(strcmp(rbusProperty_GetName(prop),"Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength")==0);

    rbusProperty_SetName(prop, "Device.WiFi.Radio.1.Stats.X_COMCAST-COM_NoiseFloor");
    TEST(strcmp(rbusProperty_GetName(prop),"Device.WiFi.Radio.1.Stats.X_COMCAST-COM_NoiseFloor")==0);

    rbusProperty_Release(prop);

    rbusProperty_Init(&prop, "Device.ETSIM2M.SCL.{i}.SAFPolicySet.{i}.ANPPolicy.{i}.RequestCategory.{i}.Schedule.{i}.AbsTimeSpan.{i}.StartTime", NULL);
    TEST(strcmp(rbusProperty_GetName(prop),"Device.ETSIM2M.SCL.{i}.SAFPolicySet.{i}.ANPPolicy.{i}.RequestCategory.{i}.Schedule.{i}.AbsTimeSpan.{i}.StartTime")==0);

    rbusProperty_SetName(prop, "Device.WiFi.AccessPoint.2.AssociatedDevice.3.SignalStrength");
    TEST(strcmp(rbusProperty_GetName(prop),"Device.WiFi.AccessPoint.2.AssociatedDevice.3.SignalStrength")==0);

    rbusProperty_SetName(prop, NULL);
    TEST(rbusProperty_GetName(prop)==NULL);

    rbusProperty_Release(prop);
}

void testPropertyList()
{
    printf("%s\n",__FUNCTION__);

    /*first, a basic test where we have 3 properties*/
    {
        rbusProperty_t prop1, prop2, prop3, propNext;
        rbusValue_t val1, val2, val3;
        int i;


        rbusProperty_Init(&prop1, "prop1", NULL);
        rbusProperty_Init(&prop2, "prop2", NULL);
        rbusProperty_Init(&prop3, "prop3", NULL);

        rbusValue_Init(&val1);
        rbusValue_Init(&val2);
        rbusValue_Init(&val3);

        rbusValue_SetString(val1, "val1");
        rbusValue_SetString(val2, "val2");
        rbusValue_SetString(val3, "val3");

        rbusProperty_SetValue(prop1, val1);
        rbusProperty_SetValue(prop2, val2);
        rbusProperty_SetValue(prop3, val3);

        rbusValue_Release(val1);
        rbusValue_Release(val2);
        rbusValue_Release(val3);

        rbusProperty_SetNext(prop1, prop2);
        rbusProperty_PushBack(prop1, prop3);

        TEST(rbusProperty_GetNext(prop1) != NULL);
        TEST(rbusProperty_GetNext(rbusProperty_GetNext(prop1)) != NULL);
        TEST(strcmp(rbusValue_GetString(rbusProperty_GetValue(prop1), NULL), "val1")==0);
        TEST(strcmp(rbusValue_GetString(rbusProperty_GetValue(rbusProperty_GetNext(prop1)), NULL), "val2")==0);
        TEST(strcmp(rbusValue_GetString(rbusProperty_GetValue(rbusProperty_GetNext(rbusProperty_GetNext(prop1))), NULL), "val3")==0);

        i = 0;
        propNext = prop1;
        while(propNext)
        {
            char const* s;
            char buff[20];
            i++;
            sprintf(buff, "val%d", i);
            s = rbusValue_GetString(rbusProperty_GetValue(propNext), NULL);
            TEST(strcmp(s, buff)==0);
            propNext = rbusProperty_GetNext(propNext);
        }

        rbusProperty_Release(prop1);
        rbusProperty_Release(prop2);
        rbusProperty_Release(prop3);
    }

    /*create a long list*/
    {
        rbusProperty_t first = NULL;
        rbusProperty_t last = NULL;
        rbusProperty_t next = NULL;
        int i;

        /*create list of 100 Int32 properties*/
        for(i=0; i<100; ++i)
        {
            
            rbusValue_t val;
            char name[20];

            rbusValue_Init(&val);
            rbusValue_SetInt32(val, i);

            sprintf(name, "prop%d", i);
            rbusProperty_Init(&next, name, val);

            rbusValue_Release(val);

            if(!first)
            {
                first = last = next;
            }
            else
            {
                if((i%2)==0)/*test setting the last next*/
                {
                    rbusProperty_SetNext(last, next);
                }
                else/*or using PushBack helper*/
                {
                    rbusProperty_PushBack(first, next);
                }
                rbusProperty_Release(next);
                last = next;
            }
        }

        /*set every 10th property's value to a string*/
        next = first;
        while(next)
        {
            rbusValue_t val = rbusProperty_GetValue(next);
            int i = rbusValue_GetInt32(val);
            if((i%10)==0)
            {
                char str[20];
                sprintf(str, "val %d", i);
                rbusValue_SetString(val, str);
            }
            next = rbusProperty_GetNext(next);
        }

        i=0;
        next = first;
        while(next)
        {
            char sdbg[100];
            char final[100];
            rbusValue_t val = rbusProperty_GetValue(next);
            snprintf(final, 100, "%s: %s", rbusProperty_GetName(next), rbusValue_ToDebugString(val,sdbg,100));

            char expected[100];
            if((i%10)==0)
                snprintf(expected, 100, "prop%d: rbusValue type:RBUS_STRING value:val %d", i, i);
            else 
                snprintf(expected, 100, "prop%d: rbusValue type:RBUS_INT32 value:%d", i, i);

            TEST(!strcmp(final, expected));
            next = rbusProperty_GetNext(next);
            i++;
        }

        rbusProperty_Release(first);
    }
}

void testPropertyValue()
{
    printf("%s\n",__FUNCTION__);

    rbusProperty_t prop1, prop2;
    rbusValue_t val1, val2;
    rbusValue_t pval1, pval2;


    rbusValue_Init(&val1);
    rbusValue_SetInt32(val1, 1001);

    rbusValue_Init(&val2);
    rbusValue_SetInt32(val2, 1002);

    rbusProperty_Init(&prop1, "prop1", val1);/*test passing val into init*/
    rbusProperty_Init(&prop2, "prop2", NULL);

    rbusProperty_SetValue(prop2, val2);/*test passing val into set*/

    pval1 = rbusProperty_GetValue(prop1);
    pval2 = rbusProperty_GetValue(prop2);

    TEST(pval1 == val1);
    TEST(pval2 == val2);
    TEST(rbusValue_GetInt32(pval1) == 1001);
    TEST(rbusValue_GetInt32(pval2) == 1002);

    rbusProperty_SetValue(prop1, val2);
    rbusProperty_SetValue(prop2, val1);

    pval1 = rbusProperty_GetValue(prop1);
    pval2 = rbusProperty_GetValue(prop2);

    TEST(pval1 == val2);
    TEST(pval2 == val1);
    TEST(rbusValue_GetInt32(pval1) == 1002);
    TEST(rbusValue_GetInt32(pval2) == 1001);

    rbusProperty_SetValue(prop1, NULL);
    TEST(rbusProperty_GetValue(prop1) == NULL);

    rbusProperty_Release(prop1);
    rbusProperty_Release(prop2);

    TEST(rbusValue_GetInt32(val1) == 1001);
    TEST(rbusValue_GetInt32(val2) == 1002);

    rbusValue_Release(val1);
    rbusValue_Release(val2);
}

void testPropertyCompare()
{
    printf("%s\n",__FUNCTION__);

    rbusProperty_t prop1, prop2;
    rbusValue_t val1, val2;

    rbusValue_Init(&val1);
    rbusValue_SetInt32(val1, 1001);

    rbusValue_Init(&val2);
    rbusValue_SetInt32(val2, 1002);

    rbusProperty_Init(&prop1, "prop1", val1);
    rbusProperty_Init(&prop2, "prop2", val2);

    TEST(rbusProperty_Compare(prop1, prop2) != 0);

    rbusProperty_SetName(prop2, "prop1");

    TEST(rbusProperty_Compare(prop1, prop2) != 0);

    rbusProperty_SetValue(prop2, val1);

    TEST(rbusProperty_Compare(prop1, prop2) == 0);

    rbusProperty_Release(prop1);
    rbusProperty_Release(prop2);
    rbusValue_Release(val1);
    rbusValue_Release(val2);
}

void testPropertyAPI(rbusHandle_t handle, int* countPass, int* countFail)
{
    (void)handle;

    testPropertyName();
    testPropertyList();
    testPropertyValue();
    testPropertyCompare();

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_PropertyAPI");
}