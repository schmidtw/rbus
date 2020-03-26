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
#include <string.h>
#include <stdlib.h>
#include <rbus.h>
#include <rbus_element.h>
#include <rbus_valuechange.h>

#include <rbus_core.h>
#include <rbus_marshalling.h>
#include <rbus_session_mgr.h>

#include <rtVector.h>

//******************************* MACROS *****************************************//
#define UNUSED1(a)              (void)(a)
#define UNUSED2(a,b)            UNUSED1(a),UNUSED1(b)
#define UNUSED3(a,b,c)          UNUSED1(a),UNUSED2(b,c)
#define UNUSED4(a,b,c,d)        UNUSED1(a),UNUSED3(b,c,d)
#define UNUSED5(a,b,c,d,e)      UNUSED1(a),UNUSED4(b,c,d,e)
#define UNUSED6(a,b,c,d,e,f)    UNUSED1(a),UNUSED5(b,c,d,e,f)

#define MAX_COMPS_PER_PROCESS           5
#define FALSE                           0
#define TRUE                            1
#define VALUE_CHANGE_POLLING_PERIOD     2 //seconds
//********************************************************************************//


//******************************* STRUCTURES *************************************//

struct _rbusHandle_t
{
    int             in_use;
    char*           comp_name;
    elementNode*    elementRoot;
    rtVector        event_subs; /*FIXME - this needs to be an associative map instead of list/vector*/
};

#define comp_info struct _rbusHandle_t

comp_info comp_array[MAX_COMPS_PER_PROCESS] = {
    {0,"", NULL, NULL},
    {0,"", NULL, NULL},
    {0,"", NULL, NULL},
    {0,"", NULL, NULL},
    {0,"", NULL, NULL}
};

typedef enum _rbus_legacy_support
{
    RBUS_LEGACY_STRING = 0,    /**< Null terminated string                                           */
    RBUS_LEGACY_INT,           /**< Integer (2147483647 or -2147483648) as String                    */
    RBUS_LEGACY_UNSIGNEDINT,   /**< Unsigned Integer (ex: 4,294,967,295) as String                   */
    RBUS_LEGACY_BOOLEAN,       /**< Boolean as String (ex:"true", "false"                            */
    RBUS_LEGACY_DATETIME,      /**< ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ) as String                 */
    RBUS_LEGACY_BASE64,        /**< Base64 representation of data as String                          */
    RBUS_LEGACY_LONG,          /**< Long (ex: 9223372036854775807 or -9223372036854775808) as String */
    RBUS_LEGACY_UNSIGNEDLONG,  /**< Unsigned Long (ex: 18446744073709551615) as String               */
    RBUS_LEGACY_FLOAT,         /**< Float (ex: 1.2E-38 or 3.4E+38) as String                         */
    RBUS_LEGACY_DOUBLE,        /**< Double (ex: 2.3E-308 or 1.7E+308) as String                      */
    RBUS_LEGACY_BYTE,
    RBUS_LEGACY_NONE
} rbusLegacyDataType_t;

typedef enum _rbus_legacy_returs {
    RBUS_LEGACY_ERR_SUCCESS = 100,
    RBUS_LEGACY_ERR_MEMORY_ALLOC_FAIL = 101,
    RBUS_LEGACY_ERR_FAILURE = 102,
    RBUS_LEGACY_ERR_NOT_CONNECT = 190,
    RBUS_LEGACY_ERR_TIMEOUT = 191,
    RBUS_LEGACY_ERR_NOT_EXIST = 192,
    RBUS_LEGACY_ERR_NOT_SUPPORT = 193,
    RBUS_LEGACY_ERR_OTHERS
} rbusLegacyReturn_t;

//********************************************************************************//

//******************************* INTERNAL FUNCTIONS *****************************//
static void rbusEventSubscription_free(void* p)
{
    rbusEventSubscription_t* sub = (rbusEventSubscription_t*)p;
    free((void*)sub->event_name);
    free(sub);
}

static rbusEventSubscription_t* rbusEventSubscription_find(rtVector event_subs, const char* event_name)
{
    /*FIXME - convert to map */
    size_t i;
    for(i=0; i < rtVector_Size(event_subs); ++i)
    {
        rbusEventSubscription_t* sub = (rbusEventSubscription_t*)rtVector_At(event_subs, i);
        if(sub && !strcmp(sub->event_name, event_name))
            return sub;
    }
    printf("rbusEventSubscription_find error: can't find %s\n", event_name);
    return NULL;
}

static void rbusTlv_appendToMessage(rbus_Tlv_t* tlv, rtMessage msg)
{
    rbus_AppendString(msg, tlv->name);
    rbus_AppendBinaryData(msg, tlv->value, tlv->length);
    rbus_AppendInt32(msg, tlv->type);
}

static void rbusTlv_popFromMessage(rbus_Tlv_t* tlv, rtMessage msg)
{
    rbus_PopString(msg, (const char**) &tlv->name);
    rbus_PopBinaryData(msg, (const void **)&tlv->value, &tlv->length);
    rbus_PopInt32(msg, (int*) &tlv->type);
}

//******************************* CALLBACKS *************************************//

int _event_subscribe_callback_handler(const char * object,  const char * event_name, const char * listener, int added, void * user_data)
{
    rbusHandle_t handle = (rbusHandle_t)user_data;
    comp_info* ci = (comp_info*)user_data;

    UNUSED2(object,listener);

    printf("event subscribe callback for [%s] event!\n", event_name);

    //determine if event_name is a path to a parameter, object table, or general event
    elementNode* el = retrieveElement(ci->elementRoot, event_name);
    if(el)
    {
        printf("found element of type %d\n", el->type);

        if(el->type == ELEMENT_TYPE_PARAMETER)
        {
            //subscribing to a parameter value-change notification
            if(added)
                rbusValueChange_AddParameter(handle, el, event_name);
            else
                rbusValueChange_RemoveParameter(handle, el, event_name);
        }
        else if(el->type == ELEMENT_TYPE_EVENT)
        {
            //subscribing to a general event notification
            if(el->cbTable && el->cbTable->rbus_eventSubHandler)
            {
                rbus_eventSubAction_e action;
                if(added)
                    action = RBUS_EVENT_ACTION_SUBSCRIBE;
                else
                    action = RBUS_EVENT_ACTION_UNSUBSCRIBE;
                (*el->cbTable->rbus_eventSubHandler)(action, event_name, NULL);
            }
        }
        else if(el->type == ELEMENT_TYPE_OBJECT_TABLE)
        {
            //subscribing to an object instance created/deleted notification
            //TODO
        }
    }
    else
    {
        printf("event subscribe callback: unexpected! element not found\n");
    }
    return 0;
}

int _event_callback_handler (const char * object_name,  const char * event_name, rtMessage message, void * user_data)
{
    rbusEventSubscription_t* subscription;
    rbusEventHandler_t handler;
    rbus_Tlv_t tlv;

    printf("Received event callback: object_name=%s event_name=%s\n", 
        object_name, event_name);

    subscription = (rbusEventSubscription_t*)user_data;

    if(!subscription || !subscription->handle || !subscription->handler)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    handler = (rbusEventHandler_t)subscription->handler;
    rbusTlv_popFromMessage(&tlv, message);

    //TODO shouldn't object_name be passed to handler
    (*handler)(subscription->handle, subscription, &tlv);

    return 0;
}

void _parse_rbusData_to_tlv (const char* pBuff, rbusLegacyDataType_t legacyType, rbus_Tlv_t *pParamTlv)
{
    if (pBuff && pParamTlv)
    {
        switch (legacyType)
        {
            case RBUS_LEGACY_STRING:
            {
                pParamTlv->value = strdup(pBuff);
                pParamTlv->length = strlen(pBuff) + 1;
                pParamTlv->type = RBUS_STRING;
                break;
            }
            case RBUS_LEGACY_INT:
            {
                int tmp = atoi(pBuff);
                pParamTlv->length = sizeof (tmp);
                pParamTlv->type = RBUS_INT32;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_UNSIGNEDINT:
            {
                unsigned int tmp = (unsigned int) atoi(pBuff);
                pParamTlv->length = sizeof (tmp);
                pParamTlv->type = RBUS_UINT32;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_BOOLEAN:
            {
                rbusBool_t tmp = RBUS_FALSE;
                if ((0 == strncasecmp("true", pBuff, 4)) || (0 == strncasecmp("1", pBuff, 1)))
                    tmp = RBUS_TRUE;
                else if ((0 == strncasecmp("false", pBuff, 5)) || (0 == strncasecmp("0", pBuff, 1)))
                    tmp = RBUS_FALSE;

                pParamTlv->length = sizeof (rbusBool_t);
                pParamTlv->type = RBUS_BOOLEAN;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_DATETIME:
            {
                pParamTlv->value = strdup(pBuff);
                pParamTlv->length = strlen(pBuff) + 1;
                pParamTlv->type = RBUS_DATE_TIME;
                break;
            }
            case RBUS_LEGACY_BASE64:
            {
                pParamTlv->value = strdup(pBuff);
                pParamTlv->length = strlen(pBuff) + 1;
                pParamTlv->type = RBUS_BASE64;
                break;
            }
            case RBUS_LEGACY_LONG:
            {
                long long tmp = atoll(pBuff);
                pParamTlv->length = sizeof (tmp);
                pParamTlv->type = RBUS_INT64;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_UNSIGNEDLONG:
            {
                unsigned long long tmp = (unsigned long long) atoll(pBuff);
                pParamTlv->length = sizeof (tmp);
                pParamTlv->type = RBUS_INT64;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_FLOAT:
            {
                float tmp = (float) atof (pBuff);
                pParamTlv->length = sizeof (tmp);
                pParamTlv->type = RBUS_FLOAT;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_DOUBLE:
            {
                double tmp = atof (pBuff);
                pParamTlv->length = sizeof (tmp);
                pParamTlv->type = RBUS_DOUBLE;
                pParamTlv->value = (void*) malloc(pParamTlv->length);
                memcpy(pParamTlv->value, &tmp, pParamTlv->length);
                break;
            }
            case RBUS_LEGACY_BYTE:
            {
                pParamTlv->value = strdup(pBuff);
                pParamTlv->length = strlen(pBuff);
                pParamTlv->type = RBUS_BYTE;
                break;
            }
            case RBUS_LEGACY_NONE:
            {
                pParamTlv->value = NULL;
                pParamTlv->length = 0;
                pParamTlv->type = RBUS_NONE;
                break;
            }
        }
    }
    else if ((pBuff == NULL) && (pParamTlv != NULL))
    {
        pParamTlv->value = NULL;
        pParamTlv->length = 0;
        pParamTlv->type = RBUS_NONE;
    }
}

void _set_callback_handler (rbusHandle_t rbusHandle, rtMessage request, rtMessage *response)
{
    rbus_errorCode_e rc = 0;
    int sessionId = 0;
    int numTlvs = 0;
    int loopCnt = 0;
    rbusBool_t commit = RBUS_TRUE;
    char* pCompName = NULL;
    char* pIsCommit = NULL;
    char* pFailedElement = NULL;
    rbus_Tlv_t* pParamTlv = NULL;
    comp_info* pCompInfo = (comp_info*)rbusHandle;

    rbus_PopInt32(request, &sessionId);
    rbus_PopString(request, (const char**) &pCompName);
    rbus_PopInt32(request, &numTlvs);

    if(numTlvs > 0)
    {
        elementNode* pElmntNode = NULL;
        rbus_callbackTable_t *pCBTable = NULL;

        pParamTlv = (rbus_Tlv_t*)calloc(numTlvs, sizeof(rbus_Tlv_t));
        for (loopCnt = 0; loopCnt < numTlvs; loopCnt++)
        {
            rbus_PopString(request, (const char**) &pParamTlv[loopCnt].name);
            rbus_PopInt32(request, (int*) &pParamTlv[loopCnt].type);
            rbus_PopBinaryData(request, (const void **)&pParamTlv[loopCnt].value, &pParamTlv[loopCnt].length);
        }

        rbus_PopString(request, (const char**) &pIsCommit);

        /* Since we set as string, this needs to compared with string..
         * Otherwise, just the #define in the top for TRUE/FALSE should be used.
         */
        if (strncasecmp("FALSE", pIsCommit, 5) == 0)
            commit = RBUS_FALSE;

        for (loopCnt = 0; loopCnt < numTlvs; loopCnt++)
        {
            /* Retrive the element node */
            pElmntNode = retrieveElement(pCompInfo->elementRoot, pParamTlv[loopCnt].name);
            if(pElmntNode != NULL)
            {
                pCBTable = pElmntNode->cbTable;
                if(pCBTable && pCBTable->rbus_setHandler)
                {
                    /* FIXME: Why does this handler needs number_of_tlv as first argument when it is always 1.
                     * we are always getting the individual element handler and calling it. so it is always 1.
                     */
                    rc = pCBTable->rbus_setHandler (1, &pParamTlv[loopCnt], sessionId, commit, pCompName);
                    if (rc != RBUS_ERROR_SUCCESS)
                    {
                        printf ("Set Failed for %s; Owner Component returned Error\n", pParamTlv[loopCnt].name);
                        pFailedElement = pParamTlv[loopCnt].name;
                        break;
                    }
                }
                else
                {
                    printf ("Set Failed for %s; No Handler found\n", pParamTlv[loopCnt].name);
                    rc = RBUS_ERROR_ACCESS_NOT_ALLOWED;
                    pFailedElement = pParamTlv[loopCnt].name;
                    break;
                }
            }
            else
            {
                printf ("Set Failed for %s; No Element registered\n", pParamTlv[loopCnt].name);
                rc = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
                pFailedElement = pParamTlv[loopCnt].name;
                break;
            }
        }
    }
    else
    {
        printf ("Set Failed as %s did not send any input\n", pCompName);
        rc = RBUS_ERROR_INVALID_INPUT;
        pFailedElement = pCompName;
    }

    rtMessage_Create(response);
    rbus_AppendInt32(*response, (int) rc);
    if (pFailedElement)
        rbus_AppendString(*response, pFailedElement);

    if(pParamTlv)
    {
        free(pParamTlv);
    }
    return;
}

void _get_callback_handler (rbusHandle_t rbusHandle, rtMessage request, rtMessage *response)
{
    comp_info* ci = (comp_info*)rbusHandle;
    int paramSize = 1, i = 0;
    rbus_errorCode_e result = RBUS_ERROR_SUCCESS;
    const char *parameterNames = NULL;
    const char *pCompName = NULL;
    rbus_Tlv_t* tlv = NULL;

    rbus_PopString(request, &pCompName);
    rbus_PopInt32(request, &paramSize);

    printf("Param Size [%d]\n", paramSize);

    if(paramSize > 0)
    {
        tlv = (rbus_Tlv_t*) calloc(paramSize, sizeof(rbus_Tlv_t));

        for(i = 0; i < paramSize; i++)
        {
            elementNode* el = NULL;
            rbus_callbackTable_t *table = NULL;

            parameterNames = NULL;
            rbus_PopString(request, &parameterNames);

            printf("Param Name [%d]:[%s]\n", i, parameterNames);

            tlv[i].name = strdup (parameterNames);

            //Do a look up and call the corresponding method
            el = retrieveElement(ci->elementRoot, tlv[i].name);
            if(el != NULL)
            {
                printf("Retrieved [%s]\n", tlv[i].name);

                table = el->cbTable;
                if(table && table->rbus_getHandler)
                {
                    printf("Table and CB exists for [%s], call the CB!\n", tlv[i].name);

                    result = table->rbus_getHandler((void *)rbusHandle, 1, &tlv[i], ci->comp_name);

                    if (result != RBUS_ERROR_SUCCESS)
                    {
                        printf("called CB with result [%d]\n", result);
                        break;
                    }
                }
                else
                {
                    printf("Element retrieved, but no cb installed for [%s]!\n", tlv[i].name);
                    result = RBUS_ERROR_ACCESS_NOT_ALLOWED;
                    break;
                }
            }
            else
            {
                printf("Not able to retrieve element [%s]\n", tlv[i].name);
                result = RBUS_ERROR_ACCESS_NOT_ALLOWED;
                break;
            }
        }

        rtMessage_Create(response);
        rbus_AppendInt32(*response, (int) result);
        if (result == RBUS_ERROR_SUCCESS)
        {
            rbus_AppendInt32(*response, paramSize);
            for(i = 0; i < paramSize; i++)
            {
                printf("Add Name: [%s]\n", tlv[i].name);
                rbus_AppendString(*response, tlv[i].name);
                printf("Add Type: [%d]\n", tlv[i].type);
                rbus_AppendInt32(*response, (int)tlv[i].type);
                printf("Add Bin data\n");
                rbus_AppendBinaryData(*response, tlv[i].value, tlv[i].length);
            }
        }
       
        /* Free the memory, regardless of success or not.. */
        for (i = 0; i < paramSize; i++)
        {
            if (tlv[i].name)
                free (tlv[i].name);
            if (tlv[i].value)
                free (tlv[i].value);
        }
        free (tlv);
    }
    else
    {
        printf ("Get Failed as %s did not send any input\n", pCompName);
        result = RBUS_ERROR_INVALID_INPUT;
        rtMessage_Create(response);
        rbus_AppendInt32(*response, (int) result);
    }

    return;
}

int _callback_handler(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response)
{
    rbusHandle_t handle = (rbusHandle_t)user_data;

    printf("Received callback for [%s]\n", destination);

    if(!strcmp(method, METHOD_GETPARAMETERVALUES))
    {
        _get_callback_handler (handle, request, response);
    }
    else if(!strcmp(method, METHOD_SETPARAMETERVALUES))
    {
        _set_callback_handler (handle, request, response);
    }
    else
    {
        printf("unhandled callback for [%s] method!\n", method);
    }

    return 0;
}

//******************************* Bus Initialization *****************************//
rbus_errorCode_e rbus_open (rbusHandle_t* rbusHandle, char *componentName)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    int foundIndex = -1;
    int  i = 0;

    if((rbusHandle == NULL) || (componentName == NULL))
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    *rbusHandle = NULL;

    /*
        Per spec: If a component calls this API more than once, any previous busHandle 
        and all previous data element registrations will be canceled.
    */
    for(i = 0; i < MAX_COMPS_PER_PROCESS; i++)
    {
        if(comp_array[i].in_use && strcmp(componentName, comp_array[i].comp_name) == 0)
        {
            rbus_close(&comp_array[i]);
        }
    }

    /*  Find open item in array: 
        TODO why can't this just be a rtVector we push/remove from? */
    for(i = 0; i < MAX_COMPS_PER_PROCESS; i++)
    {
        if(!comp_array[i].in_use)
        {
            foundIndex = i;
            break;
        }
    }

    if(foundIndex == -1)
    {
        printf("<%s>: Exceeded the allowed number of components per process!\n", __FUNCTION__);
        errorcode = RBUS_ERROR_OUT_OF_RESOURCES;
    }
    else
    {
        /*
            Per spec: the first component that calls rbus_open will establishes a new socket connection to the bus broker.
            Note: rbus_openBrokerConnection returns RTMESSAGE_BUS_ERROR_INVALID_STATE if a connection is already established.
            We cannot expect our 1st call to rbus_openBrokerConnection to succeed, as another library in this process
            may have already called rbus_openBrokerConnection.  This would happen if the ccsp_message_bus is already 
            running with rbus-core in the same process.  Thus we must call again and check the return code.
        */
        err = rbus_openBrokerConnection(componentName);
        
        if( err != RTMESSAGE_BUS_SUCCESS && 
            err != RTMESSAGE_BUS_ERROR_INVALID_STATE/*connection already opened. which is allowed*/)
        {
            printf("<%s>: rbus_openBrokerConnection() fails with %d\n", __FUNCTION__, err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            rbusHandle_t handle = &comp_array[foundIndex];

            printf("<%s>: rbus_openBrokerConnection() Success!\n", __FUNCTION__);
            printf("<%s>: Try rbus_registerObj() for component base object [%s]!\n", __FUNCTION__, componentName);

            if((err = rbus_registerObj(componentName, _callback_handler, handle)) != RTMESSAGE_BUS_SUCCESS)
            {
                /*Note that this will fail if a previous rbus_open was made with the same componentName
                  because rbus_registerObj doesn't allow the same name to be registered twice.  This would
                  also fail if ccsp using rbus-core has registered the same object name */
                printf("<%s>: rbus_registerObj() fails with %d\n", __FUNCTION__, err);
                errorcode = RBUS_ERROR_BUS_ERROR;
            }
            else
            {
                printf("<%s>: rbus_registerObj() Success!\n", __FUNCTION__);
                comp_array[foundIndex].in_use = 1;
                comp_array[foundIndex].comp_name = strdup(componentName);
                *rbusHandle = handle;
                rtVector_Create(&comp_array[foundIndex].event_subs);
                //you really only need to call once per process but it doesn't hurt to call here
                rbusValueChange_SetPollingPeriod(VALUE_CHANGE_POLLING_PERIOD);
            }

        }
    }

    return errorcode;
}

rbus_errorCode_e rbus_close (rbusHandle_t rbusHandle)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    comp_info* ci = (comp_info*)rbusHandle;

    if(rbusHandle == NULL)
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(ci->event_subs)
    {
        while(rtVector_Size(ci->event_subs))
        {
            rbusEventSubscription_t* sub = (rbusEventSubscription_t*)rtVector_At(ci->event_subs, 0);
            if(sub)
            {
                rbusEvent_Unsubscribe(rbusHandle, sub->event_name);
            }
        }
        rtVector_Destroy(ci->event_subs, NULL);
        ci->event_subs = NULL;
    }

    rbusValueChange_Close(rbusHandle);//called before freeAllElements below

    if(ci->elementRoot)
    {
        freeAllElements(&(ci->elementRoot));
        //free(ci->elementRoot); valgrind reported this and I saw that freeAllElements actually frees this . leaving comment so others won't wonder why this is gone
        ci->elementRoot = NULL;
    }

    if((err = rbus_unregisterObj(ci->comp_name)) != RTMESSAGE_BUS_SUCCESS) //FIXME: shouldn't rbus_closeBrokerConnection be called even if this fails ?
    {
        printf("<%s>: rbus_unregisterObj() for [%s] fails with %d\n", __FUNCTION__, ci->comp_name, err);
        errorcode = RBUS_ERROR_INVALID_HANDLE;
    }
    else
    {
        int canClose = 1;
        int i;

        printf("<%s>: rbus_unregisterObj() for [%s] Success!!\n", __FUNCTION__, ci->comp_name);
        free(ci->comp_name);
        ci->comp_name = NULL;
        ci->in_use = 0;

        for(i = 0; i < MAX_COMPS_PER_PROCESS; i++)
        {
            if(comp_array[i].in_use)
            {
                canClose = 0;
                break;
            }
        }

        if(canClose)
        {
            if((err = rbus_closeBrokerConnection()) != RTMESSAGE_BUS_SUCCESS)
            {
                printf("<%s>: rbus_closeBrokerConnection() fails with %d\n", __FUNCTION__, err);
                errorcode = RBUS_ERROR_BUS_ERROR;
            }
            else
            {
                printf("<%s>: rbus_closeBrokerConnection() Success!!\n", __FUNCTION__);
            }
        }
    }

    return errorcode;
}

//************************* Data Element Registration **************************//
rbus_errorCode_e rbus_regDataElements (rbusHandle_t rbusHandle,
                            int numDataElements, rbus_dataElement_t *elements)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    rbus_dataElement_t* el = elements;

    comp_info* ci = (comp_info*)rbusHandle;

    if(ci->elementRoot == NULL)
    {
        printf("First Time, create the root node for [%s]!\n", ci->comp_name);
        ci->elementRoot = getEmptyElementNode();
        ci->elementRoot->name = strdup(ci->comp_name);
        printf("Root node created for [%s]\n", ci->elementRoot->name);
    }

    int elementCount = 0;
    for(elementCount = 0; elementCount < numDataElements; elementCount++)
    {
        elementNode* node;
        if((node = insertElement(&(ci->elementRoot), el)) == NULL)
        {
            printf("<%s>: failed to insert element [%s]!!\n", __FUNCTION__, el->elementName);
            errorcode = RBUS_ERROR_OUT_OF_RESOURCES;
            break;
        }
        else
        {
            if((err = rbus_addElement(ci->comp_name, el->elementName)) != RTMESSAGE_BUS_SUCCESS)
            {
                printf("<%s>: failed to add element with core [%s]!!\n", __FUNCTION__, el->elementName);
                errorcode = RBUS_ERROR_OUT_OF_RESOURCES;
                break;
            }
            else
            {
                printf("Element inserted successfully!\n");

                /*type was calculated inside insertElement above*/
                if(node->type == ELEMENT_TYPE_EVENT      /*obviously we register all events*/
                || node->type == ELEMENT_TYPE_PARAMETER) /*less obvious, we register all parameters too
                                                           so consumers can subscribe to value change event*/
                {
                    if((err = rbus_registerEvent(ci->comp_name, el->elementName, _event_subscribe_callback_handler, rbusHandle)) != RTMESSAGE_BUS_SUCCESS)
                    {
                        printf("<%s>: failed to register event with core [%s]!!\n", __FUNCTION__, el->elementName);
                        errorcode = RBUS_ERROR_OUT_OF_RESOURCES;
                        break;
                    }
                    else
                    {
                        printf("Event registered successfully! %s\n", el->elementName);
                    }
                }
            }
        }
        el++;
    }
    if(ci->elementRoot->child)  //Debug only
    {
        printRegisteredElements(ci->elementRoot->child, 0);
        printf("\n");
    }
    return errorcode;
}

rbus_errorCode_e rbus_unregDataElements (rbusHandle_t rbusHandle,
                            int numDataElements, rbus_dataElement_t *elements)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    rbus_dataElement_t* el = elements;

    comp_info* ci = (comp_info*)rbusHandle;
    int elementCount = 0;
    for(elementCount = 0; elementCount < numDataElements; elementCount++)
    {
        if((err = rbus_removeElement(ci->comp_name, el->elementName)) != RTMESSAGE_BUS_SUCCESS)
        {
            printf("<%s>: failed to remove element from core [%s]!!\n", __FUNCTION__, el->elementName);
            errorcode = RBUS_ERROR_ELEMENT_NAME_MISSING;
            break;
        }
        else
        {
            rbusValueChange_RemoveParameter(rbusHandle, NULL, el->elementName);//called before removeElement below

            if(removeElement(&(ci->elementRoot), el) < 0)
            {
                printf("<%s>: failed to remove element [%s]!!\n", __FUNCTION__, el->elementName);
                errorcode = RBUS_ERROR_ELEMENT_NAME_MISSING;
                break;
            }
            else
            {
                printf("Element removed successfully!\n");
            }
        }
        el++;
    }
    return errorcode;
}

//************************* Discovery related Operations *******************//
rbus_errorCode_e rbus_discoverComponentName (rbusHandle_t rbusHandle,
                            int numElements, char** elementNames,
                            int *numComponents, char ***componentName)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    if(rbusHandle == NULL)
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    *numComponents = 0;
    *componentName = 0;
    char **output = NULL;
    if(RTMESSAGE_BUS_SUCCESS == rbus_findMatchingObjects((const char**)elementNames,numElements,&output))
    {
        *componentName = output;
        *numComponents = numElements;    
    }
    else
    {
         printf("return from rbus_findMatchingObjects is not success\n");
    }
  
    return errorcode;
}

rbus_errorCode_e rbus_discoverComponentDataElements (rbusHandle_t rbusHandle,
                            char* name, rbusBool_t nextLevel,
                            int *numElements, char*** elementNames)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;

    char** val = NULL;
    rtMessage response;
    *numElements = 0;
    *elementNames = 0;
    UNUSED1(nextLevel);
    if((rbusHandle == NULL) || (name == NULL))
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbus_error_t ret = RTMESSAGE_BUS_SUCCESS;
    ret = rbus_GetElementsAddedByObject(name, &response);
    if(ret == RTMESSAGE_BUS_SUCCESS)
    {
        const char *comp = NULL;
        rbus_PopInt32(response, numElements);
        if(*numElements > 0)
            *numElements = *numElements-1;  //Fix this. We need a better way to ignore the component name as element name.
        if(*numElements)
        {
            int i;
            val = (char**)calloc(*numElements,  sizeof(char*));
            memset(val, 0, *numElements * sizeof(char*));
            for(i = 0; i < *numElements; i++)
            {
                comp = NULL;
                rbus_PopString(response, &comp);
                val[i] = strdup(comp);
            }
        }
        rtMessage_Release(response);
        *elementNames = val;
    }
    return errorcode;
}

//************************* Parameters related Operations *******************//
rbus_errorCode_e rbus_get(rbusHandle_t rbusHandle, rbus_Tlv_t *tlv)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    rtMessage request, response;
    int ret = -1;
    comp_info* pCompInfo = (comp_info*) rbusHandle;

    rtMessage_Create(&request);
    /* Set the Component name that invokes the set */
    rbus_AppendString(request, pCompInfo->comp_name);
    /* Param Size */
    rbus_AppendInt32(request, (int32_t)1);
    rbus_AppendString(request, tlv->name);

    printf("Calling rbus_invokeRemoteMethod for [%s]\n", tlv->name);

    if((err = rbus_invokeRemoteMethod(tlv->name, METHOD_GETPARAMETERVALUES, request, 6000, &response)) != RTMESSAGE_BUS_SUCCESS)
    {
        printf("%s rbus_invokeRemoteMethod failed with err %d\n", __FUNCTION__, err);
        errorcode = RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        int valSize, valType;
        rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;

        printf("Received response for remote method invocation!\n");

        rbus_PopInt32(response, &ret);

        printf("Response from the remote method is [%d]!\n",ret);
        errorcode = (rbus_errorCode_e) ret;
        legacyRetCode = (rbusLegacyReturn_t) ret;

        if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            errorcode = RBUS_ERROR_SUCCESS;
            printf("Received valid response!\n");
            rbus_PopInt32(response, &valSize);
            if(1/*valSize*/)
            {
                const char *buff = NULL;

                //Param Name
                rbus_PopString(response, &buff);
                if(buff && (strcmp(tlv->name, buff) == 0))
                {
                    rbusLegacyDataType_t legacyType = RBUS_LEGACY_STRING;
                    printf("Requested param: [%s], Received Param: [%s]\n", tlv->name, buff);
                    //Param Type
                    rbus_PopInt32(response, &valType);
                    legacyType = (rbusLegacyDataType_t) valType;
                    if ((legacyType >= RBUS_LEGACY_STRING) && (legacyType <= RBUS_LEGACY_NONE))
                    {
                        rbus_PopString(response, &buff);
                        printf("Received Param Value in string : [%s]\n", buff);
                        /* This allocates memory and does conversion like atoi(), atoll() &  atof()*/
                        _parse_rbusData_to_tlv (buff, legacyType, tlv);
                    }
                    else
                    {
                        void *value = NULL;
                        tlv->type = (rbus_elementType_e)valType;
                        printf("Received Param Type: [%d]\n", tlv->type);
                        //Param Value
                        rbus_PopBinaryData(response, (const void **)&value, &tlv->length);
                        tlv->value = (void *)malloc(tlv->length);
                        memcpy(tlv->value, value, tlv->length);
                    }
                }
                else
                {
                    printf("Param mismatch!\n");
                    printf("Requested param: [%s], Received Param: [%s]\n", tlv->name, buff);
                }
            }
        }
        else
        {
            printf("Response from remote method indicates the call failed!!\n");
            errorcode = RBUS_ERROR_BUS_ERROR;
        }

        rtMessage_Release(response);
    }
    return errorcode;
}

rbus_errorCode_e rbus_getExt(rbusHandle_t rbusHandle, int paramCount, char** pParamNames, int *pNumTlvs, rbus_Tlv_t*** pOutParamTlv)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    rtMessage request, response;
    int i = 0;
    comp_info* pCompInfo = (comp_info*) rbusHandle;

    rtMessage_Create(&request);
    /* Set the Component name that invokes the set */
    rbus_AppendString(request, pCompInfo->comp_name);
    rbus_AppendInt32(request, paramCount);
    for (i = 0; i < paramCount; i++)
        rbus_AppendString(request, pParamNames[i]);

    /* Invoke the method */
    if((err = rbus_invokeRemoteMethod(pParamNames[0], METHOD_GETPARAMETERVALUES, request, 6000, &response)) != RTMESSAGE_BUS_SUCCESS)
    {
        printf("%s rbus_invokeRemoteMethod failed with err %d\n", __FUNCTION__, err);
        errorcode = RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        int numOfTlvs = 0;
        int ret = -1;
        rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
        printf("Received response for remote method invocation!\n");

        rbus_PopInt32(response, &ret);
        printf("Response from the remote method is [%d]!\n",ret);

        errorcode = (rbus_errorCode_e) ret;
        legacyRetCode = (rbusLegacyReturn_t) ret;

        *pNumTlvs = 0;
        if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            errorcode = RBUS_ERROR_SUCCESS;
            printf("Received valid response!\n");
            rbus_PopInt32(response, &numOfTlvs);
            *pNumTlvs = numOfTlvs;
            printf ("Number of return params = %d\n", numOfTlvs);


            if(numOfTlvs)
            {
                rbus_Tlv_t** pParamTlv = NULL;
                pParamTlv = (rbus_Tlv_t**) calloc (numOfTlvs, sizeof(rbus_Tlv_t*));
                const char *pNameBuff = NULL;
                int valType = 0;
                void *value = NULL;
                rbusLegacyDataType_t legacyType = RBUS_LEGACY_STRING;

                for(i = 0; i < numOfTlvs; i++)
                {
                    pParamTlv[i] = (rbus_Tlv_t*) calloc (1, sizeof(rbus_Tlv_t));

                    //Param Name
                    rbus_PopString(response, &pNameBuff);
                    pParamTlv[i]->name = strdup(pNameBuff);

                    //Param Type
                    rbus_PopInt32(response, &valType);

                    legacyType = (rbusLegacyDataType_t) valType;
                    if ((legacyType >= RBUS_LEGACY_STRING) && (legacyType <= RBUS_LEGACY_NONE))
                    {
                        const char* pBuffer = NULL;
                        rbus_PopString(response, &pBuffer);
                        printf("Received Param Value in string : [%s]\n", pBuffer);
                        /* This allocates memory and does conversion like atoi(), atoll() &  atof()*/
                        _parse_rbusData_to_tlv (pBuffer, legacyType, pParamTlv[i]);
                    }
                    else
                    {
                        pParamTlv[i]->type = (rbus_elementType_e) valType;

                        rbus_PopBinaryData(response, (const void **)&value, (unsigned int*) &pParamTlv[i]->length);
                        //Param Value Length
                        pParamTlv[i]->value = (void *)malloc(pParamTlv[i]->length);
                        memcpy (pParamTlv[i]->value, value, pParamTlv[i]->length);
                    }
                }
                *pOutParamTlv = pParamTlv;
            }
        }
        else
        {
            printf("Response from remote method indicates the call failed!!\n");
        }
        rtMessage_Release(response);
    }

    return errorcode;
}

rbus_errorCode_e rbus_getInt(rbusHandle_t rbusHandle, char* pParamName, int* pParamVal)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;

    if (pParamVal && pParamName)
    {
        rbus_Tlv_t tlv = {0};
        tlv.name = pParamName;
        errorcode = rbus_get(rbusHandle, &tlv);

        if ((errorcode == RBUS_ERROR_SUCCESS) && (tlv.type == RBUS_INT32))
        {
            memcpy ((void*)pParamVal, tlv.value, tlv.length);
        }
        if (tlv.value)
        {
            free(tlv.value);
        }
    }
    return errorcode;
}

rbus_errorCode_e rbus_getUint (rbusHandle_t rbusHandle, char* pParamName, unsigned int* pParamVal)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;

    if (pParamVal && pParamName)
    {
        rbus_Tlv_t tlv = {0};
        tlv.name = pParamName;
        errorcode = rbus_get(rbusHandle, &tlv);

        if ((errorcode == RBUS_ERROR_SUCCESS) && (tlv.type == RBUS_UINT32))
        {
            memcpy ((void*)pParamVal, tlv.value, tlv.length);
        }
        if (tlv.value)
        {
            free(tlv.value);
        }
    }
    return errorcode;
}

rbus_errorCode_e rbus_getStr (rbusHandle_t rbusHandle, char* pParamName, char* pParamVal)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;

    if (pParamVal && pParamName)
    {
        rbus_Tlv_t tlv = {0};
        tlv.name = pParamName;
        errorcode = rbus_get(rbusHandle, &tlv);

        if ((errorcode == RBUS_ERROR_SUCCESS) && (tlv.type == RBUS_STRING))
        {
            memcpy ((void*)pParamVal, tlv.value, tlv.length);
        }
        if (tlv.value)
        {
            free(tlv.value);
        }
    }
    return errorcode;
}

rbus_errorCode_e rbus_set(rbusHandle_t rbusHandle, rbus_Tlv_t* tlv,
                                            int sessionId, rbusBool_t commit)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    rtMessage setRequest, setResponse;
    comp_info* pCompInfo = (comp_info*) rbusHandle;

    if (tlv != NULL)
    {
        rtMessage_Create(&setRequest);
        /* Set the Session ID first */
        rbus_AppendInt32(setRequest, sessionId);
        /* Set the Component name that invokes the set */
        rbus_AppendString(setRequest, pCompInfo->comp_name);
        /* Set the Size of params */
        rbus_AppendInt32(setRequest, 1);

        /* Set the params in details */
        rbus_AppendString(setRequest, tlv->name);
        rbus_AppendInt32(setRequest, tlv->type);
        rbus_AppendBinaryData(setRequest, tlv->value, tlv->length);

        /* Set the Commit value; FIXME: Should we use string? */
        rbus_AppendString(setRequest, commit ? "TRUE" : "FALSE");

        if((err = rbus_invokeRemoteMethod(tlv->name, METHOD_SETPARAMETERVALUES, setRequest, 6000, &setResponse)) != RTMESSAGE_BUS_SUCCESS)
        {
            printf("%s rbus_invokeRemoteMethod failed with err %d\n", __FUNCTION__, err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
            int ret = -1;
            const char* pErrorReason = NULL;
            rbus_PopInt32(setResponse, &ret);

            printf("Response from the remote method is [%d]!\n", ret);
            errorcode = (rbus_errorCode_e) ret;
            legacyRetCode = (rbusLegacyReturn_t) ret;

            if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
            {
                errorcode = RBUS_ERROR_SUCCESS;
                printf("Successfully Set the Value\n");
            }
            else
            {
                rbus_PopString(setResponse, &pErrorReason);
                printf("Failed to Set the Value for %s\n", pErrorReason);
            }

            /* Release the reponse message */
            rtMessage_Release(setResponse);
        }
    }
    return errorcode;
}

rbus_errorCode_e rbus_setMulti(rbusHandle_t rbusHandle, int numTlvs,
                            rbus_Tlv_t *tlv, int sessionId, rbusBool_t commit)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;
    rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
    rtMessage setRequest, setResponse;
    int loopCnt = 0;
    comp_info* pCompInfo = (comp_info*) rbusHandle;

    if (tlv != NULL)
    {
        rtMessage_Create(&setRequest);
        /* Set the Session ID first */
        rbus_AppendInt32(setRequest, sessionId);
        /* Set the Component name that invokes the set */
        rbus_AppendString(setRequest, pCompInfo->comp_name);
        /* Set the Size of params */
        rbus_AppendInt32(setRequest, numTlvs);

        /* Set the params in details */
        for (loopCnt = 0; loopCnt < numTlvs; loopCnt++)
        {
            rbus_AppendString(setRequest, tlv[loopCnt].name);
            rbus_AppendInt32(setRequest, tlv[loopCnt].type);
            rbus_AppendBinaryData(setRequest, tlv[loopCnt].value, tlv[loopCnt].length);
        }

        /* Set the Commit value; FIXME: Should we use string? */
        rbus_AppendString(setRequest, commit ? "TRUE" : "FALSE");

        /* TODO: At this point in time, only given Table/Component can be updated with SET/GET..
         * So, passing the elementname as first arg is not a issue for now..
         * We must enhance the rbus in such a way that we shd be able to set across components. Lets revist this area at that time.
         */
#if 0
        /* TODO: First step towards the above comment. When we enhace to support acorss components, this following has to be looped or appropriate count will be passed */
        const char* pElementNames[] = {tlv[0].name, NULL};
        char** pComponentName = NULL;
        err = rbus_findMatchingObjects(pElementNames, 1, &pComponentName);
        if (err != RTMESSAGE_BUS_SUCCESS)
        {
            printf ("Element not found\n");
            errorcode = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        }
        else
        {
            printf ("Component name is, %s\n", pComponentName[0]);
            free (pComponentName[0]);
        }
#endif
        if((err = rbus_invokeRemoteMethod(tlv[0].name, METHOD_SETPARAMETERVALUES, setRequest, 6000, &setResponse)) != RTMESSAGE_BUS_SUCCESS)
        {
            printf("%s rbus_invokeRemoteMethod failed with err %d\n", __FUNCTION__, err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            const char* pErrorReason = NULL;
            rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
            int ret = -1;
            rbus_PopInt32(setResponse, &ret);

            printf("Response from the remote method is [%d]!\n", ret);
            errorcode = (rbus_errorCode_e) ret;
            legacyRetCode = (rbusLegacyReturn_t) ret;

            if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
            {
                errorcode = RBUS_ERROR_SUCCESS;
                printf("Successfully Set the Value\n");
            }
            else
            {
                rbus_PopString(setResponse, &pErrorReason);
                printf("Failed to Set the Value for %s\n", pErrorReason);
            }

            /* Release the reponse message */
            rtMessage_Release(setResponse);
        }
    }
    return errorcode;
}

rbus_errorCode_e rbus_setInt (rbusHandle_t rbusHandle, char* paramName, int paramVal)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;

    if (paramName != NULL)
    {
        rbus_Tlv_t tlvInput = {0};

        tlvInput.name = paramName;
        tlvInput.type = RBUS_INT32;
        tlvInput.length = sizeof (int);
        tlvInput.value = (int*) malloc (tlvInput.length);
        memcpy (tlvInput.value, (void*) &paramVal, tlvInput.length);

        errorcode = rbus_set (rbusHandle, &tlvInput, 0, RBUS_TRUE);

        /* Free the memory that was allocated */
        free(tlvInput.value);
    }
    return errorcode;
}

rbus_errorCode_e rbus_setUint (rbusHandle_t rbusHandle, char* paramName,
                                            unsigned int paramVal)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;

    if (paramName != NULL)
    {
        rbus_Tlv_t tlvInput = {0};

        tlvInput.name = paramName;
        tlvInput.type = RBUS_UINT32;
        tlvInput.length = sizeof (unsigned int);
        tlvInput.value = (unsigned int*) malloc (tlvInput.length);
        memcpy (tlvInput.value, (void*) &paramVal, tlvInput.length);

        errorcode = rbus_set (rbusHandle, &tlvInput, 0, RBUS_TRUE);

        /* Free the memory that was allocated */
        free(tlvInput.value);
    }
    return errorcode;
}

rbus_errorCode_e rbus_setStr (rbusHandle_t rbusHandle, char* paramName, char* paramVal)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_INVALID_INPUT;

    if ((paramVal != NULL) && (paramName != NULL))
    {
        rbus_Tlv_t tlvInput = {0};

        tlvInput.name = paramName;
        tlvInput.type = RBUS_STRING;
        tlvInput.length = 1 + strlen (paramVal);
        tlvInput.value = (char*) malloc (tlvInput.length);
        memcpy (tlvInput.value, (void*) paramVal, tlvInput.length);

        errorcode = rbus_set (rbusHandle, &tlvInput, 0, RBUS_TRUE);

        /* Free the memory that was allocated */
        free(tlvInput.value);
    }

    return errorcode;
}

//************************** Events ****************************//

rbus_errorCode_e  rbusEvent_Subscribe(
    rbusHandle_t        rbusHandle,
    char const*         eventName,
    rbusEventHandler_t  handler,
    void*               userData)
{
    comp_info* ci = (comp_info*)rbusHandle;
    rbus_error_t err;
    rbusEventSubscription_t* sub;

    printf("rbusEvent_Subscribe %s\n", eventName);

    sub = (rbusEventSubscription_t*)calloc(1, sizeof(rbusEventSubscription_t));
    sub->handle = rbusHandle;
    sub->handler = handler;
    sub->user_data = userData;
    sub->event_name = strdup(eventName);

    err = rbus_subscribeToEvent(NULL, eventName, _event_callback_handler, sub);

    if(err == RTMESSAGE_BUS_SUCCESS)
    {
        rtVector_PushBack(ci->event_subs, sub);
    }
    else
    {   
        rbusEventSubscription_free(sub);
        printf("rbusEvent_Subscribe failed err=%d\n", err);
    }

    return err == RTMESSAGE_BUS_SUCCESS ? RBUS_ERROR_SUCCESS: RBUS_ERROR_BUS_ERROR;
}

rbus_errorCode_e rbusEvent_Unsubscribe(
    rbusHandle_t        rbusHandle,
    char const*         eventName)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    comp_info* ci = (comp_info*)rbusHandle;
    rbusEventSubscription_t* sub;

    rbus_unsubscribeFromEvent(NULL, eventName);

    /*the use of rtVector is inefficient here.  I have to loop through the vector to find the sub by name, 
        then call RemoveItem, which loops through again to find the item by address to destroy */
    sub = rbusEventSubscription_find(ci->event_subs, eventName);
    if(sub)
    {
        rtVector_RemoveItem(ci->event_subs, sub, rbusEventSubscription_free);
    }
    else
    {
        printf("rbusEvent_Unsubscribe unexpected -- we should have found a sub but didn't\n");

    }
    return errorcode;
}

rbus_errorCode_e rbusEvent_SubscribeEx(
    rbusHandle_t                rbusHandle,
    rbusEventSubscription_t*    subscription,
    int                         numSubscriptions)
{
    comp_info* ci = (comp_info*)rbusHandle;
    rbus_error_t err;
    rbusEventSubscription_t* sub;
    int i, j;

    printf("rbusEvent_SubscribeEx\n");

    for(i = 0; i < numSubscriptions; ++i)
    {
        /*
            I'm thinking its best to make a copy of the subscription being passed by user because:
            1. its safer because what if the caller mucks around with the contents later: .e.g. tries to reuse the subscription and changes the eventName
            2. its easier to manage memory because for both rbusEvent_Subscribe and rbusEvent_SubscribeEx because we own the memory and know when to free it
        */
        sub = (rbusEventSubscription_t*)calloc(1, sizeof(rbusEventSubscription_t));
        sub->handle = rbusHandle;
        sub->handler = subscription[i].handler;
        sub->user_data = subscription[i].user_data;
        sub->event_name = strdup(subscription[i].event_name);

        err = rbus_subscribeToEvent(NULL, sub->event_name, _event_callback_handler, sub);

        if(err == RTMESSAGE_BUS_SUCCESS)
        {
            rtVector_PushBack(ci->event_subs, sub);
        }
        else
        {   
            printf("rbusEvent_SubscribeEx failed err=%d\n", err);

            rbusEventSubscription_free(sub);

            /* So here I'm thinking its best to treat SubscribeEx like a transaction because
                if any subs fails, how will the user know which ones succeeded and which failed ?
                So, as a transaction, we just undo everything, which are all those from 0 to i-1.
            */
            for(j = 0; j < i; ++j)
            {
                rbusEvent_Unsubscribe(rbusHandle, subscription[i].event_name);
            }
            return RBUS_ERROR_BUS_ERROR;
        }
    }

    return RBUS_ERROR_SUCCESS;
}

rbus_errorCode_e rbusEvent_UnsubscribeEx(
    rbusHandle_t                rbusHandle,
    rbusEventSubscription_t*    subscription,
    int                         numSubscriptions)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;
    comp_info* ci = (comp_info*)rbusHandle;
    rbusEventSubscription_t* our_copy;
    int i;

    for(i = 0; i < numSubscriptions; ++i)
    {
        rbus_unsubscribeFromEvent(NULL, subscription[i].event_name);

        /*the use of rtVector is inefficient here.  I have to loop through the vector to find the sub by name, 
            then call RemoveItem, which loops through again to find the item by address to destroy */
        our_copy = rbusEventSubscription_find(ci->event_subs, subscription[i].event_name);
        if(our_copy)
        {
            rtVector_RemoveItem(ci->event_subs, our_copy, rbusEventSubscription_free);
        }
        else
        {
            printf("rbusEvent_Unsubscribe unexpected -- we should have found a sub but didn't\n");

        }
    }
    return errorcode;
}

rbus_errorCode_e  rbusEvent_Publish(
  rbusHandle_t      rbusHandle,
  rbus_Tlv_t*       event /*TODO the Tlv will receiver come Tlc soon -- thus expect their api to change*/ 
    )
{
    comp_info* ci = (comp_info*)rbusHandle;
    rbus_error_t err;

    rtMessage msg;
    rtMessage_Create(&msg);
    rbusTlv_appendToMessage(event, msg);
    err = rbus_publishEvent(ci->comp_name,  event->name, msg);
    rtMessage_Release(msg);

    return err == RTMESSAGE_BUS_SUCCESS ? RBUS_ERROR_SUCCESS: RBUS_ERROR_BUS_ERROR;
}

//************************** Table Objects - APIs***************************//
rbus_errorCode_e rbus_updateTable(rbusHandle_t rbusHandle, char *tableName,
                            rbus_tblAction_e action, char* aliasName)
{
    rbus_errorCode_e errorcode = RBUS_ERROR_SUCCESS;

    UNUSED4(rbusHandle,tableName,action,aliasName);

    return errorcode;
}

