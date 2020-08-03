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

#ifndef RBUS_INSTANCETREE_H
#define RBUS_INSTANCETREE_H

#include "rbus_element.h"
#include "rbus_tokenchain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rbusSubscriptions *rbusSubscriptions_t;

/* The unique 'key' for a subscription is [listener, eventName, filter]
    meaning a subscriber can subscribe to the same event with different filters
 */
typedef struct _rbusSubscription
{
    char* listener;             /* the subscriber's address to publish to*/
    char* eventName;            /* the event name subscribed to e.g. Device.WiFi.AccessPoint.1.AssociatedDevice.*.SignalStrength */
    rbusFilter_t filter;        /* optional filter */
    int32_t interval;           /* optional interval */
    int32_t duration;           /* optional duration */
    bool autoPublish;           /* auto publishing */
    TokenChain* tokens;         /* tokenized eventName for pattern matching */
    elementNode* element;       /* the registation element e.g. Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength */
    rtList instances;           /* the instance elements e.g.   Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength
                                                                Device.WiFi.AccessPoint.1.AssociatedDevice.2.SignalStrength
                                                                Device.WiFi.AccessPoint.2.AssociatedDevice.1.SignalStrength */
} rbusSubscription_t;

/*create a new subscriptions registry for an rbus handle*/
void rbusSubscriptions_create(rbusSubscriptions_t* subscriptions, rbusHandle_t handle, elementNode* root);

/*destroy a subscriptions registry*/
void rbusSubscriptions_destroy(rbusSubscriptions_t subscriptions);

/*add a new subscription with unique key [listener, eventName, filter] and the corresponding*/
rbusSubscription_t* rbusSubscriptions_addSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, rbusFilter_t filter, int32_t interval, int32_t duration, bool autoPublish, elementNode* registryElem);

/*get an existing subscription by searching for its unique key [listener, eventName, filter]*/
rbusSubscription_t* rbusSubscriptions_getSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, rbusFilter_t filter);

/*remove an existing subscription*/
void rbusSubscriptions_removeSubscription(rbusSubscriptions_t subscriptions, rbusSubscription_t* sub);

/*call right after a new row is added*/
void rbusSubscriptions_onTableRowAdded(rbusSubscriptions_t subscriptions, elementNode* node);

/*call right before an existing row is delete*/
void rbusSubscriptions_onTableRowRemoved(rbusSubscriptions_t subscriptions, elementNode* node);

#ifdef __cplusplus
}
#endif
#endif
