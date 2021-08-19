/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "gtest/gtest.h"
#include <rbus.h>
#include <limits.h>
#include <errno.h>
#include "../src/rbus_buffer.h"
#include <rbus.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void rbusMessageHandler(rbusHandle_t handle, rbusMessage_t* msg, void * userData)
{
    (void)handle;
    (void)userData;
    printf("rbusMessageHandler topic=%s data=%.*s\n", msg->topic, msg->length, (char const *)msg->data);
}

static void rbusMessageHandler_neg(rbusHandle_t handle, rbusMessage_t* msg, void * userData)
{
    (void)handle;
    (void)userData;
    int count = 10;
    char buf[64] = {0};
    while(count--)
        sleep(1);
    printf("%s topic=%s data=%.*s\n", __func__,msg->topic, msg->length, (char const *)msg->data);
    snprintf(buf,sizeof(buf),"%.*s",msg->length, (char const *)msg->data);
    if(strcmp("0: Hello!",buf) == 0)
        pthread_cond_signal(&cond);
}

TEST(rbusMessageTest, testsend)
{
    pid_t pid = fork();
    if (pid == 0){
        rbusHandle_t rbus;
        char topic[] = "A.B.C";
        char buff[256];
        rbusMessage_t msg;
        int ret = 0;

        EXPECT_EQ(rbus_open(&rbus, "rbus_send"),0);
        msg.topic = topic;
        msg.data = (uint8_t*)buff;
        msg.length = snprintf(buff, sizeof(buff), "%ld: Hello!", time(NULL));
        sleep(3);
        ret=rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
        EXPECT_EQ(ret,0);
        printf("rbusMessage_Send topic=%s data=%.*s\n", msg.topic, msg.length, (char const *)msg.data);
        EXPECT_EQ(rbus_close(rbus),0);
        sleep(1);
        exit(ret);
    }else{
        int ret = 0;
        int customer_status;
        rbusHandle_t rbus;
        char topic[] = "A.B.C";
        char buff[256];
        rbusMessage_t msg;

        EXPECT_EQ(rbus_open(&rbus, "rbus_recv"),0);
        EXPECT_EQ(rbusMessage_AddListener(rbus, "A.B.C", &rbusMessageHandler, NULL),0);
        waitpid(pid, &customer_status, 0);
        EXPECT_EQ(rbusMessage_RemoveListener(rbus, "A.B.C"),0);
        EXPECT_EQ(rbus_close(rbus),0);
        printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
        EXPECT_EQ(WEXITSTATUS(customer_status),0);
        EXPECT_EQ(ret,0);
  }

}

TEST(rbusMessageTest, testsend1)
{
    pid_t pid = fork();
    if (pid == 0){
        rbusHandle_t rbus;
        char topic[] = "A.B.C";
        char buff[256];
        rbusMessage_t msg;
        int ret = 0;

        EXPECT_EQ(rbus_open(&rbus, "rbus_send"),0);
        msg.topic = topic;
        msg.data = (uint8_t*)buff;
        msg.length = snprintf(buff, sizeof(buff), "%ld: Hello!", time(NULL));
        sleep(3);
        //To check the part where RBUS_MESSAGE_NONE is sent as arg,
        //< The message is sent non-blocking with no confirmation of delivery */
        ret=rbusMessage_Send(rbus, &msg,RBUS_MESSAGE_NONE);
        EXPECT_EQ(ret,0);
        printf("rbusMessage_Send topic=%s data=%.*s\n", msg.topic, msg.length, (char const *)msg.data);
        EXPECT_EQ(rbus_close(rbus),0);
        sleep(1);
        exit(ret);
    }else{
        int ret = 0;
        int customer_status;
        rbusHandle_t rbus;
        char topic[] = "A.B.C";
        char buff[256];
        rbusMessage_t msg;

        EXPECT_EQ(rbus_open(&rbus, "rbus_recv"),0);
        EXPECT_EQ(rbusMessage_AddListener(rbus, "A.B.C", &rbusMessageHandler, NULL),0);
        waitpid(pid, &customer_status, 0);
        EXPECT_EQ(rbusMessage_RemoveListener(rbus, "A.B.C"),0);
        EXPECT_EQ(rbus_close(rbus),0);
        printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
        EXPECT_EQ(WEXITSTATUS(customer_status),0);
        EXPECT_EQ(ret,0);
    }
}

TEST(rbusMessageTest, test_negsend)
{
    rbusError_t err;
    rbusHandle_t rbus;
    char topic[] = "A.B.C";
    char buff[256];
    rbusMessage_t msg;

    msg.topic = topic;
    msg.data = (uint8_t*)buff;
    msg.length = snprintf(buff, sizeof(buff), "%ld: Hello!", time(NULL));

    EXPECT_EQ(rbus_open(&rbus, "rbus_send"),0);
    //To check the send error, when no listener is available RT_OBJECT_NO_LONGER_AVAILABLE
    EXPECT_EQ(rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT),5);
    printf("rbusMessage_Send topic=%s data=%.*s\n", msg.topic, msg.length, (char const *)msg.data);
    EXPECT_EQ(rbus_close(rbus),0);
}

TEST(rbusMessageTest, test_negsend1)
{
    pid_t pid = fork();
    if (pid == 0){
        rbusHandle_t rbus;
        //To check the send error, when sending to wrong topic RT_OBJECT_NO_LONGER_AVAILABLE
        char topic[] = "A.B.D";
        char buff[256];
        rbusMessage_t msg;
        int ret = 0;

        EXPECT_EQ(rbus_open(&rbus, "rbus_send"),0);
        msg.topic = topic;
        msg.data = (uint8_t*)buff;
        msg.length = snprintf(buff, sizeof(buff), "%ld: Hello!", time(NULL));
        sleep(3);
        ret=rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
        EXPECT_EQ(ret,5);
        printf("rbusMessage_Send topic=%s data=%.*s\n", msg.topic, msg.length, (char const *)msg.data);
        EXPECT_EQ(rbus_close(rbus),0);
        sleep(1);
        exit(ret);
    }else{
        int ret = 0;
        int customer_status;
        rbusHandle_t rbus;
        char topic[] = "A.B.C";
        char buff[256];
        rbusMessage_t msg;

        EXPECT_EQ(rbus_open(&rbus, "rbus_recv"),0);
        EXPECT_EQ(rbusMessage_AddListener(rbus, "A.B.C", &rbusMessageHandler, NULL),0);
        waitpid(pid, &customer_status, 0);
        EXPECT_EQ(rbusMessage_RemoveListener(rbus, "A.B.C"),0);
        EXPECT_EQ(rbus_close(rbus),0);
        printf("Value change customer result %d\n",WEXITSTATUS(customer_status));
        EXPECT_EQ(WEXITSTATUS(customer_status),5);
        EXPECT_EQ(ret,0);
    }
}

TEST(rbusMessageTest, DISABLED_test_negsend2)
{
    char topic[] = "A.B.C";
    pid_t pid = fork();
    if (pid == 0) {
        rbusHandle_t rbus;
        rbusMessage_t msg;
        char buff[256];
        int ret = 0;
        EXPECT_EQ(rbus_open(&rbus, "rbus_recv"),0);
        EXPECT_EQ(rbusMessage_AddListener(rbus, topic, &rbusMessageHandler_neg, NULL),0);
        pthread_cond_wait(&cond, &lock);
        //To get timeout generic error with RBUS_MESSAGE_CONFIRM_RECEIPT when handler is busy/wait
        EXPECT_EQ(rbusMessage_RemoveListener(rbus, topic),0);
        EXPECT_EQ(rbus_close(rbus),0);
        EXPECT_EQ(ret,0);
        exit(ret);
    } else {
        rbusHandle_t rbus;
        rbusMessage_t msg;
        rbusError_t err;
        char buff[256];
        int ret = 0,i = 2;
        int customer_status;
        EXPECT_EQ(rbus_open(&rbus, "rbus_send"),0);
        sleep(1);
        while(i--)
        {
            msg.topic = topic;
            msg.data = (uint8_t*)buff;
            msg.length = snprintf(buff, sizeof(buff), "%d: Hello!", i);
            err=rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
            if (err)
            {
                fprintf(stderr, "rbusMessage_Send:%s\n", rbusError_ToString(err));
            }
            EXPECT_NE(err,0);
            sleep(2);
        }
        EXPECT_EQ(rbus_close(rbus),0);
        waitpid(pid, &customer_status, 0);
        EXPECT_EQ(WEXITSTATUS(customer_status),0);
    }
}

TEST(rbusMessageTest, test_negsend3)
{
    char topic[] = "A.B.C";
    pid_t pid = fork();
    if (pid == 0) {
        rbusHandle_t rbus;
        rbusMessage_t msg;
        char buff[256];
        int ret = 0;
        EXPECT_EQ(rbus_open(&rbus, "rbus_recv"),0);
        EXPECT_EQ(rbusMessage_AddListener(rbus, topic, &rbusMessageHandler_neg, NULL),0);
        //To check the send using RBUS_MESSAGE_NONE when Listener is busy/wait
        pthread_cond_wait(&cond, &lock);
        EXPECT_EQ(rbusMessage_RemoveListener(rbus, topic),0);
        EXPECT_EQ(rbus_close(rbus),0);
        EXPECT_EQ(ret,0);
        exit(ret);
    } else {
        rbusHandle_t rbus;
        rbusMessage_t msg;
        rbusError_t err;
        char buff[256];
        int ret = 0,i = 2;
        int customer_status;
        EXPECT_EQ(rbus_open(&rbus, "rbus_send"),0);
        sleep(1);
        while(i--)
        {
            msg.topic = topic;
            msg.data = (uint8_t*)buff;
            msg.length = snprintf(buff, sizeof(buff), "%d: Hello!", i);
            err=rbusMessage_Send(rbus, &msg, RBUS_MESSAGE_NONE);
            if (err)
            {
                fprintf(stderr, "rbusMessage_Send:%s\n", rbusError_ToString(err));
            }
            EXPECT_EQ(err,0);
            sleep(2);
        }
        EXPECT_EQ(rbus_close(rbus),0);
        waitpid(pid, &customer_status, 0);
        EXPECT_EQ(WEXITSTATUS(customer_status),0);
    }
}
