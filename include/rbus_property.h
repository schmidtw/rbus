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
 * @file        rubs_property.h
 * @brief       rbusProperty
 * @defgroup    rbusProperty
 * @brief       An rbus property is a name/value pair.  It allows assigning a name (e.g. path) to an rbus value.
 *              An rbusProperty_t is a reference counted handle to an rbus property.
 * @{
 */

#ifndef RBUS_PROPERTY_H
#define RBUS_PROPERTY_H

#include <rbus_value.h>

/**
 * @brief       A handle to an rbus property.
 */
typedef struct _rbusProperty* rbusProperty_t;

/** @fn void rbusProperty_Init(rbusProperty_t* pproperty, char const* name, rbusValue_t value)
 *  @brief  Allocate and initialize a property.
 *          This automatically retains ownership of the property. 
 *          It's the caller's responsibility to release ownership by
 *          calling rbusProperty_Release once it's done with it.
 *  @param  pproperty reference to an address where the new property will be assigned.
 *          The caller is responsible for releasing the property with rbusProperty_Release
 *  @name   optional name to assign the property.  The name is dublicated/copied.  
 *          If NULL is passed the property's name will be NULL.
 *  @value  optional value to assign the property.  The property will retain ownership of the value.
 *          If the value is NULL, the property's value will be NULL.
 */
void rbusProperty_Init(rbusProperty_t* pproperty, char const* name, rbusValue_t value);

/** @fn void rbusProperty_Retain(rbusProperty_t property)
 *  @brief Take shared ownership of the property.  This allows a property to have 
 *         multiple owners.  The first owner obtains ownership with rbusProperty_Init.
 *         Additional owners can be assigned afterwards with rbusProperty_Retain.  
 *         Each owner must call rbusProperty_Release once done using the property.
 *  @param property the property to retain
 */
void rbusProperty_Retain(rbusProperty_t property);

/** @fn void rbusProperty_Release(rbusProperty_t property)
 *  @brief Release ownership of the property.  This must be called when done using
 *         a property that was retained with either rbusProperty_Init or rbusProperty_Retain.
 *  @param property the property to release
 */
void rbusProperty_Release(rbusProperty_t property);

/** @fn void rbusProperty_Compare(rbusProperty_t property1, rbusProperty_t property2)
 *  @brief Compare two properties for equality.  They are equal if both their names and values are equal.
 *  @param property1 the first property to compare
 *  @param property2 the second property to compare
 *  @return The compare result where 0 is equal and non-zero if not equal.  
 */
int rbusProperty_Compare(rbusProperty_t property1, rbusProperty_t property2);

/** @fn char* rbusProperty_GetName(rbusProperty_t property)
 *  @brief Get the name of a property.  
 *  @param property A property.
 *  @return The name of the property.  To avoid copies, this is a reference to the underlying name.
 */ 
char const* rbusProperty_GetName(rbusProperty_t property);

/** @fn void rbusProperty_SetName(rbusProperty_t property, char const* name)
 *  @brief Set the name of a property.  
 *  @param property A property.
 *  @param name the name to set which will be duplicated/copied
 */ 
void rbusProperty_SetName(rbusProperty_t property, char const* name);

/** @fn rbusValue_t rbusProperty_GetValue(rbusProperty_t property)
 *  @brief Get the value of a property.  
 *  @param property A property.
 *  @return The value of the property.  If the caller intends to store this value, 
 *          it should take ownership by calling rbusValue_Retain and then
 *          when its done with it, call rbusValue_Release.
 */ 
rbusValue_t rbusProperty_GetValue(rbusProperty_t property);

/** @fn void rbusProperty_SetValue(rbusProperty_t property, rbusValue_t value)
 *  @brief Set the value of a property.  
 *  @param property A property.
 *  @param The value to set.  The property will retain ownership of the value.
 */ 
void rbusProperty_SetValue(rbusProperty_t property, rbusValue_t value);

/** @fn char* rbusProperty_GetNext(rbusProperty_t property)
 *  @brief Get the next property from the list.  Properties can be linked together into a list.
 *         Given a property in that list, the next property in the list can be obtained with this method. 
 *  @param property A property.
 *  @return The next property in the list.  If the caller intends to store this property, 
 *          it should take ownership by calling rbusProperty_Retain and then
 *          when its done with it, call rbusProperty_Release.
 */  
rbusProperty_t rbusProperty_GetNext(rbusProperty_t property);

/** @fn void rbusProperty_SetNext(rbusProperty_t property, rbusProperty_t next)
 *  @brief Set the next property in the list.  Properties can be linked together into a list.
 *         Given a property in that list, the next property in the list can be set with this method. 
 *         If next had been previously set, then ownership of the former next property is released.
 *  @param property A property.
 *  @param next A property to be set as the next in the list.  
           next can be a list itself, in which case the list is appended.
           next can be NULL, in which case the list is terminated.
           setting NULL is also a way to clear the list.
 */ 
void rbusProperty_SetNext(rbusProperty_t property, rbusProperty_t next);

/** @fn void rbusProperty_PushBack(rbusProperty_t property, rbusProperty_t back)
 *  @brief Append a property to the end of a property list.  
           Ownership of the property being appended will be retained.
 *  @param property A property.
 *  @param back A property to append to the end of the list.
           If back is NULL, then no action is taken.
 */ 
void rbusProperty_PushBack(rbusProperty_t property, rbusProperty_t back);

/** @fn void rbusProperty_Count(rbusProperty_t property)
 *  @brief Return the number or properties in the list
 *  @param property A property (the first in list).
 *  @return The number of properties in the list.
 */ 
uint32_t rbusProperty_Count(rbusProperty_t property);

void rbusProperty_fwrite(rbusProperty_t prop, int depth, FILE* fout);

#endif

/** @} */
