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


//****************************** UTILITY FUNCTIONS ***************************//
void printNtimes(char* s, int n)
{
    (void)(s);
    if(n == 0)
        return;

    while(n!=0)
    {
        //printf("%s", s);
        n--;
    }
}
//****************************************************************************//


//********************************* FUNCTIONS ********************************//
elementNode* getEmptyElementNode(void)
{
    elementNode* node;

    node = (elementNode *) malloc(sizeof(elementNode));

    node->name = NULL;
    node->cbTable = NULL;
    node->isLeaf = 0;
    node->child = NULL;
    node->nextSibling = NULL;
    node->type = ELEMENT_TYPE_OBJECT;//default to object and if this gets used as a leaf, it will get update to be a parameter or event
    return node;
}

void freeElementNode(elementNode* node)
{
    if (node->name)
    {
        free(node->name);
    }
    if(node->cbTable)
    {
        free(node->cbTable);
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

elementNode* insertElement(elementNode** root, rbus_dataElement_t* el)
{
    char* token = NULL;
    char* name = NULL;
    elementNode* currentNode = *root;
    elementNode* nextNode = NULL;
    int ret = 0, createChild = 0;

    if(currentNode == NULL)
    {
        return NULL;
    }
    nextNode = currentNode->child;
    createChild = 1;

    //printf("<%s>: Request to insert element [%s]!!\n", __FUNCTION__, el->elementName);

    name = strdup(el->elementName);

    token = strtok(name, ".");
    while( token != NULL )
    {
        if(nextNode == NULL)
        {
            if(createChild)
            {
                //printf("Create child [%s]\n", token);
                currentNode->child = getEmptyElementNode();
                currentNode = currentNode->child;
                currentNode->name = strdup(token);
                nextNode = currentNode->child;
                createChild = 1;
            }
        }
        while(nextNode != NULL)
        {
            //printf("child name=[%s], Token = [%s]\n", nextNode->name, token);
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
                    //printf("Create Sibling [%s]\n", token);
                    currentNode->nextSibling = getEmptyElementNode();
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
        //printf("Filling cbTable and making it as leaf node!\n");
        currentNode->cbTable = (rbus_callbackTable_t*)malloc(sizeof(rbus_callbackTable_t));
        currentNode->cbTable->rbus_getHandler = el->table.rbus_getHandler;
        currentNode->cbTable->rbus_setHandler = el->table.rbus_setHandler;
        currentNode->cbTable->rbus_updateTableHandler = el->table.rbus_updateTableHandler;
        currentNode->cbTable->rbus_eventSubHandler = el->table.rbus_eventSubHandler;

        //i may need to review this again later
        //need to talk to Hari about whether this leaf elementNode is meant only for parameter/events
        //because we need to store object tables somewhere too and I don't think they are leaf
        if(currentNode->cbTable->rbus_getHandler || currentNode->cbTable->rbus_setHandler)
        {
            currentNode->type = ELEMENT_TYPE_PARAMETER;
        }
        else if(currentNode->cbTable->rbus_eventSubHandler)
        {
            currentNode->type = ELEMENT_TYPE_EVENT;
        }
        else if(currentNode->cbTable->rbus_updateTableHandler)
        {
            currentNode->type = ELEMENT_TYPE_OBJECT_TABLE;//needs review because wouldn't be a leaf
        }

        currentNode->isLeaf = 1;
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

    name = strdup(elmentName);

    //printf("<%s>: Request to retrieve element [%s]!!\n", __FUNCTION__, name);

    if(currentNode == NULL)
    {
        free(name);
        return NULL;
    }
    nextNode = currentNode->child;

    token = strtok(name, ".");
    while( token != NULL)
    {
        //printf("Token = [%s]\n", token);

        //printf("Reset tokenFound!\n");
        tokenFound = 0;
        if(nextNode == NULL)
        {
            break;
        }

        //printf("child name=[%s], Token = [%s]\n", nextNode->name, token);

        if(strcmp(nextNode->name, token) == 0)
        {
            //printf("tokenFound!\n");
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
                //printf("child name=[%s], Token = [%s]\n", nextNode->name, token);
                if(strcmp(nextNode->name, token) == 0)
                {
                    //printf("tokenFound!\n");
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
    }

    free(name);

    if(tokenFound)
    {
        //printf("Found Element with param name [%s]\n", currentNode->name);
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
    //printf("child name=[%s], Token = [%s]\n", parent->name, token);
    if(parent->child)
    {
        if(strcmp(parent->child->name, token) == 0)
        {
            //printf("tokenFound!\n");
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
                    //printf("tokenFound!\n");
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
    //printf("parent node = [%s], token = [%s]\n", parent->name, token);
    ret = findImmediateMatchingNode(parent, token, &curr, &prev);
    if(ret == 0)
    {
        int ret = -1;
        token = strtok(NULL, ".");
        if(token == NULL)
        {
	    //printf("All tokens found!!\n");
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
       //printf("ERROR finding immediate matching node!\n");
    }
    return ret;
}
int removeElement(elementNode** root, rbus_dataElement_t* el)
{
    char* token = NULL;
    char* name = NULL;
    elementNode* parent = *root;
    int ret = -1;

    if(parent == NULL)
    {
        return -1;
    }

    name = strdup(el->elementName);

    //printf("<%s>: Request to remove element [%s]!!\n", __FUNCTION__, name);
    token = strtok(name, ".");
    //printf("Token = [%s]\n", token);
    ret = findAndFreeNodeRecursively(parent, token);
    free(name);
    if(ret != 0)
    {
        //printf("Element not found[%s]\n", el->elementName);
    }
    return ret;
}

void printRegisteredElements(elementNode* root, int level)
{
    elementNode* child = root;
    elementNode* sibling = NULL;

    if(child)
    {
        printNtimes("\t", level);
        //printf("[%s]", child->name);
        if(child->child)
        {
            //printf("\n");
            printRegisteredElements(child->child, level+1);
        }
        sibling = child->nextSibling;
        while(sibling)
        {
            //printf("\n");
            printNtimes("\t", level);
            //printf("[%s]", sibling->name);
            if(sibling->child)
            {
                //printf("\n");
                printRegisteredElements(sibling->child, level+1);
            }
            sibling = sibling->nextSibling;
        }
    }
}
