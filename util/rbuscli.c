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
#include <sys/types.h>
#include <unistd.h>

#include "rbus.h"

#define RBUS_CLI_COMPONENT_NAME "rbuscli"
#define RBUS_CLI_MAX_PARAM      25
#define RBUS_CLI_MAX_CMD_ARG    (RBUS_CLI_MAX_PARAM * 3)
#define RBUS_CLI_SESSION_ID     4230

typedef struct _tlv_t {
    char *p_name;
    char *p_type;
    char *p_val;
} rbus_cli_tlv_t;

rbus_cli_tlv_t g_tlvParams[RBUS_CLI_MAX_PARAM];
rbusHandle_t   g_busHandle = 0;

rbusBool_t g_isInteractive = RBUS_FALSE;

void show_menu()
{
    printf ("\nThis utility is to get/set value over rbus.\n");
    printf ("\nUsage:\n");
    printf ("rbuscli <command> \n\n");
    printf ("Here are the avaiable commands.\n");
    printf ("***************************************************************************************\n");
    printf ("* getvalues    param_name [param_name] ...\n");
    printf ("* setvalues    param_name type value [<param_name> <type> <value>] ... [commit]\n");
    printf ("* disccomps    element_name [element_name] ...\n");
    printf ("* discelements component_name\n");
    printf ("* discelements partial_element_path immediate/all \n");
    printf ("* subscribe    event_name event_destination_name  #FIXME\n"); // FIXME
    printf ("* interactive\n");
    printf ("* help\n");
    printf ("* quit\n");
    printf ("***************************************************************************************\n");
    return;
}

char* getDataType_toString (rbus_elementType_e type)
{
    char* pTextData = "None";

    if(type == RBUS_BOOLEAN)
        pTextData = "boolean";
    else if(type == RBUS_INT16)
        pTextData = "int16";
    else if(type == RBUS_UINT16)
        pTextData = "uint16";
    else if(type == RBUS_INT32)
        pTextData = "int32";
    else if(type == RBUS_UINT32)
        pTextData = "uint32";
    else if(type == RBUS_INT64)
        pTextData = "int64";
    else if(type == RBUS_UINT64)
        pTextData = "uint64";
    else if(type == RBUS_STRING)
        pTextData = "string";
    else if(type == RBUS_DATE_TIME)
        pTextData = "date_time";
    else if(type == RBUS_BASE64)
        pTextData = "base64";
    else if(type == RBUS_BINARY)
        pTextData = "binary";
    else if(type == RBUS_FLOAT)
        pTextData = "float";
    else if(type == RBUS_DOUBLE)
        pTextData = "double";
    else if(type == RBUS_BYTE)
        pTextData = "byte";
    else if(type == RBUS_REFERENCE)
        pTextData = "reference";
    else if(type == RBUS_EVENT_DEST_NAME)
        pTextData = "event_dest_name";

    return pTextData;
}


rbus_elementType_e getDataType_fromString(const char* pType)
{
    rbus_elementType_e rc = RBUS_NONE;

    if (strncasecmp ("boolean",    pType, 4) == 0)
        rc = RBUS_BOOLEAN;
    else if (strncasecmp("int16",  pType, 5) == 0)
        rc = RBUS_INT16;
    else if (strncasecmp("uint16", pType, 6) == 0)
        rc = RBUS_UINT16;
    else if (strncasecmp("int32",  pType, 5) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint32", pType, 6) == 0)
        rc = RBUS_UINT32;
    else if (strncasecmp("int64",  pType, 5) == 0)
        rc = RBUS_INT64;
    else if (strncasecmp("uint64", pType, 6) == 0)
        rc = RBUS_UINT64;
    else if (strncasecmp("string", pType, 6) == 0)
        rc = RBUS_STRING;
    else if (strncasecmp("date_time", pType, 4) == 0)
        rc = RBUS_DATE_TIME;
    else if (strncasecmp("base64", pType, 6) == 0)
        rc = RBUS_BASE64;
    else if (strncasecmp("binary",    pType, 3) == 0)
        rc = RBUS_BINARY;
    else if (strncasecmp("float",  pType, 5) == 0)
        rc = RBUS_FLOAT;
    else if (strncasecmp("double", pType, 6) == 0)
        rc = RBUS_DOUBLE;
    else if (strncasecmp("byte",   pType, 4) == 0)
        rc = RBUS_BYTE;
    else if (strncasecmp("reference",   pType, 4) == 0)
        rc = RBUS_REFERENCE;
    else if (strncasecmp("event_dest_name",   pType, 5) == 0)
        rc = RBUS_EVENT_DEST_NAME;

    /* Risk handling, if the user types just int, lets consider int32; same for unsigned too  */
    else if (strncasecmp("int",  pType, 3) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint",  pType, 4) == 0)
        rc = RBUS_UINT32;

    return rc;
}

int getLength_fromDataType(rbus_elementType_e type)
{
    int length = 0;
    if(type == RBUS_BOOLEAN)
        length = sizeof (rbusBool_t);
    else if(type == RBUS_INT16)
        length = sizeof (short);
    else if(type == RBUS_UINT16)
        length = sizeof (unsigned short);
    else if(type == RBUS_INT32)
        length = sizeof (int);
    else if(type == RBUS_UINT32)
        length = sizeof (unsigned int);
    else if(type == RBUS_INT64)
        length = sizeof (long long);
    else if(type == RBUS_UINT64)
        length = sizeof (unsigned long long);
    else if(type == RBUS_STRING)
        length = 1; // Caller of this menthod has to figure out the length from the input value
    else if(type == RBUS_DATE_TIME)
        length = 8; // FIXME
    else if(type == RBUS_BASE64)
        length = 8; // FIXME
    else if(type == RBUS_BINARY)
        length = 8; // FIXME
    else if(type == RBUS_FLOAT)
        length = sizeof(float);
    else if(type == RBUS_DOUBLE)
        length = sizeof(double);
    else if(type == RBUS_BYTE)
        length = 1; // Caller of this menthod has to figure out the length from the input value
    else if(type == RBUS_REFERENCE)
        length = 4;
    else if(type == RBUS_EVENT_DEST_NAME)
        length = 1; // Caller of this menthod has to figure out the length from the input value

    return length;
}

void execute_discover_component_cmd(int argc, char* argv[])
{
    rbus_errorCode_e rc = RBUS_ERROR_SUCCESS;
    int elementCnt = argc - 2;
    int loopCnt = 0;
    int index = 0;
    int componentCnt = 0;
    char **pComponentNames;
    char *pElementNames[RBUS_CLI_MAX_PARAM] = {0, 0};

    for (index = 0, loopCnt = 2; index < elementCnt; index++, loopCnt++)
    {
        pElementNames[index] = argv[loopCnt];
    }

    /* We will have it opened already in case of interactive mode */
    if (0 == g_busHandle)
    {
        char compName[50] = "";
        snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
        rc = rbus_open(&g_busHandle, compName);
    }

    if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
    {
        rc = rbus_discoverComponentName (g_busHandle, elementCnt, pElementNames, &componentCnt, &pComponentNames);
        if(RBUS_ERROR_SUCCESS == rc)
        {
            printf ("Discovered components for the given elements.\n");
            for (loopCnt = 0; loopCnt < componentCnt; loopCnt++)
            {
                printf ("\tComponent %d: %s\n", (loopCnt + 1), pComponentNames[loopCnt]);
                free(pComponentNames[loopCnt]);
            }
            free(pComponentNames);
        }
        else
        {
            printf ("Failed to discover component. Error Code = %s\n", "");
        }
    }
    else
    {
        printf ("Invalid Handle to communicate with\n");
    }
    return;
}

void execute_discover_elements_cmd(int argc, char *argv[])
{
    rbus_errorCode_e rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    rbusBool_t nextLevel = RBUS_TRUE;
    int numElements = 0;
    int loopCnt = 0;
    char** pElementNames; // FIXME: every component will have more than RBUS_CLI_MAX_PARAM elements right?

    if (2 == numOfInputParams)
    {
        if (0 == strncmp (argv[3], "all", 3))
        {
            nextLevel = RBUS_FALSE;
        }
    }

    /* We will have it opened already in case of interactive mode */
    if (0 == g_busHandle)
    {
        char compName[50] = "";
        snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
        rc = rbus_open(&g_busHandle, compName);
    }

    if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
    {
        rc = rbus_discoverComponentDataElements (g_busHandle, argv[2], nextLevel, &numElements, &pElementNames);
        if(RBUS_ERROR_SUCCESS == rc)
        {
            if(numElements)
            {
                printf ("Discovered elements are:\n");
                for (loopCnt = 0; loopCnt < numElements; loopCnt++)
                {
                    printf ("\tElement %d: %s\n", (loopCnt + 1), pElementNames[loopCnt]);
                    free(pElementNames[loopCnt]);
                }
                free(pElementNames);
            }
            else
            {
                printf("No elements discovered!\n");
            }
        }
        else
        {
            printf ("Failed to discover element array. Error Code = %s\n", "");
        }
    }
    else
    {
        printf ("Invalid Handle to communicate with\n");
    }

    return;
}

void validate_and_execute_get_cmd (int argc, char *argv[])
{
    rbus_errorCode_e rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    int i = 0;
    int index = 0;
    int numOfOutTlvs = 0;
    rbus_Tlv_t **pOutputTlvs = NULL;
    char *pInputParam[RBUS_CLI_MAX_PARAM] = {0, 0};

    if (numOfInputParams < 1)
    {
        printf ("Invalid arguments. Please see the help\n");
        return;
    }

    for (index = 0, i = 2; index < numOfInputParams; index++, i++)
        pInputParam[index] = argv[i];

    /* We will have it opened already in case of interactive mode */
    if (0 == g_busHandle)
    {
        char compName[50] = "";
        snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
        rc = rbus_open(&g_busHandle, compName);
    }

    if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
    {
        rc = rbus_getExt(g_busHandle, numOfInputParams, pInputParam, &numOfOutTlvs, &pOutputTlvs);
        if(RBUS_ERROR_SUCCESS == rc)
        {
            for (i = 0; i < numOfOutTlvs; i++)
            {
                rbus_elementType_e type = pOutputTlvs[i]->type;
                printf ("Parameter %2d:\n", i+1);
                printf ("              Name  : %s\n", pOutputTlvs[i]->name);
                if(type == RBUS_BOOLEAN)
                {
                    rbusBool_t data = *(rbusBool_t*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "boolean");
                    printf ("              Value : %s\n", data ? "true" : "false");
                }
                else if(type == RBUS_INT16)
                {
                    short data = *(short*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "int16");
                    printf ("              Value : %d\n", data);
                }
                else if(type == RBUS_UINT16)
                {
                    unsigned short data = *(unsigned short*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "uint16");
                    printf ("              Value : %u\n", data);
                }
                else if(type == RBUS_INT32)
                {
                    int data = *(int*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "int32");
                    printf ("              Value : %d\n", data);
                }
                else if(type == RBUS_UINT32)
                {
                    unsigned int data = *(unsigned int*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "uint32");
                    printf ("              Value : %u\n", data);
                }
                else if(type == RBUS_INT64)
                {
                    long long int data = *(long long int*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "int64");
                    printf ("              Value : %lld\n", data);
                }
                else if(type == RBUS_UINT64)
                {
                    unsigned long long int data = *(unsigned long long int*) pOutputTlvs[i]->value;
                    printf ("              Type  : %s\n", "uint64");
                    printf ("              Value : %llu\n", data);
                }
                else if(type == RBUS_STRING)
                {
                    printf ("              Type  : %s\n", "string");
                    printf ("              Value : %s\n", (char*) pOutputTlvs[i]->value);
                }
                else if(type == RBUS_DATE_TIME) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "date_time");
                    printf ("              Value : %s\n", "");
                }
                else if(type == RBUS_BASE64) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "base64");
                    printf ("              Value : %s\n", "");
                }
                else if(type == RBUS_BINARY) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "binary");
                    printf ("              Value : ");
                }
                else if(type == RBUS_FLOAT)
                {
                    printf ("              Type  : %s\n", "float");
                    printf ("              Value : %f\n", *(float *) pOutputTlvs[i]->value);
                }
                else if(type == RBUS_DOUBLE)
                {
                    printf ("              Type  : %s\n", "double");
                    printf ("              Value : %f\n", *(double*) pOutputTlvs[i]->value);
                }
                else if(type == RBUS_BYTE) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "byte");
                    printf ("              Value : %s\n", "");
                }
                else if(type == RBUS_REFERENCE)
                {
                    printf ("              Type  : %s\n", "reference");
                    /* No data to be printed for this data type.. */
                }
                else if(type == RBUS_EVENT_DEST_NAME) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "event_dest_name");
                    printf ("              Value : %s\n", "");
                }
            }
            /* Free the memory */
            for (i = 0; i < numOfOutTlvs; i++)
            {
                free (pOutputTlvs[i]->name);
                free (pOutputTlvs[i]->value);
                free (pOutputTlvs[i]);
            }
            free (pOutputTlvs);
        }
        else
        {
            printf ("Failed to get the data\n");
        }
    }
    else
    {
        printf ("Invalid Handle to communicate with\n");
    }
}

void validate_and_execute_set_cmd (int argc, char *argv[])
{
    int i = argc - 2;

    /* must have 3 or multiples of 3 parameters.. it could possibliy have 1 extra param which
     * could be having commit and set to true/false */
    if ((i >= 3) && ((i % 3 == 0) || (i % 3 == 1)))
    {
        rbus_errorCode_e rc = RBUS_ERROR_SUCCESS;

        /* We will have it opened already in case of interactive mode */
        if (0 == g_busHandle)
        {
            char compName[50] = "";
            snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
            rc = rbus_open(&g_busHandle, compName);
        }

        if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
        {
            rbusBool_t isCommit = RBUS_TRUE;
            int sessionId = 0;
            static rbusBool_t g_pendingCommit = RBUS_FALSE;
            if (i > 4) /* Multiple set commands; Lets use TLV */
            {
                int isInvalid = 0;
                int index = 0;
                int loopCnt = 0;
                int paramCnt = i/3;
                rbus_Tlv_t setVal[RBUS_CLI_MAX_PARAM] = {{0}};

                for (index = 0, loopCnt = 0; index < paramCnt; loopCnt+=3, index++)
                {
                    /* Set Param Name */
                    setVal[index].name = argv[loopCnt+2];
                    printf ("Name = %s \n", setVal[index].name);

                    /* Set Param Type */
                    setVal[index].type = getDataType_fromString(argv[loopCnt+3]);

                    /* DEBUG */
                    //printf ("Type as %s\n", getDataType_toString (setVal[index].type));

                    if (setVal[index].type == RBUS_NONE)
                    {
                        printf ("Invalid data type. Please see the help\n");
                        isInvalid = 1;
                        break;
                    }

                    if(setVal[index].type == RBUS_BOOLEAN)
                    {
                        rbusBool_t tmp = RBUS_FALSE;
                        setVal[index].length = sizeof (rbusBool_t);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        if (0 == strncasecmp("true", argv[loopCnt+4], 4))
                            tmp = RBUS_TRUE;
                        else if (0 == strncasecmp("false", argv[loopCnt+4], 5))
                            tmp = RBUS_FALSE;
                        else if (0 == strncasecmp("1", argv[loopCnt+4], 1))
                            tmp = RBUS_TRUE;
                        else if (0 == strncasecmp("0", argv[loopCnt+4], 1))
                            tmp = RBUS_FALSE;
                        else
                        {
                            isInvalid = 1;
                            break;
                        }
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_INT16)
                    {
                        short tmp = 0;
                        setVal[index].length = sizeof (short);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = (short) atoi(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_UINT16)
                    {
                        unsigned short tmp = 0;
                        setVal[index].length = sizeof (unsigned short);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = (unsigned short) atoi(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_INT32)
                    {
                        int tmp = 0;
                        setVal[index].length = sizeof (int);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = atoi(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_UINT32)
                    {
                        unsigned int tmp = 0;
                        setVal[index].length = sizeof (unsigned int);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = (unsigned int) atoi(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_INT64)
                    {
                        long long tmp = 0;
                        setVal[index].length = sizeof (tmp);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = (long long) atoi(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_UINT64)
                    {
                        unsigned long long tmp = 0;
                        setVal[index].length = sizeof (tmp);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = (unsigned long long) atoi(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_STRING)
                    {
                        setVal[index].length = strlen (argv[loopCnt+4]) + 1;
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        memcpy (setVal[index].value, argv[loopCnt+4], setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_DATE_TIME)
                    {
                        setVal[index].length = 8; // FIXME
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        memcpy (setVal[index].value, argv[loopCnt+4], setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_BASE64)
                    {
                        setVal[index].length = 8; // FIXME
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        memcpy (setVal[index].value, argv[loopCnt+4], setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_BINARY)
                    {
                        setVal[index].length = 8; // FIXME
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        memcpy (setVal[index].value, argv[loopCnt+4], setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_FLOAT)
                    {
                        float tmp = 0;
                        setVal[index].length = sizeof (tmp);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = (float) atof(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_DOUBLE)
                    {
                        double tmp = 0;
                        setVal[index].length = sizeof (tmp);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        tmp = atof(argv[loopCnt+4]);
                        memcpy (setVal[index].value, &tmp, setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_BYTE)
                    {
                        setVal[index].length = strlen (argv[loopCnt+4]);
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        memcpy (setVal[index].value, argv[loopCnt+4], setVal[index].length);
                    }
                    else if(setVal[index].type == RBUS_REFERENCE)
                    {
                        setVal[index].length = 0; //FIXME
                    }
                    else if(setVal[index].type == RBUS_EVENT_DEST_NAME)
                    {
                        setVal[index].length = strlen (argv[loopCnt+4]) + 1;
                        setVal[index].value = (void*) malloc(setVal[index].length);
                        memcpy (setVal[index].value, argv[loopCnt+4], setVal[index].length);
                    }
                }

                if (0 == isInvalid)
                {
                    /* Set Session ID & Commit value */
                    if (g_isInteractive)
                    {
                        /* Do we have commit param? (n*3) + 1 */
                        if (i % 3 == 1)
                        {
                            /* Is commit */
                            if (strncasecmp ("true", argv[argc - 1], 4) == 0)
                                isCommit = RBUS_TRUE;
                            else if (strncasecmp ("false", argv[argc - 1], 5) == 0)
                                isCommit = RBUS_FALSE;
                            else
                                isCommit = RBUS_TRUE;

                            if (isCommit == RBUS_FALSE)
                            {
                                g_pendingCommit = RBUS_TRUE;
                                sessionId = RBUS_CLI_SESSION_ID;
                            }
                            else
                            {
                                if (g_pendingCommit)
                                    sessionId = RBUS_CLI_SESSION_ID;
                                else
                                    sessionId = 0;
                            }
                        }
                        else
                        {
                            isCommit = RBUS_TRUE;
                            if (g_pendingCommit)
                                sessionId = RBUS_CLI_SESSION_ID;
                            else
                                sessionId = 0;
                        }
                    }
                    else
                    {
                        /* For non-interactive mode, regardless of COMMIT value tha is passed, we assume the isCommit as TRUE */
                        isCommit = RBUS_TRUE;
                        sessionId = 0;
                    }

                    /* Reset the flag */
                    if (isCommit == RBUS_TRUE)
                        g_pendingCommit = RBUS_FALSE;


                    rc = rbus_setMulti(g_busHandle, paramCnt, setVal, sessionId, isCommit);
                }
                else
                {
                    /* Since we allocated memory for `loopCnt` times, we must free them.. lets update the `paramCnt` to `loopCnt`; so that the below free function will take care */
                    paramCnt = index;
                    rc = RBUS_ERROR_INVALID_INPUT;
                }

                /* free the memory that was allocated */
                for (loopCnt = 0; loopCnt < paramCnt; loopCnt++)
                {
                    if (setVal[loopCnt].value)
                        free (setVal[loopCnt].value);
                }
            }
            else /* Single Set Command */
            {
                rbus_Tlv_t setVal;

                /* Set Param Name */
                setVal.name = argv[2];

                /* Set Param Type */
                setVal.type = getDataType_fromString(argv[3]);

                /* DEBUG */
                //printf ("Type as %s\n", getDataType_toString (setVal.type));
                if (setVal.type != RBUS_NONE)
                {
                    if(setVal.type == RBUS_BOOLEAN)
                    {
                        rbusBool_t tmp = RBUS_FALSE;
                        setVal.length = sizeof (rbusBool_t);
                        setVal.value = (void*) malloc(setVal.length);
                        if (0 == strncasecmp("true", argv[4], 4))
                            tmp = RBUS_TRUE;
                        else if (0 == strncasecmp("false", argv[4], 5))
                            tmp = RBUS_FALSE;
                        else if (0 == strncasecmp("1", argv[4], 1))
                            tmp = RBUS_TRUE;
                        else if (0 == strncasecmp("0", argv[4], 1))
                            tmp = RBUS_FALSE;
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_INT16)
                    {
                        short tmp = 0;
                        setVal.length = sizeof (short);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (short) atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_UINT16)
                    {
                        unsigned short tmp = 0;
                        setVal.length = sizeof (unsigned short);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (unsigned short) atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_INT32)
                    {
                        int tmp = 0;
                        setVal.length = sizeof (int);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_UINT32)
                    {
                        unsigned int tmp = 0;
                        setVal.length = sizeof (unsigned int);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (unsigned int) atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_INT64)
                    {
                        long long tmp = 0;
                        setVal.length = sizeof (tmp);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (long long) atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_INT64)
                    {
                        long long tmp = 0;
                        setVal.length = sizeof (tmp);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (long long) atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_UINT64)
                    {
                        unsigned long long tmp = 0;
                        setVal.length = sizeof (tmp);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (unsigned long long) atoi(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_STRING)
                    {
                        setVal.length = strlen (argv[4]) + 1;
                        setVal.value = (void*) malloc(setVal.length);
                        memcpy (setVal.value, argv[4], setVal.length);
                    }
                    else if(setVal.type == RBUS_DATE_TIME)
                    {
                        setVal.length = 8; // FIXME
                        setVal.value = (void*) malloc(setVal.length);
                        memcpy (setVal.value, argv[4], setVal.length);
                    }
                    else if(setVal.type == RBUS_BASE64)
                    {
                        setVal.length = 8; // FIXME
                        setVal.value = (void*) malloc(setVal.length);
                        memcpy (setVal.value, argv[4], setVal.length);
                    }
                    else if(setVal.type == RBUS_BINARY)
                    {
                        setVal.length = 8; // FIXME
                        setVal.value = (void*) malloc(setVal.length);
                        memcpy (setVal.value, argv[4], setVal.length);
                    }
                    else if(setVal.type == RBUS_FLOAT)
                    {
                        float tmp = 0;
                        setVal.length = sizeof (tmp);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = (float) atof(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_DOUBLE)
                    {
                        double tmp = 0;
                        setVal.length = sizeof (tmp);
                        setVal.value = (void*) malloc(setVal.length);
                        tmp = atof(argv[4]);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_BYTE)
                    {
                        char tmp = argv[4][0];
                        setVal.length = sizeof(tmp);
                        setVal.value = (void*) malloc(setVal.length);
                        memcpy (setVal.value, &tmp, setVal.length);
                    }
                    else if(setVal.type == RBUS_REFERENCE)
                    {
                        setVal.length = 0; //FIXME
                    }
                    else if(setVal.type == RBUS_EVENT_DEST_NAME)
                    {
                        setVal.length = strlen (argv[4]) + 1;
                        setVal.value = (void*) malloc(setVal.length);
                        memcpy (setVal.value, argv[4], setVal.length);
                    }

                    /* Set Session ID & Commit value */
                    if (g_isInteractive)
                    {
                        if (4 == i)
                        {
                            /* Is commit */
                            if (strncasecmp ("true", argv[argc - 1], 4) == 0)
                                isCommit = RBUS_TRUE;
                            else if (strncasecmp ("false", argv[argc - 1], 5) == 0)
                                isCommit = RBUS_FALSE;
                            else
                                isCommit = RBUS_TRUE;

                            if (isCommit == RBUS_FALSE)
                            {
                                g_pendingCommit = RBUS_TRUE;
                                sessionId = RBUS_CLI_SESSION_ID;
                            }
                            else
                            {
                                if (g_pendingCommit)
                                    sessionId = RBUS_CLI_SESSION_ID;
                                else
                                    sessionId = 0;
                            }
                        }
                        else
                        {
                            isCommit = RBUS_TRUE;
                            if (g_pendingCommit)
                                sessionId = RBUS_CLI_SESSION_ID;
                            else
                                sessionId = 0;
                        }
                    }
                    else
                    {
                        /* For non-interactive mode, regardless of COMMIT value tha is passed, we assume the isCommit as TRUE */
                        isCommit = RBUS_TRUE;
                        sessionId = 0;
                    }

                    /* Reset the flag */
                    if (isCommit == RBUS_TRUE)
                        g_pendingCommit = RBUS_FALSE;

                    /* Assume a sessionId as it is going to be single entry thro this cli app; */
                    rc = rbus_set(g_busHandle, &setVal, sessionId, isCommit);

                    /* Free the data pointer that was allocated */
                    free(setVal.value);
                }
                else
                {
                    rc = RBUS_ERROR_INVALID_INPUT;
                    printf ("Invalid data type. Please see the help\n");
                }
            }

            if(RBUS_ERROR_SUCCESS == rc)
            {
                printf ("setvalues succeeded..\n");
            }
            else
            {
                printf ("setvalues failed ..\n");
            }
        }
        else
        {
            printf ("Invalid Handle to communicate with\n");
        }
    }
    else
    {
        printf ("Invalid arguments. Please see the help\n");
    }
}

int handle_cmds (int argc, char *argv[])
{
    /* Interactive shell; handle the enter key */
    if (1 == argc)
        return 0;

    if (strncmp (argv[1], "help", 1) == 0)
    {
        show_menu();
    }
    else if (strncmp (argv[1], "getvalues", 4) == 0)
    {
        validate_and_execute_get_cmd (argc, argv);
    }
    else if (strncmp (argv[1], "setvalues", 4) == 0)
    {
        validate_and_execute_set_cmd (argc, argv);
    }
    else if (strncmp (argv[1], "disccomps", 5) == 0)
    {
        int i = argc - 2;
        if (i != 0)
        {
            execute_discover_component_cmd(argc, argv);
        }
        else
        {
            printf ("Invalid arguments. Please see the help\n");
        }
    }
    else if (strncmp (argv[1], "discelements", 5) == 0)
    {
        int i = argc - 2;
        if ((i >= 1) && (i <= 2))
        {
            execute_discover_elements_cmd(argc, argv);
        }
        else
        {
            printf ("Invalid arguments. Please see the help\n");
        }
    }
    else if (strncmp (argv[1], "quit", 1) == 0)
    {
        return 1;
    }
    else if (strncmp (argv[1], "interactive", 3) == 0)
    {
        printf ("Already in interactive session.. Please see the help\n");
    }
    else
    {
        printf ("Invalid arguments. Please see the help\n");
    }

    return 0;
}

void construct_input_into_cmds(char* pTerminalInput, int *argc, char** argv)
{
    int argCnt = 0;
    int length = 0;
    char *pTmp = NULL;

    length = strlen(pTerminalInput);
    pTerminalInput[length - 1] = '\0';

    /* Update once again for safety */
    length = strlen(pTerminalInput);

    /* Fill with process name */
    argv[argCnt++] = "rbuscli";

    pTmp = strtok (pTerminalInput, ";\t ");

    while (pTmp != NULL)
    {
        argv[argCnt++] = pTmp;
        pTmp = strtok (NULL, ";\t ");
    }

    *argc = argCnt;

    return;
}

int main( int argc, char *argv[] )
{
    if( argc >= 2 )
    {
        /* Is interactive */
        if (strncmp (argv[1], "interactive", 3) == 0)
        {
            char **pUserInput = NULL;
            char userInput[2048] = "";
            int argCount = 0;
            int isExit = 0;
            char* pTmpVar = NULL;

            while (1)
            {
                g_isInteractive = RBUS_TRUE;
                printf ("rbuscli> ");
                pUserInput = (char **)calloc(RBUS_CLI_MAX_CMD_ARG, sizeof(char *));
                pTmpVar = fgets(userInput, sizeof(userInput), stdin);
                printf ("%s\n", pTmpVar);


                construct_input_into_cmds(userInput, &argCount, pUserInput);
                isExit = handle_cmds (argCount, pUserInput);

                memset (userInput, 0, sizeof(userInput));
                free (pUserInput);

                if (1 == isExit)
                    break;
            }
        }
        else
        {
            handle_cmds (argc, argv);
        }

        /* Close the handle if it is not interative */
        if (g_busHandle)
        {
            rbus_close(g_busHandle);
            g_busHandle = 0;
        }
    }
    else
        show_menu();

    return 0;
}
