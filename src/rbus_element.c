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
#include <assert.h>
#include "rbus_element.h"
#include "rbus_subscriptions.h"

#define DEBUG_ELEMENTS 0

//****************************** UTILITY FUNCTIONS ***************************//
void printNtimes(char* s, int n)
{
    (void)(s);
    if(n == 0)
        return;

    while(n!=0)
    {
        RBUSLOG_INFO("%s", s);
        n--;
    }
}
char const* getTypeString(rbusElementType_t type)
{
    switch(type)
    {
    case RBUS_ELEMENT_TYPE_PROPERTY: return "property"; break;
    case RBUS_ELEMENT_TYPE_TABLE: return "table"; break;
    case RBUS_ELEMENT_TYPE_EVENT: return "event"; break;
    case RBUS_ELEMENT_TYPE_METHOD: return "method"; break;
    default: return "object"; break;
    }
}
//****************************************************************************//


//********************************* FUNCTIONS ********************************//
elementNode* getEmptyElementNode(void)
{
    elementNode* node;

    node = (elementNode *) calloc(1, sizeof(elementNode));
    node->type = 0;//default of zero means OBJECT and if this gets used as a leaf, it will get update to be a either parameter, event, or method
    return node;
}

void freeElementNode(elementNode* node)
{
    if (node->name)
    {
        free(node->name);
    }
    if (node->fullName)
    {
        free(node->fullName);
    }
    if (node->alias)
    {
        free(node->alias);
    }
    if (node->subscriptions)
    {
        rtList_Destroy(node->subscriptions, NULL);
    }

    free(node);
}

void freeAllElements(elementNode** elementRoot)
{
    elementNode* node = *elementRoot;

    if(node)
    {
        elementNode* ch = node->child;
        elementNode* sib = node->nextSibling;

        freeElementNode(node);

        if(ch)
            freeAllElements(&ch);
        if(sib)
            freeAllElements(&sib);
    }
}

elementNode* insertElement(elementNode** root, rbusDataElement_t* elem)
{
    char* token = NULL;
    char* name = NULL;
    elementNode* currentNode = *root;
    elementNode* nextNode = NULL;
    int ret = 0, createChild = 0;
    char buff[RBUS_MAX_NAME_LENGTH];

    if(currentNode == NULL)
    {
        return NULL;
    }
    nextNode = currentNode->child;
    createChild = 1;

#if DEBUG_ELEMENTS
    RBUSLOG_INFO("<%s>: Request to insert element [%s]!!", __FUNCTION__, elem->name);
#endif

    name = strdup(elem->name);

    /* If this is a table being registered using .{i}. syntax, such as
       "Device.WiFi.AccessPoint.{i}.", then we strip off the .{i}.
       because its the token before that which is the actual table name.
        e.g. "Device.WiFi.AccessPoint." is the table and {i} is a row placeholder.
       After the table is added below, we will add the {i} as child of table, 
        but this {i} will be called a row template.  And from this template
        we can instantiate all the objects and properties under it when we 
        add a row instance.
     */
    if(elem->type == RBUS_ELEMENT_TYPE_TABLE)
    {
        size_t len = strlen(name);
        if(len > 4)
        {
            if(strcmp(name + len - 5, ".{i}.") == 0)
            {
                name[len-5] = 0;
            }
            else if(strcmp(name + len - 4, ".{i}") == 0)
            {
                name[len-4] = 0;
            }
        }
    }

    token = strtok(name, ".");

    while( token != NULL )
    {
        if(nextNode == NULL)
        {
            if(createChild)
            {
#if DEBUG_ELEMENTS
                RBUSLOG_INFO("Create child [%s]", token);
#endif
                currentNode->child = getEmptyElementNode();
                currentNode->child->parent = currentNode;
                if(currentNode == *root)    
                {
                    currentNode->child->fullName = strdup(token);
                }
                else
                {
                    snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s.%s", currentNode->fullName, token);
                    currentNode->child->fullName = strdup(buff);
                }
                currentNode = currentNode->child;
                currentNode->name = strdup(token);
                nextNode = currentNode->child;
                createChild = 1;
            }
        }
        while(nextNode != NULL)
        {
#if DEBUG_ELEMENTS
            RBUSLOG_INFO("child name=[%s], Token = [%s]", nextNode->name, token);
#endif
            if(strcmp(nextNode->name, token) == 0)
            {
                currentNode = nextNode;
                nextNode = currentNode->child;
                createChild = 1;
                break;
            }
            else
            {
                currentNode = nextNode;
                nextNode = currentNode->nextSibling;
                createChild = 0;
                if(nextNode == NULL)
                {
#if DEBUG_ELEMENTS
                    RBUSLOG_INFO("Create Sibling [%s]", token);
#endif
                    currentNode->nextSibling = getEmptyElementNode();
                    currentNode->nextSibling->parent = currentNode->parent;
                    snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s.%s", currentNode->parent->fullName, token);
                    currentNode->nextSibling->fullName = strdup(buff);
                    currentNode = currentNode->nextSibling;
                    currentNode->name = strdup(token);
                    createChild = 1;
                }
            }
        }
        token = strtok(NULL, ".");
    }
    if(ret == 0)
    {
        currentNode->type = elem->type;
        currentNode->cbTable = elem->cbTable;

        /* See the big comment near the top of this function.
           We add {i} as a child object of the table.
           This will be the row template used to instantiate rows from.
           Its presumed a provider will register more elements under this, such as
            Device.WiFi.AccessPoint.{i}.Foo etc,...
         */
        if(elem->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            elementNode* rowTemplate = getEmptyElementNode();
            rowTemplate->parent = currentNode;
            currentNode->child = rowTemplate;
            rowTemplate->name = strdup("{i}");
            snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s.%s", currentNode->fullName, rowTemplate->name);
            currentNode->child->fullName = strdup(buff);
        }
    }
    free(name);
    if(ret == 0)
        return currentNode;
    else
        return NULL;
}

elementNode* retrieveElement(elementNode* root, const char* elmentName)
{
    char* token = NULL;
    char* name = NULL;
    elementNode* currentNode = root;
    elementNode* nextNode = NULL;
    int tokenFound = 0;

#if DEBUG_ELEMENTS
    RBUSLOG_INFO("<%s>: Request to retrieve element [%s]", __FUNCTION__, elmentName);
#endif
    if(currentNode == NULL)
    {
        return NULL;
    }

    name = strdup(elmentName);

    nextNode = currentNode->child;

    /*TODO if name is a table row with an alias containing a dot, this will break (e.g. "Foo.[alias.1]")*/
    token = strtok(name, ".");
    while( token != NULL)
    {
#if DEBUG_ELEMENTS
        RBUSLOG_INFO("Token = [%s]", token);
#endif
        tokenFound = 0;
        if(nextNode == NULL)
        {
            break;
        }

#if DEBUG_ELEMENTS
        RBUSLOG_INFO("child name=[%s], Token = [%s]", nextNode->name, token);
#endif
        /*
        if(nextNode->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            assert(strcmp(nextNode->name, "{i}") == 0);
            tokenFound = 1;
            currentNode = nextNode;
            nextNode = currentNode->child;
        }
        else*/
        if(strcmp(nextNode->name, token) == 0)
        {
#if DEBUG_ELEMENTS
            RBUSLOG_INFO("tokenFound!");
#endif
            tokenFound = 1;
            currentNode = nextNode;
            nextNode = currentNode->child;
        }
        else
        {
            currentNode = nextNode;
            nextNode = currentNode->nextSibling;

            while(nextNode != NULL)
            {
#if DEBUG_ELEMENTS
                RBUSLOG_INFO("child name=[%s], Token = [%s]", nextNode->name, token);
#endif
                if(strcmp(nextNode->name, token) == 0)
                {
#if DEBUG_ELEMENTS
                    RBUSLOG_INFO("tokenFound!");
#endif
                    tokenFound = 1;
                    currentNode = nextNode;
                    nextNode = currentNode->child;
                    break;
                }
                else
                {
                    currentNode = nextNode;
                    nextNode = currentNode->nextSibling;
                }
            }
        }

        token = strtok(NULL, ".");

        if(token && nextNode && nextNode->parent && nextNode->parent->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            /* retrieveElement should only return regististration elements, not table row instantiated elements */
            token = "{i}";
        }
    }

    free(name);

    if(tokenFound)
    {
#if DEBUG_ELEMENTS
        RBUSLOG_INFO("Found Element with param name [%s]", currentNode->name);
#endif
        return currentNode;
    }
    else
    {
        return NULL;
    }
}

elementNode* retrieveInstanceElement(elementNode* root, const char* elmentName)
{
    char* token = NULL;
    char* name = NULL;
    elementNode* currentNode = root;
    elementNode* nextNode = NULL;
    int tokenFound = 0;

#if DEBUG_ELEMENTS
    RBUSLOG_INFO("<%s>: Request to retrieve element [%s]", __FUNCTION__, elmentName);
#endif
    if(currentNode == NULL)
    {
        return NULL;
    }

    name = strdup(elmentName);

    nextNode = currentNode->child;

    /*TODO if name is a table row with an alias containing a dot, this will break (e.g. "Foo.[alias.1]")*/
    token = strtok(name, ".");
    while( token != NULL)
    {
#if DEBUG_ELEMENTS
        RBUSLOG_INFO("Token = [%s]", token);
#endif
        tokenFound = 0;

        if(nextNode == NULL)
        {
            break;
        }

#if DEBUG_ELEMENTS
        RBUSLOG_INFO("child name=[%s], Token = [%s]", nextNode->name, token);
#endif

        if(strcmp(nextNode->name, token) == 0)
        {
#if DEBUG_ELEMENTS
            RBUSLOG_INFO("tokenFound!");
#endif
            tokenFound = 1;
            currentNode = nextNode;
            nextNode = currentNode->child;
        }
        else
        {
            currentNode = nextNode;
            nextNode = currentNode->nextSibling;

            while(nextNode != NULL)
            {
#if DEBUG_ELEMENTS
                RBUSLOG_INFO("child name=[%s], Token = [%s]", nextNode->name, token);
#endif
                if(strcmp(nextNode->name, token) == 0)
                {
#if DEBUG_ELEMENTS
                    RBUSLOG_INFO("tokenFound!");
#endif
                    tokenFound = 1;
                    currentNode = nextNode;
                    nextNode = currentNode->child;
                    break;
                }
                else
                {
                    /*check the alias if its a table row*/
                    if(nextNode->parent->type == RBUS_ELEMENT_TYPE_TABLE)
                    {
                        if(nextNode->alias)
                        {
                            size_t tlen = strlen(token);

                            if(tlen > 2 && token[0] == '[' && token[tlen-1] == ']')
                            {
                                if(strncmp(nextNode->alias, token+1, tlen-2) == 0)
                                {
#if DEBUG_ELEMENTS
                                    RBUSLOG_INFO("tokenFound by alias %s!", nextNode->alias);
#endif
                                    tokenFound = 1;
                                    currentNode = nextNode;
                                    nextNode = currentNode->child;
                                    break;
                                }
                            }
                        }
                    }

                    currentNode = nextNode;
                    nextNode = currentNode->nextSibling;
                }
            }
        }

        token = strtok(NULL, ".");
    }

    free(name);

    if(tokenFound)
    {
#if DEBUG_ELEMENTS
        RBUSLOG_INFO("Found Element with param name [%s]", currentNode->name);
#endif
        return currentNode;
    }
    else
    {
        return NULL;
    }
}


int findImmediateMatchingNode(elementNode* parent, char* token, elementNode** curr, elementNode** prev)
{
    elementNode *prevSibling = NULL, *sibling = NULL;
    int ret = -1;
    if(parent == NULL || token == NULL)
    {
        return ret;
    }
    //RBUSLOG_DEBUG("child name=[%s], Token = [%s]", parent->name, token);
    if(parent->child)
    {
        if(strcmp(parent->child->name, token) == 0)
        {
            //RBUSLOG_DEBUG("tokenFound!");
            *curr = parent->child;
            *prev = parent;
            ret = 0;
        }
        else if(parent->child->nextSibling)
        {
            prevSibling = parent->child;
            sibling = parent->child->nextSibling;
            while(sibling)
            {
                if(strcmp(sibling->name, token) == 0)
                {
                    //RBUSLOG_DEBUG("tokenFound!");
                    *curr = sibling;
                    *prev = prevSibling;
                    ret = 0;
                    break;
                }
                prevSibling = sibling;
                sibling = sibling->nextSibling;
            }
        }
    }
    return ret;
}
int findAndFreeNodeRecursively(elementNode* parent, char* token)
{
    elementNode *curr = NULL, *prev = NULL;
    int ret = -1;
    //RBUSLOG_DEBUG("parent node = [%s], token = [%s]", parent->name, token);
    ret = findImmediateMatchingNode(parent, token, &curr, &prev);
    if(ret == 0)
    {
        int ret = -1;
        token = strtok(NULL, ".");
        if(token == NULL)
        {
	    //RBUSLOG_DEBUG("All tokens found!!");
            if(prev->nextSibling == curr)
            {
                prev->nextSibling = curr->nextSibling;
                freeElementNode(curr);
            }
            else if(prev->child == curr)
            {
                prev->child = curr->nextSibling? curr->nextSibling: curr->child;
                freeElementNode(curr);
            }
            ret = 0;
        }
        else
        {
            ret = findAndFreeNodeRecursively(curr, token);
            if(ret == 0)
            {
                if(curr->child == NULL)
                {
                    if(prev == parent)
                    {
                        prev->child = curr->nextSibling? curr->nextSibling: curr->child;
                        freeElementNode(curr);
                    }
                    else
                    {
                        prev->nextSibling = curr->nextSibling;
                        freeElementNode(curr);
                    }
                }
                else
                {
                    ret = -1;
                }
            }
        }
    }
    else
    {
       //RBUSLOG_DEBUG("ERROR finding immediate matching node!");
    }
    return ret;
}

int removeElement(elementNode** root, char const* elementName)
{
    char* token = NULL;
    char* name = NULL;
    elementNode* parent = *root;
    int ret = -1;

    if(parent == NULL)
    {
        return -1;
    }

    name = strdup(elementName);

    //RBUSLOG_DEBUG("<%s>: Request to remove element [%s]!!", __FUNCTION__, elementName);
    token = strtok(name, ".");
    //RBUSLOG_DEBUG("Token = [%s]", token);
    ret = findAndFreeNodeRecursively(parent, token);
    free(name);
    if(ret != 0)
    {
        //RBUSLOG_DEBUG("Element not found[%s]", elementName);
    }
    return ret;
}

static void printElement(elementNode* node, int level)
{
    printNtimes("  ", level);
    RBUSLOG_INFO("[name:%s type:%s fullName:%s addr:%p, parent:%p %s%s]", 
        node->name, 
        getTypeString(node->type),
        node->fullName,
        node,
        node->parent,
        node->alias ? "alias:" : "",
        node->alias ? node->alias : "");
}

void printRegisteredElements(elementNode* root, int level)
{
    elementNode* child = root;
    elementNode* sibling = NULL;

    if(child)
    {
        printElement(child, level);
        if(child->child)
        {
            printRegisteredElements(child->child, level+1);
        }
        sibling = child->nextSibling;
        while(sibling)
        {
            printElement(sibling, level);
            if(sibling->child)
            {
                printRegisteredElements(sibling->child, level+1);
            }
            sibling = sibling->nextSibling;
        }
    }
}

void addElementSubscription(elementNode* node, rbusSubscription_t* sub, bool checkIfExists)
{
    if(checkIfExists && node->subscriptions)
    {
        rtListItem item;
        rbusSubscription_t* data;

        if(node->subscriptions)
        {
            rtList_GetFront(node->subscriptions, &item);

            while(item)
            {
                rtListItem_GetData(item, (void**)&data);
                if(data == sub)
                {
                    /*already exists so return*/
                    return;
                }
                rtListItem_GetNext(item, &item);
            }
        }
    }

    if(!node->subscriptions)
    {
        rtList_Create(&node->subscriptions);
    }

    rtList_PushBack(node->subscriptions, sub, NULL);
}

void removeElementSubscription(elementNode* node, rbusSubscription_t* sub)
{
    rtListItem item;
    rbusSubscription_t* data;

    if(node->subscriptions)
    {
        rtList_GetFront(node->subscriptions, &item);

        while(item)
        {
            rtListItem_GetData(item, (void**)&data);
            if(data == sub)
            {
                rtList_RemoveItem(node->subscriptions, item, NULL);
                return;
            }
            rtListItem_GetNext(item, &item);
        }
    }
}

bool elementHasAutoPubSubscriptions(elementNode* node, rbusSubscription_t* excluding)
{
    if(node->subscriptions)
    {
        rtListItem item;
        rbusSubscription_t* sub;

        rtList_GetFront(node->subscriptions, &item);

        while(item)
        {
            rtListItem_GetData(item, (void**)&sub);

            if(sub->autoPublish)
            {
                if(excluding != sub)
                {
                    return true;
                }
            }

            rtListItem_GetNext(item, &item);
        }
    }
    return false;
}

/*
    Example tree:

    Device.WiFi.AccessPoint.{i}.
    Device.WiFi.AccessPoint.{i}.Prop1
    Device.WiFi.AccessPoint.{i}.OtherObject.Property2
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength

    duplicateNode(
        sourceNode=Device.WiFi.AccessPoint.{i}, 
        parentNode=Device.WiFi.AccessPoint, 
        name="1")

    Result:

    Device.WiFi.AccessPoint.{i}.
    Device.WiFi.AccessPoint.{i}.Prop1
    Device.WiFi.AccessPoint.{i}.OtherObject.Property2
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength
    Device.WiFi.AccessPoint.1.Prop1
    Device.WiFi.AccessPoint.1.OtherObject.Property2
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.SignalStrength

 */
static elementNode* duplicateNode(elementNode* sourceNode, elementNode* parentNode, char const* name )
{
    elementNode* node;
    elementNode* child;
    char fullName[RBUS_MAX_NAME_LENGTH];

    node = getEmptyElementNode();

    snprintf(fullName, RBUS_MAX_NAME_LENGTH, "%s.%s", parentNode->fullName, name);
    node->fullName = strdup(fullName);
    node->name = strdup(name);
    node->type = sourceNode->type;
    node->cbTable = sourceNode->cbTable;
    node->parent = parentNode;

    /*add new node to the parent's child list*/
    if(parentNode->child)
    {
        child = parentNode->child;
        while(child->nextSibling)
            child = child->nextSibling;
        child->nextSibling = node;
    }
    else
    {
        parentNode->child = node;
    }

    /*duplicate children of sourceNode*/
    child = sourceNode->child;
    while(child)
    {
        duplicateNode(child, node, child->name);
        child = child->nextSibling;
    }

    return node;
}

/*
    Lets say we have this example registered in tree:

    Device.WiFi.AccessPoint.{i}.
    Device.WiFi.AccessPoint.{i}.Prop1
    Device.WiFi.AccessPoint.{i}.OtherObject.Property2
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength

    Now a table row with instNum 1 is created under this table:
        Device.WiFi.AccessPoint.{i}.

    We need to create the following in tree:

    Device.WiFi.AccessPoint.1.
    Device.WiFi.AccessPoint.1.Prop1
    Device.WiFi.AccessPoint.1.OtherObject.Property2
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.SignalStrength

    Steps:
        Create node Device.WiFi.AccessPoint.1.
        Duplicate the entire node tree under Device.WiFi.AccessPoint.{i}.
        Set the parent of the duplicated node tree to Device.WiFi.AccessPoint.1.

    @param tableNode        The node with name {i} of type RBUS_ELEMENT_TYPE_TABLE
    @param instNum          The new row's instance number
    @param alias            The new row's instance alias (Optional)
*/

elementNode* instantiateTableRow(elementNode* tableNode, uint32_t instNum, char const* alias)
{
    elementNode* rowTemplate;
    char name[32];

#if DEBUG_ELEMENTS
    RBUSLOG_INFO("%s: table=%s instNum=%u alias=%s", __FUNCTION__, tableNode->fullName, instNum, alias);
    printElement(tableNode, 0);
#endif

#if DEBUG_ELEMENTS
    {
        elementNode* root = tableNode;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### BEFORE INSTANTION ###################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#########################################################");
    }
#endif

    /*find the row template which has name="{i}"*/

    rowTemplate = tableNode->child;
    while(rowTemplate)
    {
        if(strcmp(rowTemplate->name, "{i}") == 0)
            break;
        rowTemplate = rowTemplate->nextSibling;
    }

    if(!rowTemplate)
    {
        assert(false);
        RBUSLOG_ERROR("%s ERROR: row template not found for table %s", __FUNCTION__, tableNode->fullName);
        return NULL;
    }

    snprintf(name, 32, "%u", instNum);

    elementNode* row = duplicateNode(rowTemplate, tableNode, name);

    if(alias)
    {
        row->alias = strdup(alias);
    }

#if DEBUG_ELEMENTS
    {
        elementNode* root = tableNode;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### AFTER INSTANTION ####################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#########################################################");
    }
#endif


    return row;
}

void deleteTableRow(elementNode* rowNode)
{
    elementNode* parent = rowNode->parent;

#if DEBUG_ELEMENTS
    RBUSLOG_INFO("%s: row=%s", __FUNCTION__, rowNode->fullName);
    printElement(rowNode, 0);
    printElement(rowNode->parent, 0);
#endif

#if DEBUG_ELEMENTS
    {
        elementNode* root = rowNode;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### BEFORE DELETE ###################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#####################################################");
    }
#endif

    /*remove node from parent*/
    if(parent)
    {
        bool found = false;
        if(parent->child == rowNode)
        {
            found = true;
            parent->child = rowNode->nextSibling;
        }
        else
        {
            elementNode* child = parent->child;
            while(child)
            {
                if(child->nextSibling == rowNode)
                {
                    found = true;
                    child->nextSibling = rowNode->nextSibling;
                    break;
                }
                child = child->nextSibling;
            }
        }

        if(found)
        {
#if DEBUG_ELEMENTS
            RBUSLOG_INFO("%s: row removed from parent %s", __FUNCTION__, parent->fullName);
#endif
        }
        else
        {
            RBUSLOG_INFO("%s: failed to find child in parent", __FUNCTION__);
        }
    }
    else
    {
        RBUSLOG_INFO("%s: row doesn't have a parent", __FUNCTION__);
    }

    /* free node's children then free node */
    freeAllElements(&rowNode->child);
    freeElementNode(rowNode);

#if DEBUG_ELEMENTS
    if(parent)
    {
        elementNode* root = parent;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### AFTER DELETE ####################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#####################################################");
    }
#endif

}

