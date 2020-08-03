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

#include <stdlib.h>
#include <rtRetainable.h>
#include <rbus_marshalling.h>
#include "rbus_filter.h"

typedef struct _rbusFilter_LogicExpression* rbusFilter_LogicExpression_t;

struct _rbusFilter_RelationExpression
{
    rbusFilter_RelationOperator_t op;
    rbusValue_t value;
};

struct _rbusFilter_LogicExpression
{
    rbusFilter_LogicOperator_t op;
    rbusFilter_t left;
    rbusFilter_t right;
};

struct _rbusFilter
{
    rtRetainable retainable;
    union
    {
        struct _rbusFilter_RelationExpression relation;
        struct _rbusFilter_LogicExpression logic;
    } e;
    rbusFilter_ExpressionType_t type;
};

void rbusFilter_InitRelation(rbusFilter_t* filter, rbusFilter_RelationOperator_t op, rbusValue_t value)
{
    (*filter) = malloc(sizeof(struct _rbusFilter));

    (*filter)->type = RBUS_FILTER_EXPRESSION_RELATION;
    (*filter)->e.relation.op = op;
    (*filter)->e.relation.value = value;
    (*filter)->retainable.refCount = 1;
    rbusValue_Retain(value);
}

void rbusFilter_InitLogic(rbusFilter_t* filter, rbusFilter_LogicOperator_t op, rbusFilter_t left, rbusFilter_t right)
{
    (*filter) = malloc(sizeof(struct _rbusFilter));

    (*filter)->type = RBUS_FILTER_EXPRESSION_LOGIC;
    (*filter)->e.logic.op = op;
    (*filter)->e.logic.left = left;
    (*filter)->e.logic.right = right;
    (*filter)->retainable.refCount = 1;

    if((*filter)->e.logic.left)
        rbusFilter_Retain((*filter)->e.logic.left);

    if((*filter)->e.logic.right)
        rbusFilter_Retain((*filter)->e.logic.right);
}

void rbusFilter_Destroy(rtRetainable* r)
{
    rbusFilter_t filter = (rbusFilter_t)r;

    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        if(filter->e.relation.value)
            rbusValue_Release(filter->e.relation.value);
    }
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        if(filter->e.logic.left)
            rbusFilter_Release(filter->e.logic.left);
        if(filter->e.logic.right)
            rbusFilter_Release(filter->e.logic.right);
    }
    free(filter);
}

void rbusFilter_Retain(rbusFilter_t filter)
{
    rtRetainable_retain(filter);
}

void rbusFilter_Release(rbusFilter_t filter)
{
    rtRetainable_release(filter, rbusFilter_Destroy);
}

bool rbusFilter_RelationApply(struct _rbusFilter_RelationExpression* ex, rbusValue_t value)
{
    int c = rbusValue_Compare(value, ex->value);
    switch(ex->op)
    {
    case RBUS_FILTER_OPERATOR_GREATER_THAN:
        return c > 0;
    case RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL:
        return c >= 0;
    case RBUS_FILTER_OPERATOR_LESS_THAN:
        return c < 0;
    case RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL:
        return c <= 0;
    case RBUS_FILTER_OPERATOR_EQUAL:
        return c == 0;
    case RBUS_FILTER_OPERATOR_NOT_EQUAL:
        return c != 0;
    default:
        return false;
    }
}

bool rbusFilter_LogicApply(struct _rbusFilter_LogicExpression* ex, rbusValue_t value)
{
    bool left, right;

    left = rbusFilter_Apply(ex->left, value);

    if(ex->op != RBUS_FILTER_OPERATOR_NOT)
        right = rbusFilter_Apply(ex->right, value);

    if(ex->op == RBUS_FILTER_OPERATOR_AND)
    {
        return left && right;
    }
    else if(ex->op == RBUS_FILTER_OPERATOR_OR)
    {
        return left || right;
    }
    else if(ex->op == RBUS_FILTER_OPERATOR_NOT)
    {
        return !left;
    }
    return false;
}

bool rbusFilter_Apply(rbusFilter_t filter, rbusValue_t value)
{
    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
        return rbusFilter_RelationApply(&filter->e.relation, value);
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
        return rbusFilter_LogicApply(&filter->e.logic, value);
    else
        return false;
}

rbusFilter_ExpressionType_t rbusFilter_GetType(rbusFilter_t filter)
{
    return filter->type;
}

rbusFilter_RelationOperator_t rbusFilter_GetRelationOperator(rbusFilter_t filter)
{
    return filter->e.relation.op;
}

rbusValue_t rbusFilter_GetRelationValue(rbusFilter_t filter)
{
    return filter->e.relation.value;
}

rbusFilter_LogicOperator_t rbusFilter_GetLogicOperator(rbusFilter_t filter)
{
    return filter->e.logic.op;
}

rbusFilter_t rbusFilter_GetLogicLeft(rbusFilter_t filter)
{
    return filter->e.logic.left;
}

rbusFilter_t rbusFilter_GetLogicRight(rbusFilter_t filter)
{
    return filter->e.logic.right;
}

void rbusFilter_fwrite(rbusFilter_t filter, FILE* fout, rbusValue_t value)
{
    if(filter->type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        if(value)
            rbusValue_fwrite(value, 0, fout);
        else
            fwrite("X", 1, 1, fout);

        switch(filter->e.relation.op)
        {
        case RBUS_FILTER_OPERATOR_GREATER_THAN: fwrite(">",1,1,fout); break;
        case RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL: fwrite(">=",1,2,fout); break;
        case RBUS_FILTER_OPERATOR_LESS_THAN: fwrite("<",1,1,fout); break;
        case RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL: fwrite("<=",1,2,fout); break;
        case RBUS_FILTER_OPERATOR_EQUAL: fwrite("==",1,2,fout); break;
        case RBUS_FILTER_OPERATOR_NOT_EQUAL: fwrite("!=",1,2,fout); break;
        };

        rbusValue_fwrite(filter->e.relation.value, 0, fout);
    }
    else if(filter->type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        if(filter->e.logic.op == RBUS_FILTER_OPERATOR_NOT)
        {
            fwrite("!", 1, 1, fout);
        }

        fwrite("(", 1, 1, fout);

        rbusFilter_fwrite(filter->e.logic.left, fout, value);

        if(filter->e.logic.op == RBUS_FILTER_OPERATOR_AND)
        {
            fwrite(" && ", 1, 4, fout);
        }
        else if(filter->e.logic.op == RBUS_FILTER_OPERATOR_OR)
        {
            fwrite(" || ", 1, 4, fout);
        }

        if(filter->e.logic.op != RBUS_FILTER_OPERATOR_NOT)
        {
            rbusFilter_fwrite(filter->e.logic.right, fout, value);
        }

        fwrite(")", 1, 1, fout);
    }
}

