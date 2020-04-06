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
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <rbus.h>

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

bool g_isInteractive = false;

void show_menu()
{
    printf ("\nThis utility is to get/set value over rbus.\n");
    printf ("\nUsage:\n");
    printf ("rbuscli <command> \n\n");
    printf ("Here are the avaiable commands.\n");
    printf ("***************************************************************************************\n");
    printf ("* getvalues    param_name [param_name] ...\n");
    printf ("* getwildcard  partial_element_path \n");
    printf ("* setvalues    param_name type value [<param_name> <type> <value>] ... [commit]\n");
    printf ("* disccomps    element_name [element_name] ...\n");
    printf ("* discelements component_name\n");
    printf ("* discelements partial_element_path immediate/all \n");
    printf ("* addrow       table_path [alias_name] \n");
    printf ("* delrow       row_path \n");
    printf ("* interactive\n");
    printf ("* help\n");
    printf ("* quit\n");
    printf ("***************************************************************************************\n");
    return;
}

char* getDataType_toString (rbusValueType_t type)
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
    else if(type == RBUS_DATETIME)
        pTextData = "date_time";
    else if(type == RBUS_BYTES)
        pTextData = "binary";
    else if(type == RBUS_SINGLE)
        pTextData = "float";
    else if(type == RBUS_DOUBLE)
        pTextData = "double";
    else if(type == RBUS_BYTE)
        pTextData = "byte";/*
    else if(type == RBUS_BASE64)
        pTextData = "base64";
    else if(type == RBUS_REFERENCE)
        pTextData = "reference";
    else if(type == RBUS_EVENT_DEST_NAME)
        pTextData = "event_dest_name";
    */
    return pTextData;
}


rbusValueType_t getDataType_fromString(const char* pType)
{
    rbusValueType_t rc = RBUS_NONE;

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
        rc = RBUS_DATETIME;
    else if (strncasecmp("base64", pType, 6) == 0)
        rc = RBUS_STRING;/*making base64 a string, since it's actually just a string*/
    else if (strncasecmp("binary",    pType, 3) == 0)
        rc = RBUS_BYTES;
    else if (strncasecmp("float",  pType, 5) == 0)
        rc = RBUS_SINGLE;
    else if (strncasecmp("double", pType, 6) == 0)
        rc = RBUS_DOUBLE;
    else if (strncasecmp("byte",   pType, 4) == 0)
        rc = RBUS_BYTE;
    else if (strncasecmp("reference",   pType, 4) == 0)
        rc = RBUS_STRING;
    else if (strncasecmp("event_dest_name",   pType, 5) == 0)
        rc = RBUS_STRING;

    /* Risk handling, if the user types just int, lets consider int32; same for unsigned too  */
    else if (strncasecmp("int",  pType, 3) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint",  pType, 4) == 0)
        rc = RBUS_UINT32;

    return rc;
}

int getLength_fromDataType(rbusValueType_t type)
{
    int length = 0;
    if(type == RBUS_BOOLEAN)
        length = sizeof (bool);
    else if(type == RBUS_CHAR)
        length = sizeof (char);
    else if(type == RBUS_BYTE)
        length = sizeof (unsigned char);
    else if(type == RBUS_INT8)
        length = sizeof (int8_t);
    else if(type == RBUS_UINT8)
        length = sizeof (uint8_t);
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
    else if(type == RBUS_DATETIME)
        length = 8; // FIXME
    else if(type == RBUS_BYTES)
        length = 8; // FIXME Caller of this menthod has to figure out the length from the input value
    else if(type == RBUS_SINGLE)
        length = sizeof(float);
    else if(type == RBUS_DOUBLE)
        length = sizeof(double);/*
    else if(type == RBUS_REFERENCE)
        length = 4;
    else if(type == RBUS_EVENT_DEST_NAME)
        length = 1; // Caller of this menthod has to figure out the length from the input value
    */
    return length;
}

void execute_discover_component_cmd(int argc, char* argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
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
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    bool nextLevel = true;
    int numElements = 0;
    int loopCnt = 0;
    char** pElementNames; // FIXME: every component will have more than RBUS_CLI_MAX_PARAM elements right?

    if (2 == numOfInputParams)
    {
        if (0 == strncmp (argv[3], "all", 3))
        {
            nextLevel = false;
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

void validate_and_execute_get_cmd (int argc, char *argv[], bool isWildCard)
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    int i = 0;
    int index = 0;
    int numOfOutVals = 0;
    rbusProperty_t outputVals = NULL;
    const char *pInputParam[RBUS_CLI_MAX_PARAM] = {0, 0};

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
        if ((!isWildCard) && (1 == numOfInputParams))
        {
            rbusValue_t getVal;
            rc = rbus_get(g_busHandle, pInputParam[0], &getVal);
            if(RBUS_ERROR_SUCCESS == rc)
            {
                numOfOutVals = 1;
                rbusProperty_Init(&outputVals, pInputParam[0], getVal);
                rbusValue_Release(getVal);
            }
        }
        else
            rc = rbus_getExt(g_busHandle, numOfInputParams, pInputParam, &numOfOutVals, &outputVals);

        if(RBUS_ERROR_SUCCESS == rc)
        {
            rbusProperty_t next = outputVals;
            for (i = 0; i < numOfOutVals; i++)
            {
                rbusValue_t val = rbusProperty_GetValue(next);

                rbusValueType_t type = rbusValue_GetType(val);
                printf ("Parameter %2d:\n", i+1);
                printf ("              Name  : %s\n", rbusProperty_GetName(next));
                if(type == RBUS_BOOLEAN)
                {

                    printf ("              Type  : %s\n", "boolean");
                    printf ("              Value : %s\n", rbusValue_GetBoolean(val) ? "true" : "false");
                }
                else if(type == RBUS_INT16)
                {
                    printf ("              Type  : %s\n", "int16");
                    printf ("              Value : %d\n", rbusValue_GetInt16(val));
                }
                else if(type == RBUS_UINT16)
                {
                    printf ("              Type  : %s\n", "uint16");
                    printf ("              Value : %u\n", rbusValue_GetUInt16(val));
                }
                else if(type == RBUS_INT32)
                {
                    printf ("              Type  : %s\n", "int32");
                    printf ("              Value : %d\n", rbusValue_GetInt32(val));
                }
                else if(type == RBUS_UINT32)
                {
                    printf ("              Type  : %s\n", "uint32");
                    printf ("              Value : %u\n", rbusValue_GetUInt32(val));
                }
                else if(type == RBUS_INT64)
                {
                    printf ("              Type  : %s\n", "int64");
                    printf ("              Value : %" PRId64 "\n", rbusValue_GetInt64(val));
                }
                else if(type == RBUS_UINT64)
                {
                    printf ("              Type  : %s\n", "uint64");
                    printf ("              Value : %" PRIu64 "\n", rbusValue_GetUInt64(val));
                }
                else if(type == RBUS_STRING)
                {
                    printf ("              Type  : %s\n", "string");
                    printf ("              Value : %s\n", rbusValue_GetString(val, NULL));
                }
                else if(type == RBUS_DATETIME) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "date_time");
                    printf ("              Value : %s\n", "");
                }
                else if(type == RBUS_BYTES) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "binary");
                    printf ("              Value : ");
                }
                else if(type == RBUS_SINGLE)
                {
                    printf ("              Type  : %s\n", "float");
                    printf ("              Value : %f\n", rbusValue_GetSingle(val));
                }
                else if(type == RBUS_DOUBLE)
                {
                    printf ("              Type  : %s\n", "double");
                    printf ("              Value : %f\n", rbusValue_GetDouble(val));
                }
                else if(type == RBUS_BYTE) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "byte");
                    printf ("              Value : 0x%x\n", rbusValue_GetByte(val));
                }
#if 0   //mrollins switched all these types to be RBUS_STRING
                else if(type == RBUS_BASE64) /* FIXME: print this data value */
                {
                    printf ("              Type  : %s\n", "base64");
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
#endif
                next = rbusProperty_GetNext(next);
            }
            /* Free the memory */
            rbusProperty_Release(outputVals);
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

void validate_and_execute_addrow_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    char *pTablePathName = NULL;
    char *pTableAliasName = NULL;
    uint32_t instanceNum = 0;

    /* Is Alias Passed */
    if (argc >= 3)
    {
        pTablePathName = argv[2];

        if (argc == 4)
            pTableAliasName = argv[3];

        /* We will have it opened already in case of interactive mode */
        if (0 == g_busHandle)
        {
            char compName[50] = "";
            snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
            rc = rbus_open(&g_busHandle, compName);
        }

        if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
        {
            rc = rbusTable_addRow(g_busHandle, pTablePathName, pTableAliasName, &instanceNum);
        }

        if(RBUS_ERROR_SUCCESS == rc)
        {
            printf ("\n\n%s%d added\n\n", pTablePathName, instanceNum);
        }
        else
        {
            printf ("Add row to a table failed ..\n");
        }
    }
    else
    {
        printf ("Invalid arguments. Please see the help\n");
    }


}

void validate_and_execute_delrow_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    char *pTablePathName = NULL;

    if (argc == 3)
    {
        pTablePathName = argv[2];

        /* We will have it opened already in case of interactive mode */
        if (0 == g_busHandle)
        {
            char compName[50] = "";
            snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
            rc = rbus_open(&g_busHandle, compName);
        }

        if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
        {
            rc = rbusTable_removeRow(g_busHandle, pTablePathName);
        }

        if(RBUS_ERROR_SUCCESS == rc)
        {
            printf ("\n\n%s deleted successfully\n\n", pTablePathName);
        }
        else
        {
            printf ("\n\nDeletion of a row from table failed ..\n");
        }
    }
    else
    {
        printf ("Invalid arguments. Please see the help\n");
    }


}

void validate_and_execute_set_cmd (int argc, char *argv[])
{
    int i = argc - 2;

    /* must have 3 or multiples of 3 parameters.. it could possibliy have 1 extra param which
     * could be having commit and set to true/false */
    if ((i >= 3) && ((i % 3 == 0) || (i % 3 == 1)))
    {
        rbusError_t rc = RBUS_ERROR_SUCCESS;

        /* We will have it opened already in case of interactive mode */
        if (0 == g_busHandle)
        {
            char compName[50] = "";
            snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
            rc = rbus_open(&g_busHandle, compName);
        }

        if ((RBUS_ERROR_SUCCESS == rc) && (0 != g_busHandle))
        {
            bool isCommit = true;
            int sessionId = 0;
            static bool g_pendingCommit = false;
            if (i > 4) /* Multiple set commands; Lets use rbusValue_t */
            {
                int isInvalid = 0;
                int index = 0;
                int loopCnt = 0;
                int paramCnt = i/3;
                rbusProperty_t properties = NULL, last = NULL;
                rbusValue_t setVal[RBUS_CLI_MAX_PARAM];
                char const* setNames[RBUS_CLI_MAX_PARAM];

                for (index = 0, loopCnt = 0; index < paramCnt; loopCnt+=3, index++)
                {
                    /* Create Param w/ Name */
                    rbusValue_Init(&setVal[index]);
                    setNames[index] = argv[loopCnt+2];

                    printf ("Name = %s \n", argv[loopCnt+2]);

                    /* Get Param Type */
                    rbusValueType_t type = getDataType_fromString(argv[loopCnt+3]);

                    /* DEBUG */
                    //printf ("Type as %s\n", getDataType_toString (type));

                    if (type == RBUS_NONE)
                    {
                        printf ("Invalid data type. Please see the help\n");
                        isInvalid = 1;
                        break;
                    }

                    rbusValue_SetFromString(setVal[index], type, argv[loopCnt+4]);

                    rbusProperty_t next;
                    rbusProperty_Init(&next, setNames[index], setVal[index]);
                    if(properties == NULL)
                    {
                        properties = last = next;
                    }
                    else
                    {
                        rbusProperty_SetNext(last, next);
                        last=next;
                    }
#if 0
                    if(type == RBUS_BOOLEAN)
                    {
                        bool tmp = false;
                        if (0 == strncasecmp("true", argv[loopCnt+4], 4))
                            tmp = true;
                        else if (0 == strncasecmp("false", argv[loopCnt+4], 5))
                            tmp = false;
                        else if (0 == strncasecmp("1", argv[loopCnt+4], 1))
                            tmp = true;
                        else if (0 == strncasecmp("0", argv[loopCnt+4], 1))
                            tmp = false;
                        else
                        {
                            isInvalid = 1;
                            break;
                        }
                        rbusValue_SetBoolean(setVal[index], tmp);
                    }
                    else if(type == RBUS_INT16)
                    {
                        rbusValue_SetInt16(setVal[index], (short) atoi(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_UINT16)
                    {
                        rbusValue_SetUInt16(setVal[index], (unsigned short) atoi(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_INT32)
                    {
                        rbusValue_SetInt32(setVal[index], atoi(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_UINT32)
                    {
                        rbusValue_SetUInt32(setVal[index], (unsigned int) atoi(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_INT64)
                    {
                        rbusValue_SetInt64(setVal[index], (long long) atoi(argv[loopCnt+4]));/*FIXME should it be atoll?*/
                    }
                    else if(type == RBUS_UINT64)
                    {
                        rbusValue_SetInt64(setVal[index], (unsigned long long) atoi(argv[loopCnt+4]));/*FIXME should it be atoll?*/
                    }
                    else if(type == RBUS_STRING)
                    {
                        rbusValue_SetString(setVal[index], argv[loopCnt+4]);
                    }
                    else if(type == RBUS_DATETIME)
                    {
                        //TODO must parse argv[loopCnt+4] into a struct timeval;
                        //rbusValue_SetTime(setVal[index], argv[loopCnt+4]);
                    }
                    else if(type == RBUS_BYTES)
                    {
                        //FIXME how are bytes passed on command line ?
                        rbusValue_SetBytes(setVal[index], (uint8_t*)argv[loopCnt+4], strlen(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_SINGLE)
                    {
                        rbusValue_SetSingle(setVal[index], (float) atof(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_DOUBLE)
                    {
                        rbusValue_SetSingle(setVal[index], atof(argv[loopCnt+4]));
                    }
                    else if(type == RBUS_BYTE)
                    {
                        //FIXME - how are bytes passed on command line (e.g. are they hex values that we need to parse ?)
                        rbusValue_SetByte(setVal[index], (unsigned char)argv[loopCnt+4][0]);
                    }
#if 0   //mrollins switched all these types to be RBUS_STRING
                    else if(type == RBUS_BASE64)
                    {
                        rbusValue_SetString(setVal[index], argv[loopCnt+4]);
                    }
                    else if(type == RBUS_REFERENCE)
                    {
                        rbusValue_SetString(setVal[index], argv[loopCnt+4]);//FIXME - what is a reference ?
                    }
                    else if(type == RBUS_EVENT_DEST_NAME)
                    {
                        rbusValue_SetString(setVal[index], argv[loopCnt+4]);//FIXME - do we need to add RBUS_EVENT_DEST_NAME type to rbusValue_t ?
                    }
#endif
#endif
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
                                isCommit = true;
                            else if (strncasecmp ("false", argv[argc - 1], 5) == 0)
                                isCommit = false;
                            else
                                isCommit = true;

                            if (isCommit == false)
                            {
                                g_pendingCommit = true;
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
                            isCommit = true;
                            if (g_pendingCommit)
                                sessionId = RBUS_CLI_SESSION_ID;
                            else
                                sessionId = 0;
                        }
                    }
                    else
                    {
                        /* For non-interactive mode, regardless of COMMIT value tha is passed, we assume the isCommit as TRUE */
                        isCommit = true;
                        sessionId = 0;
                    }

                    /* Reset the flag */
                    if (isCommit == true)
                        g_pendingCommit = false;


                    rbusSetOptions_t opts = {isCommit,sessionId};
                    rc = rbus_setMulti(g_busHandle, paramCnt, properties/*setNames, setVal*/, &opts);
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
                    rbusValue_Release(setVal[loopCnt]);
                }
                rbusProperty_Release(properties);
            }
            else /* Single Set Command */
            {
                /* Get Param Type */
                rbusValueType_t type = getDataType_fromString(argv[3]);

                /* DEBUG */
                //printf ("Type as %s\n", getDataType_toString (type));
                if (type != RBUS_NONE)
                {
                    rbusValue_t setVal;

                    /* Create Param w/ Name */
                    rbusValue_Init(&setVal);
                    rbusValue_SetFromString(setVal, type, argv[4]);
#if 0
                    if(type == RBUS_BOOLEAN)
                    {
                        bool tmp = false;
                        if (0 == strncasecmp("true", argv[4], 4))
                            tmp = true;
                        else if (0 == strncasecmp("false", argv[4], 5))
                            tmp = true;
                        else if (0 == strncasecmp("1", argv[4], 1))
                            tmp = true;
                        else if (0 == strncasecmp("0", argv[4], 1))
                            tmp = true;
                        rbusValue_SetBoolean(setVal, tmp);
                    }
                    else if(type == RBUS_INT16)
                    {
                        rbusValue_SetInt16(setVal, (short) atoi(argv[4]));
                    }
                    else if(type == RBUS_UINT16)
                    {
                        rbusValue_SetUInt16(setVal, (unsigned short) atoi(argv[4]));
                    }
                    else if(type == RBUS_INT32)
                    {
                        rbusValue_SetInt32(setVal, atoi(argv[4]));
                    }
                    else if(type == RBUS_UINT32)
                    {
                        rbusValue_SetUInt32(setVal, (unsigned int) atoi(argv[4]));
                    }
                    else if(type == RBUS_INT64)
                    {
                        rbusValue_SetInt64(setVal, (long long) atoi(argv[4]));
                    }
                    else if(type == RBUS_UINT64)
                    {
                        rbusValue_SetUInt64(setVal, (unsigned long long) atoi(argv[4]));
                    }
                    else if(type == RBUS_STRING)
                    {
                        rbusValue_SetString(setVal, argv[4]);
                    }
                    else if(type == RBUS_DATETIME)
                    {
                        //TODO must parse argv[4] into a struct timeval;
                        //rbusValue_SetTime(setVal, argv[4]);
                    }
                    else if(type == RBUS_BYTES)
                    {
                        //FIXME how are bytes passed on command line ?
                        rbusValue_SetBytes(setVal, (uint8_t*)argv[4], strlen(argv[4]));
                    }
                    else if(type == RBUS_SINGLE)
                    {
                        rbusValue_SetSingle(setVal, (float) atof(argv[4]));
                    }
                    else if(type == RBUS_DOUBLE)
                    {
                        rbusValue_SetDouble(setVal, atof(argv[4]));
                    }
                    else if(type == RBUS_BYTE)
                    {
                        /*Was previous RBUS_BYTE, is this a single byte pass on command line ? how is it passed ?*/
                        rbusValue_SetByte(setVal, (unsigned char)argv[4][0]);
                    }
#if 0 //mrollins switched all these types to be RBUS_STRING
                    else if(type == RBUS_BASE64)
                    {
                        //FIXME base64 is really just a string -- does this work though ?
                        rbusValue_SetString(setVal, argv[4]);
                    }
                    else if(type == RBUS_REFERENCE)
                    {
                        rbusValue_SetString(setVal, argv[4]);//FIXME what is a reference ?
                    }
                    else if(type == RBUS_EVENT_DEST_NAME)
                    {
                        rbusValue_SetString(setVal, argv[4]);//FIXME does it work as a RBUS_STRING ?
                    }
#endif
#endif
                    /* Set Session ID & Commit value */
                    if (g_isInteractive)
                    {
                        if (4 == i)
                        {
                            /* Is commit */
                            if (strncasecmp ("true", argv[argc - 1], 4) == 0)
                                isCommit = true;
                            else if (strncasecmp ("false", argv[argc - 1], 5) == 0)
                                isCommit = false;
                            else
                                isCommit = true;

                            if (isCommit == false)
                            {
                                g_pendingCommit = true;
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
                            isCommit = true;
                            if (g_pendingCommit)
                                sessionId = RBUS_CLI_SESSION_ID;
                            else
                                sessionId = 0;
                        }
                    }
                    else
                    {
                        /* For non-interactive mode, regardless of COMMIT value tha is passed, we assume the isCommit as TRUE */
                        isCommit = true;
                        sessionId = 0;
                    }
                    (void)sessionId;
                    /* Reset the flag */
                    if (isCommit == true)
                        g_pendingCommit = false;

                    /* Assume a sessionId as it is going to be single entry thro this cli app; */
                    rbusSetOptions_t opts = {isCommit,sessionId};
                    rc = rbus_set(g_busHandle, argv[2], setVal, &opts);

                    /* Free the data pointer that was allocated */
                    rbusValue_Release(setVal);
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
    else if (strncmp (argv[1], "getwildcard", 4) == 0)
    {
        validate_and_execute_get_cmd (argc, argv, true);
    }
    else if (strncmp (argv[1], "getvalues", 4) == 0)
    {
        validate_and_execute_get_cmd (argc, argv, false);
    }
    else if (strncmp (argv[1], "setvalues", 4) == 0)
    {
        validate_and_execute_set_cmd (argc, argv);
    }
    else if (strncmp (argv[1], "addrow", 3) == 0)
    {
        validate_and_execute_addrow_cmd (argc, argv);
    }
    else if (strncmp (argv[1], "delrow", 3) == 0)
    {
        validate_and_execute_delrow_cmd (argc, argv);
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
                g_isInteractive = true;
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
