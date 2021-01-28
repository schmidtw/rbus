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

#ifndef __RBUS_TEST_MACROS_H
#define __RBUS_TEST_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#define TALLY(T) \
    do \
    {  \
        if(!(T)) \
        { \
            gCountFail++; \
        } \
        else \
        { \
            gCountPass++; \
        } \
    }while(0);

#define TEST(T) \
    do \
    {  \
        TALLY(T) \
        if(!(T)) \
            printf("\nTEST failed at %s:%d\n" #T "\n", __FILE__, __LINE__ ); \
    }while(0);


#define VERIFY(T) \
    do \
    {  \
        int rc = (T); \
        if(rc != RBUS_ERROR_SUCCESS) \
        { \
            printf("_test_%s " #T  " failed: %d\n", __FUNCTION__, rc); \
            return rc; \
        } \
    }while(0);

#define PRINT_TEST_RESULTS(TESTNAME) printf("%s results: PASS=%d FAIL=%d\n", (TESTNAME), gCountPass, gCountFail);

#define PRINT_TEST_EVENT(TESTNAME, EVENT, SUBSCRIPTION) \
    printf("\n############################################################################\n" \
        "TEST %s\n" \
        " SUBSCRIPTION:\n" \
        "   eventName=%s\n" \
        "   userData=%s\n" \
        " EVENT:\n" \
        "   type=%d\n" \
        "   name=%s\n" \
        "   data=\n", \
            (TESTNAME), \
            (SUBSCRIPTION)->eventName, \
            (char*)(SUBSCRIPTION)->userData, \
            (EVENT)->type, \
            (EVENT)->name); \
    rbusObject_fwrite((EVENT)->data, 8, stdout); \
    printf("\n############################################################################\n");

static int gCountPass = 0;
static int gCountFail = 0;

#ifdef __cplusplus
}
#endif
#endif
