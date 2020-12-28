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

/**
 * @file        rubs_object.h
 * @brief       rbusObject
 * @defgroup    rbusObject
 * @brief       An rbus object is a named collection of properties.
 *
 * An rbusObject_t is a reference counted handle to an rbus object.  
 * @{
 */

#ifndef RBUS_OBJECT_H
#define RBUS_OBJECT_H

#include <rbus_property.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _rbusObjectType
{
    RBUS_OBJECT_SINGLE_INSTANCE,
    RBUS_OBJECT_MULTI_INSTANCE
}rbusObjectType_t;

/**
 * @brief       A handle to an rbus object.
 */
typedef struct _rbusObject* rbusObject_t;

/** @fn void rbusObject_Init(rbusObject_t* object, char const* name)
 *  @brief  Allocate, initialize, and take ownershipt of an object.
 *
 *  This automatically retains ownership of the object. 
 *  It's the caller's responsibility to release ownership by
 *  calling rbusObject_Release once it's done with it.
 *  @param  pobject reference to an address where the new object will be assigned.
 *          The caller is responsible for releasing the object with rbusObject_Release
 *  @param  name  optional name to assign the object.  The name is dublicated/copied.  
 *          If NULL is passed the object's name will be NULL.
 */
void rbusObject_Init(rbusObject_t* pobject, char const* name);

void rbusObject_InitMultiInstance(rbusObject_t* pobject, char const* name);

/** @fn void rbusObject_Retain(rbusObject_t object)
 *  @brief Take shared ownership of the object.
 *
 *  This allows an object to have multiple owners.  The first owner obtains ownership
 *  with rbusObject_Init. Additional owners can be assigned afterwards with rbusObject_Retain.
 *  Each owner must call rbusObject_Release once done using the object.
 *  @param object the object to retain
 */
void rbusObject_Retain(rbusObject_t object);

/** @fn void rbusObject_Release(rbusObject_t object)
 *  @brief Release ownership of the object.
 *
 *  This must be called when done using a object that was retained with either rbusObject_Init or rbusObject_Retain.
 *  @param object the object to release
 */
void rbusObject_Release(rbusObject_t object);

/** @fn void rbusObject_Compare(rbusObject_t object1, rbusObject_t object2)
 *  @brief Compare two objects for equality.  They are equal if both their names and values are equal.
 *  @param object1 the first object to compare
 *  @param object2 the second object to compare
 *  @param recursive compare all child and their descendants objects
 *  @return The compare result where 0 is equal and non-zero if not equal.  
 */
int rbusObject_Compare(rbusObject_t object1, rbusObject_t object2, bool recursive);

/** @fn char const* rbusObject_GetName(rbusObject_t object)
 *  @brief Get the name of a object.
 *  @param object An object.
 *  @return The name of the object.  To avoid copies, this is a reference to the underlying name.
 */
char const* rbusObject_GetName(rbusObject_t object);

/** @fn rbusObject_SetName(rbusObject_t object, char const* name)
 *  @brief Set the name of a object.
 *  @param object An object.
 *  @param name the name to set which will be duplicated/copied
 */ 
void rbusObject_SetName(rbusObject_t object, char const* name);

/** @fn rbusProperty_t rbusObject_GetProperties(rbusObject_t object)
 *  @brief Get the property list of an object.
 *  @param object An object.
 *  @return The property list of the object.
 */ 
rbusProperty_t rbusObject_GetProperties(rbusObject_t object);

/** @fn void rbusObject_SetProperties(rbusObject_t object, rbusProperty_t properties)
 *  @brief Set the property list of an object.
 *
 *  If a property list already exists on the object, then ownership of all properties in that list is released.
 *  @param object An object.
 *  @param properties A property list to set on the object.  Any existing propery list is released.
 */
void rbusObject_SetProperties(rbusObject_t object, rbusProperty_t properties);

/** @fn rbusProperty_t rbusObject_GetProperty(rbusObject_t object, char const* name)
 *  @brief Get a property by name from an object.
 *  @param object An object.
 *  @param name The name of the property to get.
 *  @return The property matching the name, or NULL if no match found.
 */ 
rbusProperty_t rbusObject_GetProperty(rbusObject_t object, char const* name);

/** @fn void rbusObject_SetProperty(rbusObject_t object, rbusProperty_t property)
 *  @brief Set a property on an object.
 *
 *  If a property with the same name already exists, its ownership released.
 *  @param object An object.
 *  @param property A property to set on the object.  The caller should set the name
           on this property before calling this method.
 */
void rbusObject_SetProperty(rbusObject_t object, rbusProperty_t property);

/** @fn rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* name)
 *  @brief Get the value of a property by name from an object.
 *  @param object An object.
 *  @param name The name of the property to get the value of.
 *  @return The value of the property matching the name, or NULL if no match found.
 */ 
rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* name);

/** @fn void rbusObject_SetValue(rbusObject_t object, char const* name, rbusValue_t value)
 *  @brief Set the value of a property by name on an object.
 *
 * If a property with the same name does not exist, a new property is created.
 * Ownership of any previous property/value will be released.
 *  @param object An object.
 *  @param name the name of the property inside the object to set the value to
 *  @param value a value to set on the object's property
 */
void rbusObject_SetValue(rbusObject_t object, char const* name, rbusValue_t value);

rbusObject_t rbusObject_GetParent(rbusObject_t object);
void rbusObject_SetParent(rbusObject_t object, rbusObject_t parent);

rbusObject_t rbusObject_GetChildren(rbusObject_t object);
void rbusObject_SetChildren(rbusObject_t object, rbusObject_t children);

rbusObject_t rbusObject_GetNext(rbusObject_t object);
void rbusObject_SetNext(rbusObject_t object, rbusObject_t next);

rbusObjectType_t rbusObject_GetType(rbusObject_t object);

void rbusObject_fwrite(rbusObject_t obj, int depth, FILE* fout);

#ifdef __cplusplus
}
#endif
#endif
/** @} */
