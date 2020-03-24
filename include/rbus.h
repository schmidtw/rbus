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
#ifndef __RBUS_H
#define __RBUS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup rbus RBus
 *  @brief rbus is a common, generic and efficient message bus used across
 *  RDK-V, B and C profiles.
 */

/** @defgroup RBUSCommon Common definitions and Error codes
 *  @ingroup rbus
 */

/** @defgroup BusInit Initialization
 *  @ingroup rbus
 */

/** @defgroup ElementOp Elements related operations (Registration, discovery and get / set)
 *  @ingroup rbus
 */

/** @defgroup EventOp Eventing related operations
 *  @ingroup rbus
 */

/** @defgroup TableOp Table Objects related operations
 *  @ingroup rbus
 */

/** @defgroup GetResp Get Response related operations
 *  @ingroup rbus
 */


//******************** Defs, Enums, Data Structure & Callbacks *****************//
/** @ingroup RBUSCommon
 *  @{
 */

//************ Definition for Boolean Type Values ************//
#define RBUS_TRUE  1
#define RBUS_FALSE 0

///  @brief rbusBool_t Boolean type definition for RBus
typedef int rbusBool_t;

///  @brief rbusHandle_t Identifier for each opened component
struct _rbusHandle_t;
typedef struct _rbusHandle_t* rbusHandle_t;

///  @brief rbus_errorCode_e indicates all possible error codes of RBus.
typedef enum
{
    //Generic error codes
    RBUS_ERROR_SUCCESS                  = 0,    /**< SUCCESS                  */
    RBUS_ERROR_BUS_ERROR                = 1,    /**< Bus Error                */
    RBUS_ERROR_INVALID_INPUT,                   /**< Invalid Input            */
    RBUS_ERROR_NOT_INITIALIZED,                 /**< Bus not initialized      */
    RBUS_ERROR_OUT_OF_RESOURCES,                /**< Running out of resources */
    RBUS_ERROR_DESTINATION_NOT_FOUND,           /**< Dest element not found   */
    RBUS_ERROR_DESTINATION_NOT_REACHABLE,       /**< Dest element not reachable*/
    RBUS_ERROR_DESTINATION_RESPONSE_FAILURE,    /**< Dest failed to respond   */
    RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION,/**< Invalid dest response   */
    RBUS_ERROR_INVALID_OPERATION,               /**< Invalid Operation        */
    RBUS_ERROR_INVALID_EVENT,                   /**< Invalid Event            */
    RBUS_ERROR_INVALID_HANDLE,                  /**< Invalid Handle           */

    //Initialization error codes
    RBUS_ERROR_COMPONENT_NAME_DUPLICATE = 1001, /**< Comp name already exists */

    //Element Registration error codes
    RBUS_ERROR_ELEMENT_NAME_DUPLICATE   = 2001, /**< One or more element name(s)
                                                   were previously registered */

    RBUS_ERROR_ELEMENT_NAME_MISSING,            /**< No names were provided in
                                                   the name field             */

    //Discovery operations error codes
    RBUS_ERROR_COMPONENT_DOES_NOT_EXIST = 3001, /**< A bus connection for this
                                                   component name was not
                                                   previously opened.         */

    RBUS_ERROR_ELEMENT_DOES_NOT_EXIST,          /**< One or more data element
                                                   name(s) do not currently have
                                                   a valid registration       */

    //Access Control error codes
    RBUS_ERROR_ACCESS_NOT_ALLOWED       = 4001, /**< Access to the requested data
                                                   element was not permitted by
                                                   the provider component.    */

    //Get Response Builder error codes
    RBUS_ERROR_INVALID_CONTEXT          = 5001  /**< The Context is not same as
                                                    what was sent in the get
                                                    callback handler.         */
} rbus_errorCode_e;

 /// @brief rbus_elementType_e indicates the type of parameter possible in RBus
typedef enum
{
    RBUS_BOOLEAN  = 0x500,  /**< Boolean ("true" / "false" or "0" / "1")                */
    RBUS_INT16,             /**< Short (ex: 32767 or -32768)                            */
    RBUS_UINT16,            /**< Unsigned Short (ex: 65535)                             */
    RBUS_INT32,             /**< Integer (ex: 2147483647 or -2147483648)                */
    RBUS_UINT32,            /**< Unsigned Integer (ex: 4,294,967,295)                   */
    RBUS_INT64,             /**< Long (ex: 9223372036854775807 or -9223372036854775808) */
    RBUS_UINT64,            /**< Unsigned Long (ex: 18446744073709551615)               */
    RBUS_STRING,            /**< Null terminated string                                 */
    RBUS_DATE_TIME,         /**< ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ)                 */
    RBUS_BASE64,            /**< Base64 representation of the binary data               */
    RBUS_BINARY,            /**< Binary Data, application specific                      */
    RBUS_FLOAT,             /**< Float (ex: 1.2E-38 or 3.4E+38)                         */
    RBUS_DOUBLE,            /**< Double (ex: 2.3E-308 or 1.7E+308)                      */
    RBUS_BYTE,              /**< A byte (ex: 00 or FF)                                  */
    RBUS_REFERENCE,         /**< Reference buffer. Use case specific                    */
    RBUS_EVENT_DEST_NAME,   /**< the destination name of an event receiver              */
    RBUS_NONE
} rbus_elementType_e;

/// @brief rbus_Tlv_t Name, Type, Length and Value for an element.
typedef struct
{
    char*               name;   /**< name of the element                      */
    rbus_elementType_e  type;   /**< Type of element                          */
    unsigned int        length; /**< Length of the element value in octets    */
    void*               value;  /**< Value of the element                     */
} rbus_Tlv_t;
/** @} */

/** @addtogroup EventOp
 *  @{
 */
/// @brief rbus_eventSubAction_e Actions that can be performed for an event
typedef enum
{
    RBUS_EVENT_ACTION_SUBSCRIBE = 0,
    RBUS_EVENT_ACTION_UNSUBSCRIBE
} rbus_eventSubAction_e;

/// @brief rbus_eventType_e indicates the type of event.
typedef enum
{
    //Keep these values suitable for bitmasks (1, 2, 4, 8..)
    RBUS_EVENT_TYPE_ON_CHANGE             = 1,
    RBUS_EVENT_TYPE_ON_INTERVAL           = 2,
    RBUS_EVENT_TYPE_IMMEDIATE             = 4
} rbus_eventType_e;

/// @brief rbus_ThresholdType_e The threshold type for "OnChange" event type
typedef enum
{
    //Keep these values suitable for bitmasks (1, 2, 4, 8..)
    RBUS_THRESHOLD_ON_CHANGE_ANY               = 1,
    RBUS_THRESHOLD_ON_CHANGE_GREATER_THAN      = 2,
    RBUS_THRESHOLD_ON_CHANGE_LESS_THAN         = 4,
    RBUS_THRESHOLD_ON_CHANGE_EQUALS            = 8
} rbus_ThresholdType_e;

/// @brief rbus_Threshold_t thresholds that can be used during subscription
typedef struct
{
    rbus_ThresholdType_e    type;       /**< type of threshold                */
    void*                   value;      /**< value to be checked against
                                             (Value is event specific. The type
                                             of value will be same as that
                                             of the event parameter for which
                                             the event is raised. So, it is
                                             void* here)                      */
} rbus_Threshold_t;

/// @brief rbusEventFilter_t Filter that can be used for event subscription
typedef struct
{
    rbus_eventType_e        eventType;  /**< Event type.
                                             If the type is "EVENT_ON_INTERVAL",
                                                                use "interval"
                                             If the type is "EVENT_ON_CHANGE",
                                                                use "threshold"
                                        */
    union
    {
        int                 interval;   /**< Total interval period after which
                                             the event needs to be fired. Should
                                             be in multiples of minInterval
                                        */
        rbus_Threshold_t    threshold;  /**< Threshold to be checked against
                                             before firing an event when there
                                             is a "change" in value
                                        */
    } filter;                           /**< Event filter. Would either be an
                                             interval based filter or threshold
                                             based filter
                                        */
} rbusEventFilter_t;

/// @brief rbus_filterCap_t Filter capabilities for an event.
typedef struct
{
    int                     type;       /**< All supported types for a given
                                             event. "Bitwise OR"ed value of
                                             each supported types in
                                             rbus_eventType_e
                                        */
    int                     minInterval;/**< The minInterval(in ms)supported for
                                             the event. Event can be subscribed
                                             for multiples of minInterval.
                                             Applicable only when EVENT_ON_INTERVAL
                                             type is supported.
                                        */
    int                     threshold;  /**< All supported threshold for a given
                                             event. "Bitwise OR"ed value of each
                                             supported types in
                                             rbus_ThresholdType_e. Applicable
                                             only when EVENT_ON_CHANGE type
                                             is supported.
                                        */
} rbus_filterCap_t;

/// @brief rbus_eventInfo_t Event Info (name & filter capabilities of event)
typedef struct
{
    char*                   name;       /**< Event Name */
    rbus_filterCap_t        filterCap;    /**< Filter Capabilities available
                                             for the event */
} rbus_eventInfo_t;

/// @brief rbusEventSubscription_t
typedef struct rbusEventSubscription_t
{
    char const*         event_name; /** Full qualified event name */
    rbusEventFilter_t*  filter;     /** Optional filter that the consumer would like 
                                        the sender to apply before sending the event
                                      */
    uint32_t            duration;   /** Optional maximum duration in seconds until which 
                                        the subscription should be in effect. Beyond this 
                                        duration, the event would be unsubscribed automatically. 
                                        Pass "0" for indefinite event subscription which requires 
                                        the rbusEvent_Unsubscribe API to be called explicitly.
                                      */
    void*               handler     /** fixme rbusEventHandler_t internal*/;
    void*               user_data;  /** The user_data set when subscribing to the event. */
    rbusHandle_t        handle;     /** The rbus handle associated with this subscription */
} rbusEventSubscription_t;

/** \fn typedef void (* rbusEventHandler_t)(rbusHandle_t              handle,
 *                                          rbusEventSubscription_t*  subscription,
 *                                          rbus_Tlv_t const*         eventData)
 *  \brief A component will receive this API callback when an event is received.
 *      This callback is registered with rbusEvent_Subscribe. \n
 *  Used by: Any component that subscribes for events.
 *  \param rbusHandle Bus Handle
 *  \param subscription Subscription data created when the client subscribed
        for the event.  This will contain the user_data, for example.
 *  \param eventData Event data sent from the publishing component
 *  \return void
 */
typedef void (*rbusEventHandler_t)(
      rbusHandle_t              handle,
      rbusEventSubscription_t*  subscription,
      rbus_Tlv_t const*         eventData);
/** @} */


/** @addtogroup TableOp
 *  @{
 */
/// @brief rbus_tblAction_e Actions that can be performed on a table object
typedef enum
{
    RBUS_TABLE_ACTION_ADD_INSTANCE = 0,
    RBUS_TABLE_ACTION_REMOVE_INSTANCE
} rbus_tblAction_e;
/** @} */


/** @addtogroup ElementOp
 *  @{
 */
 /** \fn typedef rbus_errorCode_e (*RBus_GetHandler)(int numTlvs,
                               rbus_Tlv_t* tlv, char *requestingComponentName)
 *  \brief One of the property handlers. The provider component would have to
 *  implement this callback handler to handle any "get parameter values"
 *  operations performed by the consumers.  \n
 *  Used by: Any component that provides one or more variables that can be read
 *  by another component.
 *  \param context An opaque handle that needs to be passed back to get response
            handler APIs.
 *  \param numTlvs the number (count) of parameters
 *  \param tlv the parameters
 *  \param requestingComponentName the name of the component making the request
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_INPUT
 */
typedef rbus_errorCode_e (*RBus_GetHandler)(void *context, int numTlvs,
                              rbus_Tlv_t* tlv, char *requestingComponentName);


/** \fn typedef rbus_errorCode_e (*RBus_SetHandler)(int numTlvs,
                                rbus_Tlv_t* tlv, int sessionId, rbusBool_t commit,
                                char *requestingComponentName)
 *  \brief One of the property handlers. The provider component would have to
 *  implement this callback handler to handle any "set parameter values"
 *  operations performed by the consumers. \n
 *  Used by: Any component that provides one or more variables that can be "set"
 *  by another component.
 *  \param numTlvs the number (count) of parameters
 *  \param tlv the parameters
 *  \param sessionId One or more parameters can be set together over a session.
 *  The session ID binds all set operations. A value of sessionID 0 indicates
 *  this is not a session based operation. A non-zero value indicates that the
 *  value should be "remembered" temporarily. Only when the "commit" parameter
 *  is "true", then all remembered parameters on this session should be set
 *  together.
 *  \param commit When TRUE, this indicates this is the last set operation in
 *  the session and all pending params can be set / committed together in this
 *  operation.  When FALSE, the parameters must not yet be committed.
 *  \param requestingComponentName the name of the component making the request
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:RBUS_ERROR_INVALID_INPUT
 */
typedef rbus_errorCode_e (*RBus_SetHandler)(int numTlvs,
                                rbus_Tlv_t* tlv, int sessionId, rbusBool_t commit,
                                char *requestingComponentName);


/** \fn typedef rbus_errorCode_e (* RBus_UpdateTableHandler)(char *tableName,
                                char* aliasName, rbus_tblAction_e action,
                                char *requestingComponentName)
 *  \brief One of the property handlers. The provider component may implement
 *  this callback handler to enable another component to add or delete a table
 *  row. This callback is only for "dynamic" tables.  It will not be provided
 *  for "static" tables that do not support adding or deleting table rows.
 *  The requesting component may specify a unique name for a row to be added
 *  or deleted in *aliasName.  If used for row additions this name must not
 *  previously exist in this table. When used for row deletions, the name must
 *  previously exist for a row.  If NULL, this is not specified. \n
 *  Used by: Provider
 *  \param tableName The name of this table.
 *  \param aliasName the name to be used for a table row to be added or the name
 *          of the table row to be deleted.  NULL if not specified.
 *  \param action Indicates if the action is to add or delete a row in this table.
 *  \param requestingComponentName The name of the component making the request
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:RBUS_ERROR_INVALID_INPUT
 */
typedef rbus_errorCode_e (* RBus_UpdateTableHandler)(char *tableName,
                                char* aliasName, rbus_tblAction_e action,
                                char *requestingComponentName);


/** \fn typedef rbus_errorCode_e (* RBus_EventSubHandler)(rbus_eventSubAction action,
                                char* eventName, rbus_eventFilter_t filter)
 *  \brief A component will receive this API callback when the first component
 *  subscribes to an event/filter pair (event is needed) or the last componet
 *  unsubscribes to an event/filter pair or all component subscriptions have
 *  timed out due to max duration (event is not needed). Distribution of the
 *  event to multiple subscribers will be transparently handled by the library \n
 *  Used by: Any component that allows other component(s) to subscribe
 *  and unsubscribe for events.
 *  \param action Subscribe or unsubscribe flag
 *  \param action Indicates if this is for subscribe or unsubscribe action
 *  \param eventName This is the fully qualified name of the event
 *  \param filter The filter that the subscribing component would like the
 *  provider component to apply before deciding to send the event.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_EVENT
 */
typedef rbus_errorCode_e (* RBus_EventSubHandler) (rbus_eventSubAction_e action,
                                const char* eventName, rbusEventFilter_t* filter);


///  @brief rbus_callbackTable_t List of callbacks for a data element name that
///  is registered or  unregistered. This table also specifies the possible
///  usage for each data element. If a particular usage (for ex., get, set,
///  eventSubscription, etc) is supported, then that callback must be set to a
///  function pointer for the handler. If a particular usage is not supported by
///  the component, the callback shall be set "NULL". On call to registger the
///  data element, the rbus library checks for NULL and substitutes a pointer
///  to an error handler function for all unused features
typedef struct
{
    RBus_GetHandler         rbus_getHandler;                /**< Get parameters
                                                            handler for the
                                                            named paramter   */
    RBus_SetHandler         rbus_setHandler;                /**< Set parameters
                                                            handler for the
                                                            named parameter  */
    RBus_UpdateTableHandler rbus_updateTableHandler;        /**< Add and delete
                                                            table row handler
                                                            for named table  */
    RBus_EventSubHandler    rbus_eventSubHandler;           /**< Event subscribe
                                                            and unsubscribe
                                                            handler for the
                                                            event name       */
}  rbus_callbackTable_t;

///  @brief rbus_dataElement_t The structure used when registering or
///         unregistering a data element to the bus.
typedef struct
{
    char*      		        elementName;/**< Name and type of an element      */
    rbus_callbackTable_t    table;      /**< Element Handler table. A specific
                                             callback can be NULL, if no usage*/
}rbus_dataElement_t;
/** @} */


//*********************************** Functions ******************************//

//******************************* Bus Initialization *************************//
/** @ingroup BusInit
 *  @{
 */
/** \fn rbus_errorCode_e rbus_open (rbusHandle_t* rbusHandle, char *componentName)
 *  \brief  Open a bus connection for a software component.
 *  If multiple components share a software process, the first component that
 *  calls this API will establishes a new socket connection to the bus broker.
 *  All calls to this API will receive a dedicated bus handle for that component.
 *  If a component calls this API more than once, any previous busHandle and all
 *  previous data element registrations will be canceled.                     \n
 *  Note: This API supports a single component per software process and also
 *  supports multiple components that share a software process. In the case of
 *  multiple components that share a software process, each component must call
 *  this API to open a bus connection. Only a single socket connection is opened
 *  per software process but each component within that process receives a
 *  separate busHandle.                                                       \n
 *  Used by:  All RBus components to begin a connection with the bus.
 *  \param rbusHandle Bus Handle
 *  \param componentName the name of the component initializing onto the bus
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_BUS_ERROR: Indicates there is some bus error. Try later.
 */
rbus_errorCode_e rbus_open (rbusHandle_t* rbusHandle, char *componentName);

/** \fn rbus_errorCode_e rbus_close (rbusHandle_t rbusHandle)
 *  \brief  Removes a logical bus connection from a component to the bus.     \n
 *  Note: In the case of multiple components that share a software process, the
 *  socket connection remains up until the last component in that software process
 *  closes its bus connection.                                                \n
 *  Used by:  All RBus components (multiple components may share a software process)
 *  \param rbusHandle Bus Handle
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_BUS_ERROR: Indicates there is some bus error. Try later.
 */
rbus_errorCode_e rbus_close (rbusHandle_t rbusHandle);
/** @} */


/** @addtogroup ElementOp
 *  @{
 */

//************************* Data Element Registration **************************//
/** \fn rbus_errorCode_e rbus_regDataElements (rbusHandle_t rbusHandle,
                            int numDataElements, rbus_dataElement_t *elements)
 *  \brief  A Component uses this API to register one or more named Data
 *  Elements (i.e., parameters and/or event names) that will be accessible /
 *  subscribable by other components. This also registers the callback functions
 *  associated with each data element using the dataElement structure.        \n
 *  Used by:  All components that provide named parameters and/or events that
 *  may be accessed/subscribed by other component(s)
 *  \param rbusHandle Bus Handle
 *  \param numDataElements the number (count) of data elements to be registered
 *  \param elements the list of elements to be registered
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ELEMENT_NAME_DUPLICATE: Data Element name already exists
 */
rbus_errorCode_e rbus_regDataElements (rbusHandle_t rbusHandle,
                            int numDataElements, rbus_dataElement_t *elements);

/** \fn rbus_errorCode_e rbus_unregDataElements(rbusHandle_t rbusHandle,
                            int numDataElements, rbus_dataElement_t *elements)
 *  \brief  A Component uses this API to unregister one or more previously
 *  registered Data Elements (i.e., named parameters and/or event names) that
 *  will no longer be accessible / subscribable by other components.          \n
 *  Used by:  All components that provide named parameters and/or events that
 *  may be accessed/subscribed by other component(s)
 *  \param rbusHandle Bus Handle
 *  \param numDataElements the number (count) of data elements to be unregistered
 *  \param elements the list of elements to be unregistered
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ELEMENT_NAME_MISSING: No data element names provided.
 */
rbus_errorCode_e rbus_unregDataElements (rbusHandle_t rbusHandle,
                            int numDataElements, rbus_dataElement_t *elements);


//************************* Discovery related Operations *******************//
/** \fn rbus_errorCode_e rbus_discoverComponentName (rbusHandle_t rbusHandle,
                            int numElements, char** elementNames,
                            int *numComponents, char **componentName)
 *  \brief This allows a component to get a list of components that provide
 *  a set of data elements name(s).  The requesting component provides the
 *  number of data elements and a list of data  elements.  The bus infrastructure
 *  provides the number of components and the component names.                \n
 *  Used by: Client
 *  \param rbusHandle Bus Handle
 *  \param numElements The number (count) of data elements
 *  \param elementNames The list of element for which components are to be found
 *  \param numComponents Total number of components
 *  \param componentName Name of the components discovered
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ELEMENT_DOES_NOT_EXIST: Data Element was not previously registered.
 */
rbus_errorCode_e rbus_discoverComponentName (rbusHandle_t rbusHandle,
                            int numElements, char** elementNames,
                            int *numComponents, char ***componentName);


/** \fn rbus_errorCode_e rbus_discoverComponentDataElements (rbusHandle_t rbusHandle,
                            char* name, rbusBool_t nextLevel, int *numElements,
                            char** elementNames)
 *  \brief   This enables a component to get a list of all data elements
 *  provided by a component. The  requesting component provides component name
 *  or all data elements under a "partial path" of an element that ends with
 *  "dot". The bus infrastructure provides the number of data elements and the
 *  data element names array.                                                \n
 *  Used by: Client
 *  \param rbusHandle Bus Handle
 *  \param name Name of the component or partial path of an element name.
 *  \param nextLevel Indicates whether to retrieve only the immediate elements
           (true) or retrieve all elements under the partial path (false).
 *  \param numElements Number (count) of data elements that were discovered
 *  \param elementNames List of elements discovered
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_COMPONENT_DOES_NOT_EXIST: Component name was not previously registered.
 */
rbus_errorCode_e rbus_discoverComponentDataElements (rbusHandle_t rbusHandle,
                            char* name, rbusBool_t nextLevel, int *numElements,
                            char*** elementNames);


//************************* Parameters related Operations *******************//
/** \fn rbus_errorCode_e rbus_get(rbusHandle_t rbusHandle, char* destCompName, rbus_Tlv_t *tlv)
 *  \brief A component uses this to perform a generic get operation on a
 *  single variable length parameter \n
 *  Used by: All components that need to get an individual parameter
 *  \param rbusHandle Bus Handle
 *  \param tlv The individual parameter. Includes name, type, length, value of
 *  parameter queried. "name" is filled by caller, rest of the values filled
 *  and returned by the implementation.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_get(rbusHandle_t rbusHandle, rbus_Tlv_t *tlv);


/** \fn rbus_errorCode_e rbus_getExt(rbusHandle_t rbusHandle, int paramCount,
                       char** paramNames, int *numTlvs, rbus_Tlv_t*** tlv);
 *  \brief Gets one or more parameter value(s) (TLVs) in a single bus operation.
 *  This API also supports wild cards or table entries that provide multiple
 *  param values for a single param input. These options are explained below: \n
 *  Option 1: Single param query:                                             \n
 *      paramCount      : 1                                                   \n
 *      paramNames      : parameter name (single element)                     \n
 *      numTlvs         : 1                                                   \n
 *      tlv             : parameter value (caller to free memory)             \n
 *  Option 2: Multi params query:                                             \n
 *      paramCount      : n                                                   \n
 *      paramNames      : Array of parameter names (multi element array)      \n
 *      numTlvs         : n                                                   \n
 *      tlv             : Array of parameter values (caller to free memory)   \n
 *  Option 3: Partial path query:                                             \n
 *      paramCount      : 1                                                   \n
 *      paramNames      : Param name path that ends with "." (ex: Device.IP.) \n
 *      numTlvs         : n                                                   \n
 *      tlv             : Array of parameter values (caller to free memory)   \n
 *  Option 4: Instance Wild card query:                                       \n
 *      paramCount      : 1                                                   \n
 *      paramNames      : Instance Wild card query is possible for tables.
                          For ex: Device.IP.Interface.*.Status
                          Multi indices wild card query is also supported
                          For ex: Device.IP.Interface.*.IP4Address.*.IPAddress
                          Wildcards can be combined with partial path names
                          For ex: Device.IP.Interface.*. IP4Address.
                          Note: In all cases, the exact query would be routed to
                          provider component and it is the responsibility of
                          the provider component to respond with appropriate
                          response.                                           \n
 *      numTlvs         : n                                                   \n
 *      tlv             : Array of parameter values (caller to free memory)   \n
 *  Used by: All components that need to get one or more parameters
 *  \param rbusHandle Bus Handle
 *  \param paramCount The number (count) of input elements (parameters)
 *  \param paramNames Input elements (parameters)
 *  \param numTlvs The number (count) of output parameter values
 *  \param tlv The output parameter values
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_getExt(rbusHandle_t rbusHandle, int paramCount,
                       char** paramNames, int *numTlvs, rbus_Tlv_t*** tlv);


/** \fn rbus_errorCode_e rbus_getInt(rbusHandle_t rbusHandle, char* paramName, int* paramVal)
 *  \brief A component uses this to perform an integer get operation.  \n
 *  Used by: All components that need to get an integer parameter
 *  \param rbusHandle busHandle
 *  \param paramName the name of the integer parameter
 *  \param paramVal the value of the integer parameter
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to the requested parameter is not permitted
 */
rbus_errorCode_e rbus_getInt(rbusHandle_t rbusHandle, char* paramName, int* paramVal);


/** \fn rbus_errorCode_e rbus_getUint (rbusHandle_t rbusHandle, char* paramName,
                                                        unsigned int* paramVal)
 *  \brief A component uses this to perform a get operation on an
 *  unsigned int parameter. \n
 *  Used by: All components that need to get an unsigned integer parameter
 *  \param rbusHandle Bus Handle
 *  \param paramName the name of the unsigned integer parameter
 *  \param paramVal the value of the unsigned integer parameter
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_getUint (rbusHandle_t rbusHandle, char* paramName,
                                                        unsigned int* paramVal);


/** \fn rbus_errorCode_e rbus_getStr (rbusHandle_t rbusHandle, char* paramName,
                                                                  char* paramVal)
 *  \brief A component uses this to perform a get operation on a
 *  string parameter. \n
 *  Used by: All components that need to get a string parameter
 *  \param rbusHandle Bus Handle
 *  \param paramName the name of the string parameter
 *  \param paramVal the value of the string parameter
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_getStr (rbusHandle_t rbusHandle, char* paramName, char* paramVal);


/** \fn rbus_errorCode_e rbus_set(rbusHandle_t rbusHandle, rbus_Tlv_t * tlv,
                                                    int sessionId, rbusBool_t commit)
 *  \brief A component uses this to perform a set operation for a single
 *  explicit parameter and has the option to used delayed (coordinated)
 *  commit commands. \n
 *  Used by: All components that need to set an individual parameter
 *  \param rbusHandle Bus Handle
 *  \param tlv the individual parameter
 *  \param sessionId   One or more parameter can be set together over a session.
 *  The session ID binds all set operations. A value of sessionID 0 indicates
 *  this is not a session based operation. A non-zero value indicates that the
 *  value should be "remembered" temporarily. Only when the "commit" parameter
 *  is "true", then all remembered parameters on this session should be set
 *  together.
 *  \param commit Indicates this is the last set operation in the session and
 *  all pending params can be set / committed together now.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted
 */
rbus_errorCode_e rbus_set(rbusHandle_t rbusHandle, rbus_Tlv_t* tlv,
                                                    int sessionId, rbusBool_t commit);


/** \fn rbus_errorCode_e rbus_setMulti(rbusHandle_t rbusHandle, int numTlvs,
                                rbus_Tlv_t * tlv, int sessionId, rbusBool_t commit)
 *  \brief A component uses this to perform a set operation for multiple
 *  parameters at once.  \n
 *  Used by: All components that need to set multiple parameters
 *  \param rbusHandle Bus Handle
 *  \param numTlvs the number (count) of parameters
 *  \param tlv the parameters
 *  \param sessionId One or more parameters can be set together over a session.
 *  The session ID binds all set operations. A value of sessionID 0 indicates
 *  this is not a session based operation. A non-zero value indicates that the
 *  value should be "remembered" temporarily. Only when the "commit" parameter
 *  is "true", then all remembered parameters on this session should be set
 *  together.
 *  \param commit Indicates this is the last set operation in the session and
 *  all pending params can be set / committed together now.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_setMulti(rbusHandle_t rbusHandle, int numTlvs,
                                rbus_Tlv_t *tlv, int sessionId, rbusBool_t commit);


/** \fn rbus_errorCode_e rbus_setInt (rbusHandle_t rbusHandle, char* paramName,
                                                                   int paramVal)
 *  \brief A component uses this to perform a simple set operation on an integer
 *  and commit the operation. \n
 *  Used by: All components that need to set an integer and commit the change.
 *  \param rbusHandle Bus Handle
 *  \param paramName the name of the integer parameter
 *  \param paramVal the value of the integer parameter
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted
 */
rbus_errorCode_e rbus_setInt (rbusHandle_t rbusHandle, char* paramName, int paramVal);


/** \fn rbus_errorCode_e rbus_setUint (rbusHandle_t rbusHandle, char* paramName,
                                                        unsigned int paramVal)
 *  \brief A component uses this to perform a simple set operation on an
 *  unsigned integer and commit the operation.  \n
 *  Used by: All components that need to set an unsigned integer and commit
 *  the change.
 *  \param rbusHandle Bus Handle
 *  \param paramName the name of the unsigned integer parameter
 *  \param paramVal the value of the unsigned integer parameter
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_setUint (rbusHandle_t rbusHandle, char* paramName,
                                                        unsigned int paramVal);


/** \fn rbus_errorCode_e rbus_setStr (rbusHandle_t rbusHandle, char* paramName,
                                                                 char* paramVal)
 *  \brief A component uses this to perform a set operation on a
 *  string parameter. \n
 *  Used by: All components that need to set a string parameter
 *  \param rbusHandle Bus Handle
 *  \param paramName the name of the string parameter
 *  \param paramVal the value of the string parameter
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are:
 *  RBUS_ERROR_ACCESS_NOT_ALLOWED: Access to requested parameter is not permitted.
 */
rbus_errorCode_e rbus_setStr (rbusHandle_t rbusHandle, char* paramName, char* paramVal);
/** @} */


//********************** Response Building for Get Handler *****************//
/** @addtogroup GetResp
 *  @{
 */
/** \fn rbus_errorCode_e  rbus_initGetResp(void *context, int paramCount)
 *  \brief Component's get handlers use this API to initialize "get" response.
 *  Provider components use a set of "response building" APIs to initialize,
 *  and build responses to "get" queries. This API shall be called to
 *  initialize the get response by passing the number of parameters involved in
 *  the get response. Calling this API is optional, if the number of parameters
 *  involved in the get response is 1.  \n
 *  Used by: Provider components that has registered get handlers.
 *  \param context The opaque handle received in the get callback handler.
 *  \param paramCount Number of parameters in the response.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_CONTEXT
 */
rbus_errorCode_e  rbus_initGetResp(void *context, int paramCount);

/** \fn rbus_errorCode_e  rbus_buildGetRespFromTlv(void *context, rbus_Tlv_t tlv)
 *  \brief Component's get handlers use this API to "build" the "get" response
 *  by passing a tlv value. This API shall be called (potentially in a loop)
 *  to build the get response (depending on number of parameters involved in
 *  the get response). \n
 *  Used by: Provider components that has registered get handlers.
 *  \param context The opaque handle received in the get callback handler.
 *  \param tlv Response in the form of tlv.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_CONTEXT
 */
rbus_errorCode_e  rbus_buildGetRespFromTlv(void *context, rbus_Tlv_t tlv);

/** \fn rbus_errorCode_e  rbus_buildGetRespFromEntry(void *context,
                                            char *paramName,
                                            rbus_elementType_e type,
                                            void *value,
                                            int length)
 *  \brief Component's get handlers use this API to "build" the "get" response
 *  by passing a "value entry". This API shall be called (potentially in a loop)
 *  to build the get response (depending on number of parameters involved in
 *  the get response). \n
 *  Used by: Provider components that has registered get handlers.
 *  \param context The opaque handle received in the get callback handler.
 *  \param paramName parameter name in the response.
 *  \param type Type of parameter.
 *  \param value Value of parameter.
 *  \param length Length of parameter value.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_CONTEXT
 */
rbus_errorCode_e  rbus_buildGetRespFromEntry(void *context,
                                            char *paramName,
                                            rbus_elementType_e type,
                                            void *value,
                                            int length);

/** \fn rbus_errorCode_e  rbus_buildGetRespFromTlvList(void *context,
                                            rbus_Tlv_t *tlvList, int length)
 *  \brief Component's get handlers use this API to "build" the "get" response
 *  by passing a list (array) of tlv's. This API shall be called
 *  to build the get response in the form of list (array) of tlv's. \n
 *  Used by: Provider components that has registered get handlers.
 *  \param context The opaque handle received in the get callback handler.
 *  \param tlvList Array of tlv values.
 *  \param length Length of TLV list (array).
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_CONTEXT
 */
rbus_errorCode_e  rbus_buildGetRespFromTlvList(void *context,
                                            rbus_Tlv_t *tlvList, int length);

/** @} */

//************************** Event Notification ****************************//
/** @addtogroup EventOp
 *  @{
 */

/** \fn rbus_errorCode_e  rbusEvent_Subscribe ( rbusHandle_t        rbusHandle,
 *                                              char const*         eventName,
 *                                              rbusEventHandler_t  handler,
 *                                              void*               userData)
 *  \brief Components use this API to subscribe for an event  \n
 *  Used by: Components that needs to subscribe for an event.
 *  \param rbusHandle Bus Handle
 *  \param eventName fully qualified name of the event.
 *  \param handler event callback handler
 *  \param userData user data to be passed back to the callback handler
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_EVENT
 */
rbus_errorCode_e  rbusEvent_Subscribe(
    rbusHandle_t        rbusHandle,
    char const*         eventName,
    rbusEventHandler_t  handler,
    void*               userData);

/** \fn rbus_errorCode_e  rbusEvent_Unsubscribe ( rbusHandle_t        rbusHandle,
 *                                                char const*         eventName)
 *  \brief Components use this API to unsubscribe for an event  \n
 *  Used by: Components that needs to unsubscribe for an event.
 *  \param rbusHandle Bus Handle
 *  \param eventName fully qualified name of the event.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_EVENT
 */
rbus_errorCode_e rbusEvent_Unsubscribe(
    rbusHandle_t        rbusHandle,
    char const*         eventName);

/** \fn rbus_errorCode_e  rbusEvent_SubscribeEx ( rbusHandle_t              rbusHandle,
 *                                                rbusEventSubscription_t*  subscription,
 *                                                int                       numSubscriptions)
 *  \brief Components use this API to subscribe for 1 or more events
 *      with the option to add extra attributes to the subscription 
 *      through the rbusEventSubscription_t structure\n
 *  Used by: Components that needs to subscribe for events.
 *  \param rbusHandle Bus Handle
 *  \param subscription pointer to an array of subscriptions
 *  \param numSubscriptions number of array elements in subscription param
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_EVENT
 */
rbus_errorCode_e rbusEvent_SubscribeEx(
    rbusHandle_t              rbusHandle,
    rbusEventSubscription_t*  subscription,
    int                       numSubscriptions);

/** \fn rbus_errorCode_e  rbusEvent_UnsubscribeEx ( rbusHandle_t              rbusHandle,
 *                                                rbusEventSubscription_t*  subscription,
 *                                                int                       numSubscriptions)
 *  \brief Components use this API to unsubscribe for 1 or more events
 *      with the option to add extra attributes to the unsubscription 
 *      through the rbusEventSubscription_t structure\n
 *  Used by: Components that needs to unsubscribe for events.
 *  \param rbusHandle Bus Handle
 *  \param subscription pointer to an array of subscriptions
 *  \param numSubscriptions number of array elements in subscription param
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_EVENT
 */
rbus_errorCode_e rbusEvent_UnsubscribeEx(
    rbusHandle_t              rbusHandle,
    rbusEventSubscription_t*  subscription,
    int                       numSubscriptions);

/** \fn rbus_errorCode_e  rbusEvent_Publish ( rbusHandle_t      rbusHandle,
 *                                            rbus_Tlv_t*       event)
 *  \brief A component uses this API to publish an event.
 *  This library keeps state of all event subscriptions and duplicates event
 *  messages as needed for distribution. \n
 *  Used by: Components that provide events
 *  \param rbusHandle Bus Handle
 *  \param event The event data. The rbus_Tlv_t name member must be set
 *      to the fully qualified event name.
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_EVENT
 */
rbus_errorCode_e  rbusEvent_Publish(
    rbusHandle_t    rbusHandle,
    rbus_Tlv_t*     event);

/** @} */


//************************** Table Objects - APIs***************************//
/** @addtogroup TableOp
 *  @{
 */
/** \fn rbus_errorCode_e rbus_updateTable (busHandle rbusHandle, char *tableName,
                                    rbus_tblAction_e action, char* aliasName)
 *  \brief Update a table (i.e., add or remove an entry to / from the table).
 *  This API only updates the table (i.e., the "updates" include adding or
 *  removing a row to/from the table).
 *  Newly added rows themselves must be updated separately using set operations
 *  aliasName is used to specify a name for the table row.                    \n
 *  Used by:  Any component that needs to add or delete table rows in
 *  another component.
 *  \param rbusHandle Bus Handle
 *  \param tableName The name of a table (ex: Device.IP.Interface.)
 *  \param action Add or Remove an entry.
 *  \param aliasName The name of a row in a table
 *  \return RBus error code as defined by rbus_errorCode_e.
 *  Possible values are: RBUS_ERROR_INVALID_INPUT
 */
rbus_errorCode_e rbus_updateTable(rbusHandle_t rbusHandle, char *tableName,
                                    rbus_tblAction_e action, char* aliasName);
/** @} */


#ifdef __cplusplus
}
#endif
#endif
