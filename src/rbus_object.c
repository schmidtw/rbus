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

#include <rbus.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <rtRetainable.h>

struct _rbusObject
{
    rtRetainable retainable;
    char* name;
    rbusObjectType_t type;      /*single or multi-instance*/
    rbusProperty_t properties;  /*the list of properties(tr181 parameters) on this object*/
    struct _rbusObject* parent;  /*the object's parent (NULL for root object)*/
    struct _rbusObject* children;/*the object's child objects which could be a mix of single or multi instance tables (not rows)*/
    struct _rbusObject* next;    /*this object's siblings where the type of sibling is based on parent type
                                  1) if parent is RBUS_OBJECT_SINGLE_INSTANCE | RBUS_OBJECT_MULTI_INSTANCE_ROW: 
                                        list of RBUS_OBJECT_SINGLE_INSTANCE and/or RBUS_OBJECT_MULTI_INSTANCE_TABLE
                                  2) if parent RBUS_OBJECT_MULTI_INSTANCE_TABLE: next is in a list of RBUS_OBJECT_MULTI_INSTANCE_ROW*/
};

void rbusObject_Init(rbusObject_t* object, char const* name)
{
    (*object) = malloc(sizeof(struct _rbusObject));

    if(name)
        (*object)->name = strdup(name);
    else
        (*object)->name = NULL;

    (*object)->properties = NULL;

    (*object)->retainable.refCount = 1;

    (*object)->parent = (*object)->children = (*object)->next = NULL;

    (*object)->type = RBUS_OBJECT_SINGLE_INSTANCE;
}

void rbusObject_InitMultiInstance(rbusObject_t* pobject, char const* name)
{
    rbusObject_Init(pobject, name);
    (*pobject)->type = RBUS_OBJECT_MULTI_INSTANCE;
}

void rbusObject_Destroy(rtRetainable* r)
{
    rbusObject_t object = (rbusObject_t)r;
    if(object->name)
    {
        free(object->name);
        object->name = NULL;
    }
    if(object->properties)
    {
        rbusProperty_Release(object->properties);
    }

    rbusObject_SetChildren(object, NULL);
    rbusObject_SetNext(object, NULL);
    rbusObject_SetParent(object, NULL);

    free(object);
}

void rbusObject_Retain(rbusObject_t object)
{
    rtRetainable_retain(object);
}

void rbusObject_Release(rbusObject_t object)
{
    rtRetainable_release(object, rbusObject_Destroy);
}

int rbusObject_Compare(rbusObject_t object1, rbusObject_t object2, bool recursive)
{
    int rc;
    int match;
    rbusProperty_t prop1;
    rbusProperty_t prop2;
    char const* prop1Name;
    char const* prop2Name;

    if(object1 == object2)
        return 0;

    rc = strcmp(object1->name, object2->name);
    if(rc != 0)
        return rc;

    /*verify each property in object1 has a matching property in object2*/
    prop1 = object1->properties;
    while(prop1)
    {
        prop1Name = rbusProperty_GetName(prop1);
        prop2 = object2->properties;
        match = 0;
        while(prop2)
        {
            prop2Name = rbusProperty_GetName(prop2);
            if(strcmp(prop1Name, prop2Name)==0)
            {
                rc = rbusProperty_Compare(prop1, prop2);
                if(rc != 0)
                    return rc;
                match = 1;
                break;
            }
            prop2 = rbusProperty_GetNext(prop2);
        }
        if(!match)
            return 1; /*TODO: 1 implies object1 > object2 but its unclear why that should be the case*/
        prop1 = rbusProperty_GetNext(prop1);
    }

    /*verify there are no additional properties in object2 that do not exist in object1*/
    prop2 = object2->properties;
    while(prop2)
    {
        prop2Name = rbusProperty_GetName(prop2);
        prop1 = object1->properties;
        match = 0;
        while(prop1)
        {
            prop1Name = rbusProperty_GetName(prop1);

            if(strcmp(prop2Name, prop1Name)==0)
            {
                match = 1;
                break;
            }
            prop1 = rbusProperty_GetNext(prop1);
        }
        if(!match)
            return -1; /*TODO: -1 implies object1 < object2 but its unclear why that should be the case*/
        prop2 = rbusProperty_GetNext(prop2);
    }

    /*compare children, recursively*/
    if(recursive)
    {
        int rc;
        int match;
        rbusObject_t obj1;
        rbusObject_t obj2;
        char const* obj1Name;
        char const* obj2Name;
        /*verify each property in object1 has a matching property in object2*/
        obj1 = object1->children;
        while(obj1)
        {
            obj1Name = rbusObject_GetName(obj1);
            obj2 = object2->children;
            match = 0;
            while(obj2)
            {
                obj2Name = rbusObject_GetName(obj2);
                if(strcmp(obj1Name, obj2Name)==0)
                {
                    rc = rbusObject_Compare(obj1, obj2, true);
                    if(rc != 0)
                        return rc;
                    match = 1;
                    break;
                }
                obj2 = rbusObject_GetNext(obj2);
            }
            if(!match)
                return 1;
            obj1 = rbusObject_GetNext(obj1);
        }
        /*verify there are no additional properties in object2 that do not exist in object1*/
        obj2 = object2->children;
        while(obj2)
        {
            obj2Name = rbusObject_GetName(obj2);
            obj1 = object1->children;
            match = 0;
            while(obj1)
            {
                obj1Name = rbusObject_GetName(obj1);
                if(strcmp(obj2Name, obj1Name)==0)
                {
                    match = 1;
                    break;
                }
                obj1 = rbusObject_GetNext(obj1);
            }
            if(!match)
                return -1;
            obj2 = rbusObject_GetNext(obj2);
        }
    }
    return 0; /*everything is equal*/
}

char const* rbusObject_GetName(rbusObject_t object)
{
    return object->name;
}

void rbusObject_SetName(rbusObject_t object, char const* name)
{
    if(object->name)
        free(object->name);
    if(name)
        object->name = strdup(name);
    else
        object->name = NULL;
}

rbusProperty_t rbusObject_GetProperties(rbusObject_t object)
{
    return object->properties;
}

void rbusObject_SetProperties(rbusObject_t object, rbusProperty_t properties)
{
    if(object->properties)
        rbusProperty_Release(object->properties);
    object->properties = properties;
    if(object->properties)
        rbusProperty_Retain(object->properties);
}

rbusProperty_t rbusObject_GetProperty(rbusObject_t object, char const* name)
{
    rbusProperty_t prop = object->properties;
    while(prop && strcmp(rbusProperty_GetName(prop), name))
        prop = rbusProperty_GetNext(prop);
    return prop;
}

void rbusObject_SetProperty(rbusObject_t object, rbusProperty_t newProp)
{
    if(object->properties == NULL)
    {
        rbusObject_SetProperties(object, newProp);
    }
    else
    {
        rbusProperty_t oldProp = object->properties;
        rbusProperty_t lastProp = NULL;
        /*search for property by name*/
        while(oldProp && strcmp(rbusProperty_GetName(oldProp), rbusProperty_GetName(newProp)))
        {
            if(rbusProperty_GetNext(oldProp))
            {
                lastProp = oldProp;
                oldProp = rbusProperty_GetNext(oldProp);
            }
            else
            {
                lastProp = oldProp;
                oldProp = NULL;
            }
        }
        if(oldProp)/*existing property found by name*/
        {   /*replace oldProp with newProp, preserving the rest of the property list*/

            /*append all the properties after oldProp to the newProp*/
            rbusProperty_t next = rbusProperty_GetNext(oldProp);
            if(next)
            {
                rbusProperty_PushBack(newProp, next);/*newProp will retain the list*/
            }
            
            /*link newProp to the tail of all the properties that came before oldProp*/
            if(lastProp) 
            {
                /*this will release oldProp(which will release oldProp's next)
                  and retain newPorp(which retained the oldProp's next above)*/
                rbusProperty_SetNext(lastProp, newProp);
            }
            else
            {
                rbusObject_SetProperties(object, newProp);/*this will release oldProp and retain newProp*/
            }
        }
        else/*no existing property with that name*/
        {
            rbusProperty_SetNext(lastProp, newProp);/*this will retain property*/
        }
    }        
}

rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* name)
{
    rbusProperty_t prop;
    if(name)
        prop = rbusObject_GetProperty(object, name);
    else
        prop = object->properties;
    if(prop)
        return rbusProperty_GetValue(prop);
    else
        return NULL;
}

void rbusObject_SetValue(rbusObject_t object, char const* name, rbusValue_t value)
{
    rbusProperty_t prop = rbusObject_GetProperty(object, name);
    if(prop)
    {
        rbusProperty_SetValue(prop, value);
    }
    else
    {
        rbusProperty_Init(&prop, name, value);
        if(object->properties == NULL)
        {
            rbusObject_SetProperties(object, prop);
        }
        else
        {
            rbusProperty_PushBack(object->properties, prop);
        }
        rbusProperty_Release(prop);
    }
}

rbusObject_t rbusObject_GetParent(rbusObject_t object)
{
    return object->parent;
}

void rbusObject_SetParent(rbusObject_t object, rbusObject_t parent)
{
    object->parent = parent;
    /*
    if(object->parent)
        rbusObject_Release(object->parent);
    object->parent = parent;
    if(object->parent)
        rbusObject_Retain(object->parent);
    */
}

rbusObject_t rbusObject_GetChildren(rbusObject_t object)
{
    return object->children;
}

void rbusObject_SetChildren(rbusObject_t object, rbusObject_t children)
{
    if(object->children)
        rbusObject_Release(object->children);
    object->children = children;
    if(object->children)
        rbusObject_Retain(object->children);
}

rbusObject_t rbusObject_GetNext(rbusObject_t object)
{
    return object->next;
}

void rbusObject_SetNext(rbusObject_t object, rbusObject_t next)
{
    if(object->next)
        rbusObject_Release(object->next);
    object->next = next;
    if(object->next)
        rbusObject_Retain(object->next);
}

rbusObjectType_t rbusObject_GetType(rbusObject_t object)
{
    return object->type;
}

void rbusObject_fwrite(rbusObject_t obj, int depth, FILE* fout)
{
    int i;
    rbusObject_t child;
    rbusProperty_t prop;
    for(i=0; i<depth; ++i)
        fprintf(fout, " ");
    fprintf(fout, "rbusObject name=%s\n\r", rbusObject_GetName(obj));
    prop = rbusObject_GetProperties(obj);
    rbusProperty_fwrite(prop, depth+1, fout);
    child = rbusObject_GetChildren(obj);
    while(child)
    {
        rbusObject_fwrite(child, depth+1, fout);
        child = rbusObject_GetNext(child);
    }
}

#if 0
void rbusObject_SetBoolean(rbusObject_t object, char const* name, bool b);
void rbusObject_SetChar(rbusObject_t object, char const* name, char c);
void rbusObject_SetUChar(rbusObject_t object, char const* name, unsigned char u);
void rbusObject_SetInt8(rbusObject_t object, char const* name, int8_t i8);
void rbusObject_SetUInt8(rbusObject_t object, char const* name, uint8_t u8);
void rbusObject_SetInt16(rbusObject_t object, char const* name, int16_t i16);
void rbusObject_SetUInt16(rbusObject_t object, char const* name, uint16_t u16);
void rbusObject_SetInt32(rbusObject_t object, char const* name, int32_t i32);
void rbusObject_SetUInt32(rbusObject_t object, char const* name, uint32_t u32);
void rbusObject_SetInt64(rbusObject_t object, char const* name, int64_t i64);
void rbusObject_SetUInt64(rbusObject_t object, char const* name, uint64_t u64);
void rbusObject_SetSingle(rbusObject_t object, char const* name, float f32);
void rbusObject_SetDouble(rbusObject_t object, char const* name, double f64);
void rbusObject_SetTime(rbusObject_t object, char const* name, struct timeval* tv);
void rbusObject_SetString(rbusObject_t object, char const* name, char const* s);
void rbusObject_SetBytes(rbusObject_t object, char const* name, uint8_t const* bytes, int len);
void rbusObject_SetObject(rbusObject_t object, char const* name, rbusObject_t o);

bool rbusObject_GetBoolean(rbusObject_t object, char const* name);
char rbusObject_GetChar(rbusObject_t object, char const* name);
unsigned char rbusObject_GetUChar(rbusObject_t object, char const* name);
int8_t rbusObject_GetInt8(rbusObject_t object, char const* name);
uint8_t rbusObject_GetUInt8(rbusObject_t object, char const* name);
int16_t rbusObject_GetInt16(rbusObject_t object, char const* name);
uint16_t rbusObject_GetUInt16(rbusObject_t object, char const* name);
int32_t rbusObject_GetInt32(rbusObject_t object, char const* name);
uint32_t rbusObject_GetUInt32(rbusObject_t object, char const* name);
int64_t rbusObject_GetInt64(rbusObject_t object, char const* name);
uint64_t rbusObject_GetUInt64(rbusObject_t object, char const* name);
float rbusObject_GetSingle(rbusObject_t object, char const* name);
double rbusObject_GetDouble(rbusObject_t object, char const* name);
struct timeval* rbusObject_GetTime(rbusObject_t object, char const* name);
char const* rbusObject_GetString(rbusObject_t object, char const* name);
uint8_t const* rbusObject_GetBytes(rbusObject_t object, char const* name);
rbusObject_t rbusObject_GetObject(rbusObject_t object, char const* name);
#endif 

#if 0

void rbusObject_SetBoolean(rbusObject_t object, char const* name, bool b)
{
    rbusProperty_t prop = rbusObject_GetProperty(object, property->name);
    if(prop)
        return rbusProperty_SetValue(prop, rbusValue_FromBoolean(value));
}
void rbusObject_SetChar(rbusObject_t object, char const* name, char c)
{
}
void rbusObject_SetUChar(rbusObject_t object, char const* name, unsigned char u)
{
}
void rbusObject_SetInt8(rbusObject_t object, char const* name, int8_t i8)
{
}
void rbusObject_SetUInt8(rbusObject_t object, char const* name, uint8_t u8)
{
}
void rbusObject_SetInt16(rbusObject_t object, char const* name, int16_t i16)
{
}
void rbusObject_SetUInt16(rbusObject_t object, char const* name, uint16_t u16)
{
}
void rbusObject_SetInt32(rbusObject_t object, char const* name, int32_t i32)
{
}
void rbusObject_SetUInt32(rbusObject_t object, char const* name, uint32_t u32)
{
}
void rbusObject_SetInt64(rbusObject_t object, char const* name, int64_t i64)
{
}
void rbusObject_SetUInt64(rbusObject_t object, char const* name, uint64_t u64)
{
}
void rbusObject_SetSingle(rbusObject_t object, char const* name, float f32)
{
}
void rbusObject_SetDouble(rbusObject_t object, char const* name, double f64)
{
}
void rbusObject_SetTime(rbusObject_t object, char const* name, struct timeval* tv)
{
}
void rbusObject_SetString(rbusObject_t object, char const* name, char const* s)
{
}
void rbusObject_SetBytes(rbusObject_t object, char const* name, uint8_t const* bytes, int len)
{
}
void rbusObject_SetObject(rbusObject_t object, char const* name, rbusObject_t o)
{
}
bool rbusObject_GetBoolean(rbusObject_t object, char const* name)
{
}
char rbusObject_GetChar(rbusObject_t object, char const* name)
{
}
unsigned char rbusObject_GetUChar(rbusObject_t object, char const* name)
{
}
int8_t rbusObject_GetInt8(rbusObject_t object, char const* name)
{
    rbusProperty_t prop = rbusObject_GetProperty(object, property->name);
    if(prop)
        return rbusProperty
    return 0;
}
uint8_t rbusObject_GetUInt8(rbusObject_t object, char const* name)
{
    return 0;
}
int16_t rbusObject_GetInt16(rbusObject_t object, char const* name)
{
    return 0;
}
uint16_t rbusObject_GetUInt16(rbusObject_t object, char const* name)
{
    return 0;
}
int32_t rbusObject_GetInt32(rbusObject_t object, char const* name)
{
    return 0;
}
uint32_t rbusObject_GetUInt32(rbusObject_t object, char const* name)
{
    return 0;
}
int64_t rbusObject_GetInt64(rbusObject_t object, char const* name)
{
    return 0;
}
uint64_t rbusObject_GetUInt64(rbusObject_t object, char const* name)
{
    return 0;
}
float rbusObject_GetSingle(rbusObject_t object, char const* name)
{
    return 0;
}
double rbusObject_GetDouble(rbusObject_t object, char const* name)
{
    return 0;
}
struct timeval* rbusObject_GetTime(rbusObject_t object, char const* name)
{
    return 0;
}
char const* rbusObject_GetString(rbusObject_t object, char const* name)
{
    return 0;
}
uint8_t const* rbusObject_GetBytes(rbusObject_t object, char const* name)
{
    return 0;
}
rbusObject_t rbusObject_GetObject(rbusObject_t object, char const* name)
{
    return 0;
}
#endif
