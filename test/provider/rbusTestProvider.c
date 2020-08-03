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
#include <assert.h>
#include <rbus.h>
#include <rtList.h>
#include "../common/runningParamHelper.h"
#include "../common/testValueHelper.h"


/*value tests*/
TestValueProperty* gTestValues;

/*
 * Generic Data Model Tree Structure
 */
#define MAX_ALIAS_LEN 64
#define RBUS_ELEMENT_TYPE_OBJECT 1000
#define RBUS_ELEMENT_TYPE_TABLE_ROW 1001

struct Node;
struct TableNode;
struct TableRowNode;
struct PropertyNode;

typedef rbusError_t (*addTableRowHandler_t)(struct TableNode* tableNode, char const* alias, uint32_t* instNum);
typedef rbusError_t (*removeTableRowHandler_t)(struct TableRowNode* rowNode);

typedef struct Node
{
    rbusElementType_t type;
    char name[MAX_ALIAS_LEN];
    struct Node* parent;
    rtList children;
} Node;

typedef struct TableNode
{
    struct Node node;
    addTableRowHandler_t addHandler;
    removeTableRowHandler_t removeHandler;
    uint32_t instNum;
} TableNode;

typedef struct TableRowNode
{
    struct Node node;
    uint32_t instNum;
    /*row alias stored in Node.name*/
} TableRowNode;

typedef struct PropertyNode
{
    struct Node node;
    rbusProperty_t prop;
} PropertyNode;

Node* getNode(Node* startNode, char const* fullName)
{
    char* tokName;
    char* token;
    Node* node = NULL;

    tokName = strdup(fullName);
    token = strtok(tokName, ".");

    while(token)
    {
        if(!node)
        {
            if(strcmp(startNode->name, token)==0)
                node = startNode;
        }
        else
        {
            Node* nextNode = NULL;
            Node* child;
            rtListItem item;
            void* data;

            rtList_GetFront(node->children, &item);

            while(item)
            {
                rtListItem_GetData(item, &data);

                child = (Node*)data;

                if(child->type == RBUS_ELEMENT_TYPE_TABLE_ROW)
                {
                    /*check if row alias matches
                      alias is stored in the name*/
                    if(strlen(child->name) > 0)
                    {
                        char alias[MAX_ALIAS_LEN+2];

                        /*tr-069: the alias must be wrapped in square brackets*/
                        snprintf(alias, MAX_ALIAS_LEN+2, "[%s]", child->name);

                        if(strcmp(alias, token)==0)
                        {
                            nextNode = child;
                        }
                    }

                    /*check if row instance number matches*/
                    if(!nextNode)
                    {
                        char number[MAX_ALIAS_LEN];

                        snprintf(number, MAX_ALIAS_LEN, "%d", ((TableRowNode*)child)->instNum);

                        if(strcmp(number, token)==0)
                        {
                            nextNode = child;
                        }
                    }
                }
                else if(strcmp(child->name, token)==0)
                {
                    nextNode = child;
                }

                if(nextNode)
                    break;

                rtListItem_GetNext(item, &item);
            }

            if(nextNode)
            {
                node = nextNode;
            }
            else
            {
                free(tokName);
                return NULL;
            }
        }
        token = strtok(NULL, ".");
    }
    free(tokName);
    return node;    
}

void destroyNode(Node* node)
{
    rtListItem item;
    void* data;

    /*destroy all children*/    
    rtList_GetFront(node->children, &item);

    while(item)
    {
        rtListItem_GetData(item, &data);
        rtListItem_GetNext(item, &item);

        destroyNode(data);
    }
    rtList_Destroy(node->children, NULL);

    /*release property*/
    if(node->type == RBUS_ELEMENT_TYPE_PROPERTY)
    {
        rbusProperty_Release(((PropertyNode*)node)->prop);
    }

    /*remove node from parent children list*/
    if(node->parent)
    {
        rtList_GetFront(node->parent->children, &item);

        /*remove row from table*/
        while(item)
        {
            rtListItem_GetData(item, &data);

            if(data == node)
            {
                rtList_RemoveItem(node->parent->children, item, NULL);
                break;
            }

            rtListItem_GetNext(item, &item);
        }
    }

    /*and of course*/
    free(node);
}

rbusError_t defaultRemoveTableRowHandler(TableRowNode* rowNode)
{
    destroyNode((Node*)rowNode);
    return RBUS_ERROR_SUCCESS;
}

Node* createNode(Node* parent, rbusElementType_t type, char const* name)
{
    Node* node;
    size_t nodeSize;

    switch((int)type)
    {
        case RBUS_ELEMENT_TYPE_TABLE: 
        nodeSize = sizeof(struct TableNode); 
        break;

        case RBUS_ELEMENT_TYPE_TABLE_ROW:
        nodeSize = sizeof(struct TableRowNode); 
        break;

        case RBUS_ELEMENT_TYPE_PROPERTY:
        nodeSize = sizeof(struct PropertyNode); 
        break;

        default:
        nodeSize = sizeof(struct Node);
        break;
    }

    node = calloc(1, nodeSize);

    node->type = type;

    if(name)
    {
        strncpy(node->name, name, MAX_ALIAS_LEN);
    }

    if(parent)
    {
        node->parent = parent;
        rtList_PushBack(parent->children, node, NULL);
    }

    rtList_Create(&node->children);

    return node;
}

TableNode* createTableNode(Node* parent, char const* name, addTableRowHandler_t addHandler)
{
    TableNode* node = (TableNode*)createNode(parent, RBUS_ELEMENT_TYPE_TABLE, name);
    node->addHandler = addHandler;
    node->removeHandler = defaultRemoveTableRowHandler;
    node->instNum = 1;
    return node;
}

rbusError_t createTableRowNode(Node* parent, char const* alias, uint32_t* instNum, TableRowNode** pnode)
{
    *pnode = NULL;

    assert(parent->type == RBUS_ELEMENT_TYPE_TABLE);

    /*verify alias not in use*/
    if(alias && strlen(alias))
    {
        rtListItem item;

        rtList_GetFront(parent->children, &item);

        while(item)
        {
            Node* child;

            rtListItem_GetData(item, (void**)&child);

            assert(child->type == RBUS_ELEMENT_TYPE_TABLE_ROW);

            if(child->type == RBUS_ELEMENT_TYPE_TABLE_ROW)
            {
                if(child->name && strlen(child->name) && strcmp(child->name, alias)==0)
                {
                    return RBUS_ERROR_ELEMENT_NAME_DUPLICATE;
                }
            }
            rtListItem_GetNext(item, &item);
        }
    }
    
    *pnode = (TableRowNode*)createNode(parent, RBUS_ELEMENT_TYPE_TABLE_ROW, alias);
    
    *instNum = (*pnode)->instNum = ((TableNode*)parent)->instNum++;

    return RBUS_ERROR_SUCCESS;
}

PropertyNode* createPropertyNode(Node* parent, char const* name)
{
    rbusValue_t val;
    PropertyNode* node = (PropertyNode*)createNode(parent, RBUS_ELEMENT_TYPE_PROPERTY, name);
    rbusValue_Init(&val);
    rbusValue_SetString(val, "");
    rbusProperty_Init(&node->prop, NULL, val);
    rbusValue_Release(val);
    return node;
}

/*
 * End Generic Data Model Tree Structure
 */

/*
 * Application Data Model Instantiation
 */
//TODO handle filter matching

int subscribed1 = 0;
int subscribed2 = 0;

Node* gRootNode;

rbusError_t addTable3RowHandler(TableNode* tableNode, char const* alias, uint32_t* instNum)
{
    TableRowNode* rowNode;
    rbusError_t err;

    err = createTableRowNode((Node*)tableNode, alias, instNum, &rowNode);

    if(err != RBUS_ERROR_SUCCESS)
        return err;

    /*add properties and tables to the new row object*/
    createPropertyNode((Node*)rowNode, "data");

    return RBUS_ERROR_SUCCESS;
}

rbusError_t addTable2RowHandler(TableNode* tableNode, char const* alias, uint32_t* instNum)
{
    TableRowNode* rowNode;
    rbusError_t err;

    err = createTableRowNode((Node*)tableNode, alias, instNum, &rowNode);

    if(err != RBUS_ERROR_SUCCESS)
        return err;

    /*add properties and tables to the new row object*/
    createTableNode((Node*)rowNode, "Table3", addTable3RowHandler);
    createPropertyNode((Node*)rowNode, "data");

    return RBUS_ERROR_SUCCESS;
}

rbusError_t addTable1RowHandler(TableNode* tableNode, char const* alias, uint32_t* instNum)
{
    TableRowNode* rowNode;
    rbusError_t err;

    err = createTableRowNode((Node*)tableNode, alias, instNum, &rowNode);

    if(err != RBUS_ERROR_SUCCESS)
        return err;

    /*add properties and tables to the new row object*/
    createTableNode((Node*)rowNode, "Table2", addTable2RowHandler);
    createPropertyNode((Node*)rowNode, "data");

    return RBUS_ERROR_SUCCESS;
}

void initNodeTree()
{
    gRootNode = createNode(NULL, RBUS_ELEMENT_TYPE_OBJECT, "Device");
    Node* pTestProvider = createNode(gRootNode, RBUS_ELEMENT_TYPE_OBJECT, "TestProvider");
    createTableNode(pTestProvider, "Table1", addTable1RowHandler);
}

Node* getNodeWithTypeVerification(Node* startNode, rbusElementType_t type, char const* name)
{
    Node* node;

    node = getNode(startNode, name);

    if(!node)
    {
        printf("provider: node not found %s\n", name);
        return NULL;
    }

    if(node->type != type)
    {
        printf("provider: node type invalid %s\n", name);
        return NULL;
    }

    return node;
}

rbusError_t tableAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
    (void)handle;

    printf("%s %s %s\n", __FUNCTION__, tableName, aliasName);

    TableNode* node = (TableNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_TABLE, tableName);

    if(node)
    {
        return node->addHandler(node, aliasName, instNum);
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t tableRemoveRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
    (void)handle;

    printf("%s %s\n", __FUNCTION__, rowName);

    TableRowNode* node = (TableRowNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_TABLE_ROW, rowName);

    if(node)
    {
        TableNode* table = (TableNode*)node->node.parent;

        return table->removeHandler(node);
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t dataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* propName = rbusProperty_GetName(property);

    printf("%s %s\n", __FUNCTION__, propName);

    PropertyNode* node = (PropertyNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_PROPERTY, propName);

    if(node)
    {
        char* sVal = rbusValue_ToString(rbusProperty_GetValue(node->prop), NULL, 0);
        printf("%s %s: node found with value=%s\n", __FUNCTION__, propName, sVal);
        free(sVal);

        rbusProperty_SetValue(property, rbusProperty_GetValue(node->prop));

        return RBUS_ERROR_SUCCESS;
    }     
    else
    {
        printf("%s %s: node not found\n" , __FUNCTION__, propName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t dataSetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    (void)handle;
    (void)options;

    char const* propName = rbusProperty_GetName(property);

    printf("%s %s\n", __FUNCTION__, propName);

    PropertyNode* node = (PropertyNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_PROPERTY, propName);

    if(node)
    {
        rbusProperty_SetValue(node->prop, rbusProperty_GetValue(property));
        return RBUS_ERROR_SUCCESS;
    }     
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t resetTablesSetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    printf("%s called for property %s\n", __FUNCTION__, name);

    if(rbusValue_GetBoolean(rbusProperty_GetValue(property)) == true)
    {
        printf("%s resetting table data\n", __FUNCTION__);
        destroyNode(gRootNode);
        initNodeTree();
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;

    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp("Device.TestProvider.Event1!", eventName))
    {
        subscribed1 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp("Device.TestProvider.Event2!", eventName))
    {
        subscribed2 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getValueHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)opts;

    printf("%s called for property %s\n", __FUNCTION__, name);

    TestValueProperty* data = gTestValues;
    while(data->name)
    {
        if(strcmp(name,data->name)==0)
        {
            rbusProperty_SetValue(property, data->values[0]);
            break;
        }
        data++;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t setValueHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    printf("%s called for property %s\n", __FUNCTION__, name);

    TestValueProperty* data = gTestValues;
    while(data->name)
    {
        if(strcmp(name,data->name)==0)
        {
            rbusValue_SetPointer(&data->values[0], rbusProperty_GetValue(property));
            break;
        }
        data++;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    static uint32_t count = 0;
    (void)handle;
    (void)opts;

    char buff[100];
    snprintf(buff, 100, "value %d", count++/2);//fake a value change every 3rd time the getHandler is called
    printf("Called get handler for [%s=%s]\n", rbusProperty_GetName(property), buff);

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, buff);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCIntHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    char const* name = rbusProperty_GetName(property);
    int index = atoi(&name[strlen(name)-1]);

    /*fake a value change every 'myfreq' times this function is called*/
    static int32_t mydata[6] = {0,0,0,0,0,0};  /*the actual value to send back*/
    static int32_t mydelta[6] = {1,1,1,1,1,1}; /*how much to change the value by*/
    static int32_t mycount[6] = {0,0,0,0,0,0}; /*number of times this function called*/
    static int32_t myfreq = 2;  /*number of times this function called before changing value*/
    static int32_t mymin = 0, mymax=5; /*keep value between mymin and mymax*/

    rbusValue_t value;

    mycount[index]++;
    if((mycount[index] % myfreq) == 0) 
    {
        mydata[index] += mydelta[index];
        if(mydata[index] == mymax)
            mydelta[index] = -1;
        else if(mydata[index] == mymin)
            mydelta[index] = 1;
    }

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, mydata[index]);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    printf("getVCIntHandler [%s]=[%d]\n", name, mydata[index]);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCStrHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    char const* name = rbusProperty_GetName(property);
    int index = atoi(&name[strlen(name)-1]);

    /*fake a value change every 'myfreq' times this function is called*/
    static int32_t mydata[6] = {0,0,0,0,0,0};  /*the actual value to send back*/
    static int32_t mydelta[6] = {1,1,1,1,1,1}; /*how much to change the value by*/
    static int32_t mycount[6] = {0,0,0,0,0,0}; /*number of times this function called*/
    static int32_t myfreq = 2;  /*number of times this function called before changing value*/
    static int32_t mymin = 0, mymax=5; /*keep value between mymin and mymax*/

    static char* values[6] = {
        "aaaa", "bbbb", "cccc", "dddd", "eeee", "ffff"
    };

    rbusValue_t value;

    mycount[index]++;
    if((mycount[index] % myfreq) == 0) 
    {
        mydata[index] += mydelta[index];
        if(mydata[index] == mymax)
            mydelta[index] = -1;
        else if(mydata[index] == mymin)
            mydelta[index] = 1;
    }

    rbusValue_Init(&value);
    rbusValue_SetString(value, values[mydata[index]]);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    printf("getVCStrHandler [%s]=[%s]\n", name, values[mydata[index]]);

    return RBUS_ERROR_SUCCESS;
}

typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
}MethodData;

static void* asyncMethodFunc(void *p)
{
    MethodData* data;
    rbusObject_t outParams;
    rbusValue_t value;
    rbusError_t err;
    char buff[256];

    printf("%s enter\n", __FUNCTION__);

    sleep(3);

    data = p;


    rbusObject_Init(&outParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, "MethodAsync2()");
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    snprintf(buff, 255, "Async Method Response inParams=%s\n", rbusValue_ToString(rbusObject_GetValue(data->inParams, "param1"), NULL, 0));
    rbusValue_SetString(value, buff);
    rbusObject_SetValue(outParams, "value", value);
    rbusValue_Release(value);
    
    printf("%s sending response\n", __FUNCTION__);
    err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_SUCCESS, outParams);
    if(err != RBUS_ERROR_SUCCESS)
    {
        printf("%s rbusMethod_SendAsyncResponse failed err:%d\n", __FUNCTION__, err);
    }

    rbusObject_Release(data->inParams);
    rbusObject_Release(outParams);

    free(data);

    printf("%s exit\n", __FUNCTION__);

    return NULL;
}

static rbusError_t methodHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;
    rbusValue_t value;

    printf("methodHandler called: %s\n", methodName);
    rbusObject_fwrite(inParams, 1, stdout);


    if(strstr(methodName, "Method1()"))
    {
        rbusValue_Init(&value);
        rbusValue_SetString(value, "Method1()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        printf("methodHandler success\n");
        return RBUS_ERROR_SUCCESS;
    }
    else
    if(strstr(methodName, "Method2()"))
    {
        rbusValue_Init(&value);
        rbusValue_SetString(value, "Method2()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        printf("methodHandler success\n");
        return RBUS_ERROR_SUCCESS;
    }
    else
    if(strstr(methodName, "MethodAsync1()"))
    {
        sleep(4);
        rbusValue_Init(&value);
        rbusValue_SetString(value, "MethodAsync1()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        printf("methodHandler success\n");
        return RBUS_ERROR_SUCCESS;
    }
    else
    if(strstr(methodName, "MethodAsync2()"))
    {
        pthread_t pid;
        MethodData* data = malloc(sizeof(MethodData));
        data->asyncHandle = asyncHandle;
        data->inParams = inParams;
        rbusObject_Retain(inParams);
        if(pthread_create(&pid, NULL, asyncMethodFunc, data) || pthread_detach(pid))
        {
            printf("%s failed to spawn thread\n", __FUNCTION__);
            return RBUS_ERROR_BUS_ERROR;
        }
        return RBUS_ERROR_ASYNC_RESPONSE;
    }
    printf("methodHandler fail\n");
    return RBUS_ERROR_BUS_ERROR;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    char componentName[] = "TestProvider";
    int eventCounts[2]={0,0};
    int j;
    #define numDataElems 26

    rbusDataElement_t dataElement[numDataElems] = {
        {"Device.TestProvider.Event1!", RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, eventSubHandler, NULL}},
        {"Device.TestProvider.Event2!", RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, eventSubHandler, NULL}},
        /*testing value-change filter for Int32 and Strings only, for now*/
        {"Device.TestProvider.VCParam", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamInt0", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamInt1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamInt2", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamInt3", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamInt4", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamInt5", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamStr0", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamStr1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamStr2", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamStr3", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamStr4", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.VCParamStr5", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,NULL, NULL}},
        {"Device.TestProvider.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, eventSubHandler, NULL}},
        {"Device.TestProvider.Table1.{i}.Table2.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, eventSubHandler, NULL}},
        {"Device.TestProvider.Table1.{i}.Table2.{i}.Table3.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, eventSubHandler, NULL}},
        {"Device.TestProvider.Table1.{i}.data", RBUS_ELEMENT_TYPE_PROPERTY, {dataGetHandler, dataSetHandler, NULL, NULL, NULL, NULL}},
        {"Device.TestProvider.Table1.{i}.Table2.{i}.data", RBUS_ELEMENT_TYPE_PROPERTY, {dataGetHandler, dataSetHandler, NULL, NULL, NULL, NULL}},
        {"Device.TestProvider.Table1.{i}.Table2.{i}.Table3.{i}.data", RBUS_ELEMENT_TYPE_PROPERTY, {dataGetHandler, dataSetHandler, NULL, NULL, NULL, NULL}},
        {"Device.TestProvider.ResetTables", RBUS_ELEMENT_TYPE_PROPERTY, {NULL, resetTablesSetHandler, NULL, NULL, NULL, NULL}},
        {"Device.TestProvider.Table1.{i}.Method1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.TestProvider.Table1.{i}.Table2.{i}.Method2()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.TestProvider.MethodAsync1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.TestProvider.Table1.{i}.MethodAsync2()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}}
    };

    printf("provider: start\n");

    TestValueProperties_Init(&gTestValues);

    initNodeTree();

    rc = rbus_open(&handle, componentName);
    printf("provider: rbus_open=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit2;

    TestValueProperty* data = gTestValues;
    while(data->name)
    {
        rbusDataElement_t el = { data->name, RBUS_ELEMENT_TYPE_PROPERTY, {getValueHandler,setValueHandler,NULL,NULL,NULL,NULL}};
        rc = rbus_regDataElements(handle, 1, &el);
        printf("provider: rbus_regDataElements=%d\n", rc);
        if(rc != RBUS_ERROR_SUCCESS)
            goto exit1;
        data++;
    }
    
    rc = rbus_regDataElements(handle, numDataElems, dataElement);
    printf("provider: rbus_regDataElements=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit1;

    if(runningParamProvider_Init(handle, "Device.TestProvider.TestRunning") != RBUS_ERROR_SUCCESS)
    {
        printf("provider: failed to register running param\n");
        goto exit1;
    }
    printf("provider: waiting for consumer to start\n");
    while (!runningParamProvider_IsRunning())
    {
        usleep(10000);
    }

    printf("provider: running\n");
    while (runningParamProvider_IsRunning())
    {
        #define BUFF_LEN 100
        char buffer[BUFF_LEN];

        sleep(1);

        for(j=0; j<2; ++j)
        {
            if((j==0 && subscribed1) || (j==1 && subscribed2))
            {
                printf("publishing Event%d\n", j);

                snprintf(buffer, BUFF_LEN, "event %d data %d", j, eventCounts[j]);

                rbusObject_t data;
                rbusValue_t bufferVal;
                rbusValue_t indexVal;

                rbusValue_Init(&bufferVal);
                rbusValue_Init(&indexVal);

                rbusValue_SetString(bufferVal, buffer);
                rbusValue_SetInt32(indexVal, eventCounts[j]);

                rbusObject_Init(&data, NULL);
                rbusObject_SetValue(data, "buffer", bufferVal);
                rbusObject_SetValue(data, "index", indexVal);

                rbusEvent_t event;
                event.name = dataElement[j].name;
                event.data = data;
                event.type = RBUS_EVENT_GENERAL;

                rc = rbusEvent_Publish(handle, &event);

                rbusValue_Release(bufferVal);
                rbusValue_Release(indexVal);
                rbusObject_Release(data);

                if(rc != RBUS_ERROR_SUCCESS)
                    printf("provider: rbusEvent_Publish Event%d failed: %d\n", j, rc);

                eventCounts[j]++;
            }
        }
    }

    printf("provider: finishing\n");

    rc = rbus_unregDataElements(handle, numDataElems, dataElement);
    printf("provider: rbus_unregDataElements=%d\n", rc);

exit1:
    rc = rbus_close(handle);
    printf("provider: rbus_close=%d\n", rc);

exit2:

    TestValueProperties_Release(gTestValues);
    destroyNode(gRootNode);

    printf("provider: exit\n");
    return rc;
}
