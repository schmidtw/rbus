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
#include <time.h>
#include <sys/time.h>
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

rbusValueType_t getDataType_fromString(const char* pType)
{
    rbusValueType_t rc = RBUS_NONE;

    if (strncasecmp ("boolean",    pType, 4) == 0)
        rc = RBUS_BOOLEAN;
    else if (strncasecmp("char",   pType, 4) == 0)
        rc = RBUS_CHAR;
    else if (strncasecmp("byte",   pType, 4) == 0)
        rc = RBUS_BYTE;
    else if (strncasecmp("int8",   pType, 4) == 0)
        rc = RBUS_INT8;
    else if (strncasecmp("uint8",   pType, 5) == 0)
        rc = RBUS_UINT8;
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
    else if (strncasecmp("single",  pType, 5) == 0)
        rc = RBUS_SINGLE;
    else if (strncasecmp("double", pType, 6) == 0)
        rc = RBUS_DOUBLE;
    else if (strncasecmp("datetime", pType, 4) == 0)
        rc = RBUS_DATETIME;
    else if (strncasecmp("string", pType, 6) == 0)
        rc = RBUS_STRING;
    else if (strncasecmp("bytes",    pType, 3) == 0)
        rc = RBUS_BYTES;

    /* Risk handling, if the user types just int, lets consider int32; same for unsigned too  */
    else if (strncasecmp("int",  pType, 3) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint",  pType, 4) == 0)
        rc = RBUS_UINT32;

    return rc;
}

char *getDataType_toString(rbusValueType_t type)
{
    char *pTextData = "None";
    switch(type)
    {
    case RBUS_BOOLEAN:
        pTextData = "boolean";
        break;
    case RBUS_CHAR :
        pTextData = "char";
        break;
    case RBUS_BYTE:
        pTextData = "byte";
        break;
    case RBUS_INT8:
        pTextData = "int8";
        break;
    case RBUS_UINT8:
        pTextData = "uint8";
        break;
    case RBUS_INT16:
        pTextData = "int16";
        break;
    case RBUS_UINT16:
        pTextData = "uint16";
        break;
    case RBUS_INT32:
        pTextData = "int32";
        break;
    case RBUS_UINT32:
        pTextData = "uint32";
        break;
    case RBUS_INT64:
        pTextData = "int64";
        break;
    case RBUS_UINT64:
        pTextData = "uint64";
        break;
    case RBUS_STRING:
        pTextData = "string";
        break;
    case RBUS_DATETIME:
        pTextData = "datetime";
        break;
    case RBUS_BYTES:
        pTextData = "bytes";
        break;
    case RBUS_SINGLE:
        pTextData = "float";
        break;
    case RBUS_DOUBLE:
        pTextData = "double";
        break;
    }
    return pTextData ;
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
                char *pStrVal = rbusValue_ToString(val,NULL,0);

                printf ("Parameter %2d:\n", i+1);
                printf ("              Name  : %s\n", rbusProperty_GetName(next));
                printf ("              Type  : %s\n", getDataType_toString(type));
                printf ("              Value : %s\n", pStrVal);

                if(pStrVal)
                    free(pStrVal);

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

                if (type != RBUS_NONE)
                {
                    rbusValue_t setVal;

                    /* Create Param w/ Name */
                    rbusValue_Init(&setVal);
                    rbusValue_SetFromString(setVal, type, argv[4]);
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
