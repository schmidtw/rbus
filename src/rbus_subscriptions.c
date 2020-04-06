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

#include "rbus_subscriptions.h"
#include <memory.h>



struct _rbusSubscriptions
{
    rbusHandle_t handle;
    elementNode* root;
    rtList subList;
};

int subscriptionKeyCompare(rbusSubscription_t* subscription, char const* listener, char const* eventName, void* filter)
{
    int rc;
    rc = strcmp(subscription->listener, listener);
    if(rc == 0)
    {
        rc = strcmp(subscription->eventName, eventName);
        (void)filter;
        /* TODO compare filter
        if(rc == 0)
        {
        }
        */
    }
    return rc;
}

void subscriptionFree(void* p)
{
    rbusSubscription_t* sub = p;
    rtListItem item;

    rtList_GetFront(sub->instances, &item);
    while(item)
    {
        elementNode* node;
        rtListItem_GetData(item, (void**)&node);
        removeElementSubscription(node, sub);
        rtListItem_GetNext(item, &item);
    }
    if(sub->tokens)
        TokenChain_destroy(sub->tokens);
    if(sub->instances)
        rtList_Destroy(sub->instances, NULL);
    free(sub->eventName);
    free(sub->listener);
    free(sub);
}

void rbusSubscriptions_create(rbusSubscriptions_t* subscriptions, rbusHandle_t handle, elementNode* root)
{
    *subscriptions = malloc(sizeof(struct _rbusSubscriptions));
    (*subscriptions)->handle = handle;
    (*subscriptions)->root = root;
    rtList_Create(&(*subscriptions)->subList);
}

/*destroy a subscriptions registry*/
void rbusSubscriptions_destroy(rbusSubscriptions_t subscriptions)
{
    rtList_Destroy(subscriptions->subList, subscriptionFree);
    free(subscriptions);
}

void rbusSubscriptions_onSubscriptionCreated(rbusSubscription_t* sub, elementNode* node);

/*add a new subscription*/
rbusSubscription_t* rbusSubscriptions_addSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, void* filter, bool autoPublish, elementNode* registryElem)
{
    rbusSubscription_t* sub;
    TokenChain* tokens;

    tokens = TokenChain_create(eventName, registryElem);
    if(tokens == NULL)
    {
        rtLog_Error("%s: invalid token chain\n", __FUNCTION__);
        return NULL;
    }

    sub = malloc(sizeof(rbusSubscription_t));

    sub->listener = strdup(listener);
    sub->eventName = strdup(eventName);
    sub->filter = filter;//TODO
    sub->autoPublish = autoPublish;
    sub->element = registryElem;
    sub->tokens = tokens;
    rtList_Create(&sub->instances);
    rtList_PushBack(subscriptions->subList, sub, NULL);

    rbusSubscriptions_onSubscriptionCreated(sub, subscriptions->root);
    return sub;
}

/*get an existing subscription by searching for its unique key [eventName, listener, filter]*/
rbusSubscription_t* rbusSubscriptions_getSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, void* filter)
{
    rtListItem item;
    rbusSubscription_t* sub;

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub);

        if(subscriptionKeyCompare(sub, listener, eventName, filter) == 0)
        {
            return sub;
        }
        rtListItem_GetNext(item, &item);
    }
    return NULL;
}

/*remove an existing subscription*/
void rbusSubscriptions_removeSubscription(rbusSubscriptions_t subscriptions, rbusSubscription_t* sub)
{
    rtListItem item;
    rbusSubscription_t* sub2;

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub2);
        if(sub == sub2)
        {
            rtList_RemoveItem(subscriptions->subList, item, subscriptionFree);
            break;
        }
        rtListItem_GetNext(item, &item);
    }    
}

/*  called after a new subscription is created 
 *  we go through the element tree and check to see if the 
 *  new subscription matches any existing instance nodes
 */
void rbusSubscriptions_onSubscriptionCreated(rbusSubscription_t* sub, elementNode* node)
{
    if(node)
    {
        elementNode* child = node->child;

        while(child)
        {
            if(child->type != 0)
            {
                if(TokenChain_match(sub->tokens, child))
                {
                    rtList_PushBack(sub->instances, child, NULL);
                    addElementSubscription(child, sub, false);
                }
            }

            /*recurse into children except for row templates {i}*/
            if( child->child && !(child->parent->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0) )
            {
                
                rbusSubscriptions_onSubscriptionCreated(sub, child);
            }

            child = child->nextSibling;
        }
    }
}

/*  called after a new node instance is created 
 *  we go through the list of subscriptions and check to see if the 
 *  new node is picked up by any subscription eventName
 */
void rbusSubscriptions_onElementCreated(rbusSubscriptions_t subscriptions, elementNode* node)
{
    if(node)
    {
        elementNode* child = node->child;

        while(child)
        {
            if(child->type == 0)
            {
                rbusSubscriptions_onElementCreated(subscriptions, child);
            }
            else
            {
                rtListItem item;
                rbusSubscription_t* sub;

                rtList_GetFront(subscriptions->subList, &item);

                while(item)
                {
                    rtListItem_GetData(item, (void**)&sub);

                    if(TokenChain_match(sub->tokens, child))
                    {
                        rtList_PushBack(sub->instances, child, NULL);
                        addElementSubscription(child, sub, false);
                        break;
                    }
                    rtListItem_GetNext(item, &item);
                }

                /*we dont recurse into child because either child is a leaf (e.g. property/method/event)
                  or child is a table that doesn't have any rows yet, since its brand new */
            }

            child = child->nextSibling;
        }
    }
}

/*called before a node instance is deleted */
void rbusSubscriptions_onElementDeleted(rbusSubscriptions_t subscriptions, elementNode* node)
{
    if(node)
    {
        elementNode* child = node->child;

        while(child)
        {
            /*if child's type is a subscribable type*/
            if(child->type != 0)
            {
                rtListItem item;
                rbusSubscription_t* sub;

                rtList_GetFront(subscriptions->subList, &item);

                /* go through all subscriptions */
                while(item)
                {
                    rtListItem item2;
                    elementNode* inst;

                    rtListItem_GetData(item, (void**)&sub);

                    rtList_GetFront(sub->instances, &item2);

                    /* go through all instances in this subscription */
                    while(item2)
                    {
                        rtListItem_GetData(item2, (void**)&inst);

                        if(child == inst)
                        {
                            rtList_RemoveItem(sub->instances, item2, NULL);
                            removeElementSubscription(child, sub);
                            break;
                        }
                        rtListItem_GetNext(item2, &item2);
                    }

                    rtListItem_GetNext(item, &item);
                } 
            }

            /*recurse into children except for row templates {i}*/
            if( child->child && !(child->parent->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0) )
            {
                rbusSubscriptions_onElementDeleted(subscriptions, child);
            }

            child = child->nextSibling;
        }
    }
}

void rbusSubscriptions_onTableRowAdded(rbusSubscriptions_t subscriptions, elementNode* node)
{
    rbusSubscriptions_onElementCreated(subscriptions, node);
}

void rbusSubscriptions_onTableRowRemoved(rbusSubscriptions_t subscriptions, elementNode* node)
{
    rbusSubscriptions_onElementDeleted(subscriptions, node);
}

