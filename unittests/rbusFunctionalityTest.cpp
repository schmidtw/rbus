/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "gtest/gtest.h"

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <rbus.h>

#define FILTER_VAL 3

char gtest_err[64];

typedef enum
{
  RBUS_GTEST_FILTER1 = 0,
  RBUS_GTEST_FILTER2,
  RBUS_GTEST_GET,
  RBUS_GTEST_GET_EXT,
  RBUS_GTEST_SET,
  RBUS_GTEST_DISC_COMP,
  RBUS_GTEST_METHOD,
  RBUS_GTEST_METHOD_ASYNC
} rbusGtest_t;

static bool asyncCalled = false;
static rbusError_t asyncError = RBUS_ERROR_SUCCESS;

rbusError_t getVCHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
  char const* name = rbusProperty_GetName(property);
  (void)handle;
  (void)opts;

  /*fake a value change every 'myfreq' times this function is called*/
  static int32_t mydata = 0;  /*the actual value to send back*/
  static int32_t mydelta = 1; /*how much to change the value by*/
  static int32_t mycount = 0; /*number of times this function called*/
  static int32_t myfreq = 2;  /*number of times this function called before changing value*/
  static int32_t mymin = 0, mymax=5; /*keep value between mymin and mymax*/

  rbusValue_t value;

  mycount++;

  if((mycount % myfreq) == 0)
  {
    mydata += mydelta;
    if(mydata == mymax)
      mydelta = -1;
    else if(mydata == mymin)
      mydelta = 1;
  }

  printf("Provider: Called get handler for [%s] val=[%d]\n", name, mydata);

  rbusValue_Init(&value);
  rbusValue_SetInt32(value, mydata);
  rbusProperty_SetValue(property, value);
  rbusValue_Release(value);

  return RBUS_ERROR_SUCCESS;
}

rbusError_t getHandler1(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
  char const* name = rbusProperty_GetName(property);
  (void)handle;
  (void)opts;

  printf("Provider: Called get handler for [%s] \n", name );


  return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandler1(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
  char const* name = rbusProperty_GetName(property);
  rbusValue_t value = rbusProperty_GetValue(property);
  char *val = NULL;

  (void)handle;
  (void)opts;

  if(value)
    val = rbusValue_ToString(value,NULL,0);

  printf("setHandler1 called: property=%s value %s\n", name,val);
  return RBUS_ERROR_SUCCESS;
}

rbusError_t ppTableGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);

    (void)handle;
    (void)opts;

    printf(
        "ppTableGetHandler called:\n" \
        "\tproperty=%s\n",
        name);

  return RBUS_ERROR_SUCCESS;
}
rbusError_t ppTableAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
  (void)handle;
  (void)aliasName;

  if(!strcmp(tableName, "Device.TestProvider.PartialPath"))
  {
    static int instanceNumber = 1;
    *instNum = instanceNumber++;
  }

  printf("partialPathTableAddRowHandler table=%s instNum=%d\n", tableName, *instNum);
  return RBUS_ERROR_SUCCESS;
}

rbusError_t ppTableRemRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
  (void)handle;
  (void)rowName;
  return RBUS_ERROR_SUCCESS;
}

rbusError_t ppParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
  rbusValue_t value;
  char const* name = rbusProperty_GetName(property);

  (void)handle;
  (void)opts;

  printf(
      "ppParamGetHandler called:\n" \
      "\tproperty=%s\n",
      name);

  if(!strcmp(name, "Device.TestProvider.PartialPath.1.Param1") ||
      !strcmp(name, "Device.TestProvider.PartialPath.1.Param2") ||
      !strcmp(name, "Device.TestProvider.PartialPath.2.Param1") ||
      !strcmp(name, "Device.TestProvider.PartialPath.2.Param2")
    )
  {
    /*set value to the name of the parameter so consumer can easily verify result*/
    rbusValue_Init(&value);
    rbusValue_SetString(value, name);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
  }
  else
  {
    printf("ppParamGetHandler invalid name %s\n", name);
    return RBUS_ERROR_BUS_ERROR;
  }
}

static rbusError_t methodHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
  (void)handle;
  (void)asyncHandle;
  rbusValue_t value;

  printf("methodHandler called: %s\n", methodName);
  rbusObject_fwrite(inParams, 1, stdout);


  if(strstr(methodName, "Method1()")) {
    rbusValue_Init(&value);
    rbusValue_SetString(value, "Method1()");
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);
    printf("methodHandler success\n");
    return RBUS_ERROR_SUCCESS;
  } else if(strstr(methodName, "MethodAsync1()")) {
    sleep(4);
    rbusValue_Init(&value);
    rbusValue_SetString(value, "MethodAsync1()");
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);
    printf("methodHandler success\n");
    return RBUS_ERROR_SUCCESS;
  }
  printf("methodHandler fail\n");
  return RBUS_ERROR_BUS_ERROR;
}

static int rbusProvider(int runtime)
{
  rbusHandle_t handle;
  int rc = RBUS_ERROR_SUCCESS;

  char componentName[] = "rbusProvider";

  rbusDataElement_t dataElements[] = {
    {"Device.Provider1.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {"Device.Provider1.Param2", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler1, setHandler1, NULL, NULL, NULL, NULL}},
    {"Device.TestProvider.PartialPath.{i}.", RBUS_ELEMENT_TYPE_TABLE, {ppTableGetHandler, NULL, ppTableAddRowHandler, ppTableRemRowHandler, NULL, NULL}},
    {"Device.TestProvider.PartialPath.{i}.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
    {"Device.TestProvider.PartialPath.{i}.Param2", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
    {"Device.TestProvider.Table1.Method1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
    {"Device.TestProvider.MethodAsync1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}}
  };
  #define elements_count sizeof(dataElements)/sizeof(dataElements[0])

  printf("provider: start \n");

  rc = rbus_open(&handle, componentName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(rc != RBUS_ERROR_SUCCESS)
  {
    printf("provider: rbus_open failed: %d\n", rc);
    goto exit2;
  }

  rc = rbus_regDataElements(handle, elements_count, dataElements);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(rc != RBUS_ERROR_SUCCESS)
  {
    printf("provider: rbus_regDataElements failed: %d\n", rc);
    goto exit1;
  }

  rbusTable_addRow(handle, "Device.TestProvider.PartialPath", NULL, NULL);
  rbusTable_addRow(handle, "Device.TestProvider.PartialPath", NULL, NULL);
  sleep(runtime);

  rbusTable_removeRow(handle, "Device.TestProvider.PartialPath");
  rbus_unregDataElements(handle, elements_count, dataElements);

exit1:
  rbus_close(handle);

exit2:
  printf("provider: exit\n");
  return rc;
}

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
  (void)handle;

  rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
  rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");
  rbusValue_t filter = rbusObject_GetValue(event->data, "filter");

  printf("Consumer receiver ValueChange event for param %s\n", event->name);

  if(newValue)
    printf("  New Value: %d\n", rbusValue_GetInt32(newValue));

  if(oldValue)
    printf("  Old Value: %d\n", rbusValue_GetInt32(oldValue));

  if(filter) {
    printf("  filter: %d\n", rbusValue_GetBoolean(filter));
    if(rbusValue_GetBoolean(filter) == 1) {
      int val = rbusValue_GetInt32(newValue);
      if(val != FILTER_VAL) {
        sprintf(gtest_err,"Invalid value '%d' got when filter '%d' is set\n",FILTER_VAL,val);
      }
    }
  }

  if(subscription->userData)
    printf("User data: %s\n", (char*)subscription->userData);
}

static void asyncMethodHandler1(
    rbusHandle_t handle, 
    char const* methodName, 
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;

    printf("asyncMethodHandler1 called: method=%s  error=%d\n", methodName, error);

    asyncCalled = true;

}

static int rbusConsumer(rbusGtest_t test)
{
  int rc = RBUS_ERROR_SUCCESS;
  int runtime = 40;
  char user_data[32] = {0};
  rbusHandle_t handle;
  rbusFilter_t filter;
  rbusValue_t filterValue;
  rbusEventSubscription_t subscription = {"Device.Provider1.Param1", NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};
  char componentName[] = "rbusConsumer";

  sleep(3);
  rc = rbus_open(&handle, componentName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(rc != RBUS_ERROR_SUCCESS)
  {
    printf("consumer: rbus_open failed: %d\n", rc);
    return -1;
  }

  switch(test)
  {
    case RBUS_GTEST_FILTER1:
      {
        /* subscribe to all value change events on property "Device.Provider1.Param1" */
        strcpy(user_data,"My User Data");
        rc = rbusEvent_Subscribe(
            handle,
            "Device.Provider1.Param1",
            eventReceiveHandler,
            user_data);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime-10);

        rbusEvent_Unsubscribe(
            handle,
            "Device.Provider1.Param1");
      }
      break;
    case RBUS_GTEST_FILTER2:
      {
        /* subscribe using filter to value change events on property "Device.Provider1.Param1"
           setting filter to: value >= 3.
         */
        rbusValue_Init(&filterValue);
        rbusValue_SetInt32(filterValue, FILTER_VAL);

        rbusFilter_InitRelation(&filter, RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL, filterValue);

        subscription.filter = filter;

        rc = rbusEvent_SubscribeEx(handle, &subscription, 1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        rbusValue_Release(filterValue);
        rbusFilter_Release(filter);
        sleep(runtime-10);

        rc = rbusEvent_UnsubscribeEx(handle, &subscription, 1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        if(strlen(gtest_err) != 0)
          rc = RBUS_ERROR_BUS_ERROR;
      }
      break;
    case RBUS_GTEST_SET:
      {
        rc = rbus_setStr(handle, "Device.Provider1.Param2", "Gtest set value");
      }
      break;
    case RBUS_GTEST_GET:
      {
        char* value = NULL;
        rc = rbus_getStr(handle, "Device.TestProvider.PartialPath.1.Param1", &value);
      }
      break;
    case RBUS_GTEST_GET_EXT:
      {
        rbusProperty_t props = NULL;
        rbusProperty_t next;
        int actualCount = 0;
        rbusValue_t actualValue;
        const char *params = "Device.TestProvider.PartialPath.";
        const char *params1 = "Device.TestProvider.PartialPath.5.";

        rc = rbus_getExt(handle, 1, &params, &actualCount, &props);
        if(rc == RBUS_ERROR_SUCCESS)
        {
          next = props;
          while(next)
          {
            actualValue = rbusProperty_GetValue(next);
            if(actualValue != NULL && rbusValue_GetType(actualValue) == RBUS_STRING)
              printf("val %s\n", rbusValue_GetString(actualValue, NULL));

            //rbusProperty_fwrite(next, 1, stdout);
            next =  rbusProperty_GetNext(next);
          }
          rbusProperty_Release(props);
        }
        rc = rbus_getExt(handle, 1, &params1, &actualCount, &props);
        if(rc != RBUS_ERROR_SUCCESS)
        {
          /*Invalid wildcard call*/
          rc = RBUS_ERROR_SUCCESS;
        }
      }
      break;
    case RBUS_GTEST_DISC_COMP:
      {
        int i;
        char* elementNames1[] = {"Device.Provider1.Param1"};
        int numComponents = 0;
        char **componentName = NULL;

        rc = rbus_discoverComponentName(handle,1,elementNames1,&numComponents,&componentName);
        if(RBUS_ERROR_SUCCESS == rc) {
          printf ("Discovered components are,\n");
          for(i=0;i<numComponents;i++)
          {
            printf("rbus_discoverComponentName %s: %s\n", elementNames1[i],componentName[i]);
            free(componentName[i]);
          }
          free(componentName);
        }
      }
      break;
    case RBUS_GTEST_METHOD:
      {
        uint32_t instNum;
        rbusObject_t inParams;
        rbusObject_t outParams;
        rbusValue_t value;
        char method1[RBUS_MAX_NAME_LENGTH] = {0};

        rbusObject_Init(&inParams, NULL);
        rbusValue_Init(&value);
        rbusValue_SetString(value, "param1");
        rbusObject_SetValue(inParams, "param1", value);
        rbusValue_Release(value);

        snprintf(method1, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Table1.Method1()");
        printf("\n# TEST rbusMethod_Invoke(%s) \n#\n", method1);
        rc = rbusMethod_Invoke(handle, method1, inParams, &outParams);
        printf("consumer: rbusMethod_Invoke(%s) %s\n", method1,
            rc == RBUS_ERROR_SUCCESS ? "success" : "fail");
        if(rc == RBUS_ERROR_SUCCESS)
        {
          rbusObject_Release(outParams);
        }

      }
      break;
    case RBUS_GTEST_METHOD_ASYNC:
      {
        rbusObject_t inParams;
        rbusValue_t value;

        rbusObject_Init(&inParams, NULL);
        rbusValue_Init(&value);
        rbusValue_SetString(value, "param1");
        rbusObject_SetValue(inParams, "param1", value);
        rbusValue_Release(value);

        asyncCalled = false;
        asyncError = RBUS_ERROR_SUCCESS;
        rc = rbusMethod_InvokeAsync(handle, "Device.TestProvider.MethodAsync1()", inParams, asyncMethodHandler1, 0);
        printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.MethodAsync1()",
            rc == RBUS_ERROR_SUCCESS ? "success" : "fail");
        sleep(5);
      }
      break;
  }

  rbus_close(handle);
  return rc;
}

TEST(rbusApiGetExt, test)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_GET_EXT);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(5);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiGet, test)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_GET);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(5);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiSet, test)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_SET);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(5);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiDiscoverComp, test)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_DISC_COMP);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(5);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiMethod, test)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_METHOD);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(5);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiMethodAsync, test)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_METHOD_ASYNC);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(10);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiValueChangeTest, test1)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_FILTER1);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(50);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiValueChangeTest, test2)
{
  memset(gtest_err,0,sizeof(gtest_err));
  pid_t pid = fork();
  if (pid == 0){
    int ret = 0;
    ret = rbusConsumer(RBUS_GTEST_FILTER2);
    exit(ret);
  } else {
    int ret = 0;
    int customer_status;
    ret = rbusProvider(50);
    waitpid(pid, &customer_status, 0);
    printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
    EXPECT_EQ(WEXITSTATUS(customer_status),0);
    EXPECT_EQ(ret,0);
  }
}
