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
#ifndef __RBUS_CALLBACK_H
#define __RBUS_CALLBACK_H

#include <stdlib.h>
#include <rbus.h>

#include <rbus_core.h>

#ifdef __cplusplus
extern "C" {
#endif

//******************************* STRUCTURES *********************************//
typedef struct elementNode elementNode;

typedef enum
{
    ELEMENT_TYPE_OBJECT,        //e.g. Device.WiFi
    ELEMENT_TYPE_OBJECT_TABLE,  //e.g. Device.WiFi.Radio.{i}.
    ELEMENT_TYPE_PARAMETER,     //e.g. Device.DeviceInfo.Manufacturer
    ELEMENT_TYPE_EVENT,         //e.g. Device.Boot!
} elementType_t;

struct elementNode
{
    char*                   name;       // object or param name, not full name
    rbus_callbackTable_t*   cbTable;    // Valid only for the leaf node
    int                     isLeaf;     // 1 for leaf node
    elementNode*            child;      // Downward
    elementNode*            nextSibling;// Right
    elementType_t           type;
};

//******************************* FUNCTIONS *********************************//
elementNode* getEmptyElementNode(void);
void freeElementNode(elementNode* node);
void freeAllElements(elementNode** elementRoot);
elementNode* insertElement(elementNode** root, rbus_dataElement_t* el);
int removeElement(elementNode** root, rbus_dataElement_t* el);
void printRegisteredElements(elementNode* root, int level);
elementNode* retrieveElement(elementNode* root, const char* elmentName);

#ifdef __cplusplus
}
#endif
#endif
