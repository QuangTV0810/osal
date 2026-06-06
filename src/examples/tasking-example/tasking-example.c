/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/*
** example1.c
**
** This is an example OSAL Application. This Application creates three tasks,
** and passes messages back and forth using queues
*/

#include <stdio.h>
// #ifdef _POSIX_THREADS
#include <pthread.h>
// #endif
#include "common_types.h"
#include "osapi.h"

/** Task 0 */
#define TASK_0_ID 0U
#define TASK_0_STACK_SIZE (1024U * 2U)
#define TASK_0_PRIORITY 100U

void task_0(void);

/* Task 1 */

#define TASK_1_ID 1
#define TASK_1_STACK_SIZE 1024 * 2
#define TASK_1_PRIORITY 101

// uint8 task_1_stack[TASK_1_STACK_SIZE];

void task_1(void);

/* Task 2 */

#define TASK_2_ID 2
#define TASK_2_STACK_SIZE 1024 * 4
#define TASK_2_PRIORITY 102

// uint8 task_2_stack[TASK_2_STACK_SIZE];

void task_2(void);

/* Task 3 */

#define TASK_3_ID 3
#define TASK_3_STACK_SIZE 1024 * 4
#define TASK_3_PRIORITY 103

// uint8 task_3_stack[TASK_3_STACK_SIZE];

void task_3(void);


/* Task 4 */

#define TASK_4_ID 4
#define TASK_4_STACK_SIZE 1024
#define TASK_4_PRIORITY 104

void task_4(void);

/* OS Constructs */

#define MSGQ_ID 1
#define MSGQ_DEPTH 50
#define MSGQ_SIZE 4

#define CTRLQ_ID 2
#define CTRLQ_DEPTH 4
#define CTRLQ_SIZE 4

#define MUTEX_ID 1

osal_id_t task_0_id;
osal_id_t task_1_id;
osal_id_t task_2_id;
osal_id_t task_3_id;
osal_id_t task_4_id;

osal_id_t mutex_id;
osal_id_t msgq_id;
osal_id_t ctrlq_id;

/* Global Data */

uint32 shared_resource_x;
bool task_0_delete_requested;

/* ********************** MAIN **************************** */

void OS_Application_Startup(void)
{
    uint32 status;

    OS_API_Init();

    OS_printf("********If You see this, we got into OS_Application_Startup****\n");

    status = OS_QueueCreate(&msgq_id, "MsgQ", MSGQ_DEPTH, MSGQ_SIZE, 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Message Queue\n");
    }

    status = OS_QueueCreate(&ctrlq_id, "CtrlQ", CTRLQ_DEPTH, CTRLQ_SIZE, 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Control Queue\n");
    }

    status = OS_MutSemCreate(&mutex_id, "Mutex", 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating mutex\n");
    }
    else
    {
        OS_printf("MutexSem ID = %d\n", (int)mutex_id);
    }

    status = OS_TaskCreate(&task_0_id, "Task_0", task_0, OSAL_TASK_STACK_ALLOCATE, TASK_0_STACK_SIZE, OSAL_PRIORITY_C(TASK_0_PRIORITY), 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Task 0\n");
    }

    status = OS_TaskCreate(&task_1_id, "Task 1", task_1, OSAL_TASK_STACK_ALLOCATE, TASK_1_STACK_SIZE, OSAL_PRIORITY_C(TASK_1_PRIORITY), 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Task 1\n");
    }

    status = OS_TaskCreate(&task_2_id, "Task 2", task_2, OSAL_TASK_STACK_ALLOCATE, TASK_2_STACK_SIZE, OSAL_PRIORITY_C(TASK_2_PRIORITY), 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Task 2\n");
    }

    status = OS_TaskCreate(&task_3_id, "Task 3", task_3, OSAL_TASK_STACK_ALLOCATE, TASK_3_STACK_SIZE, OSAL_PRIORITY_C(TASK_3_PRIORITY), 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Task 3\n");
    }

    status = OS_TaskCreate(&task_4_id, "Task 4", task_4, OSAL_TASK_STACK_ALLOCATE, TASK_4_STACK_SIZE, OSAL_PRIORITY_C(TASK_4_PRIORITY), 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error creating Task 4\n");
    }
}

/* ********************** TASK 0 **************************** */
static void delete_task_handler_callback(void)
{
    OS_printf("Task 0 delete handler invoked\n");
}

void task_0(void)
{
    uint32 command;
    uint32 data_size;
    int32 status;
// #ifdef _POSIX_THREADS
    pthread_t native_task;
    osal_id_t found_task_id;
// #endif

    OS_printf("Starting task 0\n");
    status = OS_TaskInstallDeleteHandler(delete_task_handler_callback);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error installing delete handler for Task 0\n");
    }

// #ifdef _POSIX_THREADS
    native_task = pthread_self();
    status      = OS_TaskFindIdBySystemData(&found_task_id, &native_task, sizeof(native_task));
    OS_printf("Task 0 find-by-system-data status = %ld, found task id = %lu\n",
              (long)status,
              OS_ObjectIdToInteger(found_task_id));
// #endif

    while (1)
    {
        OS_printf("Task 0 waiting on control queue\n");
        status = OS_QueueGet(ctrlq_id, (void *)&command, CTRLQ_SIZE, &data_size, OS_PEND);
        OS_printf("Task 0 woke from control queue, status = %ld command = %lu\n",
                  (long)status,
                  (unsigned long)command);
    }
}

void task_4(void)
{
    uint8 cnt = 0;
    OS_printf("Starting task 4\n");
    while (1)
    {
        OS_TaskDelay(1000);
        OS_printf("Task 4: Hello World! %d\n", cnt++);

        if (cnt == 10U)
        {
            OS_printf("Task 4: Requesting Task 0 exit\n");
            OS_TaskExit();
        }
    }
}

/* ********************** TASK 1 **************************** */

void task_1(void)
{
    uint32 status;

    OS_printf("Starting task 1\n");

    while (1)
    {
        status = OS_MutSemTake(mutex_id);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 1:Error calling OS_MutSemTake with mutex_id = %d\n", (int)mutex_id);
        }

        shared_resource_x = task_1_id;

        status = OS_QueuePut(msgq_id, (void *)&shared_resource_x, sizeof(uint32), 0);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 1:Error calling OS_QueuePut ( 1 )\n");
        }

        shared_resource_x = task_1_id;

        status = OS_QueuePut(msgq_id, (void *)&shared_resource_x, sizeof(uint32), 0);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 1:Error calling OS_QueuePut ( 2 )\n");
        }

        status = OS_MutSemGive(mutex_id);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 1:Error calling OS_MutSemGive\n");
        }

        OS_TaskDelay(100);
    }
}

/* ********************** TASK 2 **************************** */

void task_2(void)
{
    uint32 status;
    status = OS_TaskInstallDeleteHandler(delete_task_handler_callback);
    if (status != OS_SUCCESS)
    {
        OS_printf("Error installing delete handler for Task 0\n");
    }
    OS_printf("Starting task 2\n");

    while (1)
    {
        status = OS_MutSemTake(mutex_id);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 2:Error calling OS_MutSemTake\n");
        }

        shared_resource_x = task_2_id;

        status = OS_QueuePut(msgq_id, (void *)&shared_resource_x, sizeof(uint32), 0);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 2:Error calling OS_QueuePut (1)\n");
        }

        OS_TaskDelay(150);

        shared_resource_x = task_2_id;

        status = OS_QueuePut(msgq_id, (void *)&shared_resource_x, sizeof(uint32), 0);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 2:Error calling OS_QueuePut (2)\n");
        }

        status = OS_MutSemGive(mutex_id);
        if (status != OS_SUCCESS)
        {
            OS_printf("TASK 2:Error calling OS_MutSemGive\n");
        }

        OS_TaskDelay(500);
    }
}

/* ********************** TASK 3 **************************** */

void task_3(void)
{
    uint32 data_received;
    uint32 data_size;
    uint32 receive_count = 0U;
    uint32 status;

    OS_printf("Starting task 3\n");

    while (1)
    {
        status = OS_QueueGet(msgq_id, (void *)&data_received, MSGQ_SIZE, &data_size, OS_PEND);

        if (status == OS_SUCCESS)
        {
            ++receive_count;
            OS_printf("TASK 3: Received - %d\n", (int)data_received + 1);

            if (task_0_delete_requested == false && receive_count >= 10U)
            {
                task_0_delete_requested = true;
                status = OS_TaskGetIdByName(&task_0_id, "Task_0");
                if (status == OS_SUCCESS)
                {
                    // OS_QueuePut(ctrlq_id, (void *)&task_0_exit_requested, sizeof(uint32), 0);
                    // OS_printf("TASK 3: Requesting Task 0 exit\n");

                    status = OS_TaskDelete(task_0_id);
                    OS_printf("TASK 3: OS_TaskDelete(Task 0) status = %ld\n", (long)status);
                }
                // else
                // {
                //     OS_printf("TASK 3: Error calling OS_TaskGetIdByName for Task_0\n");
                // }
                // OS_printf("TASK 3: Calling OS_TaskDelete(Task 0) to test delete handler\n");
                // status = OS_TaskDelete(task_0_id);
                // OS_printf("TASK 3: OS_TaskDelete(Task 0) status = %ld\n", (long)status);
            }
        }
        else
        {
            OS_printf("TASK 3: Error calling OS_QueueGet\n");
        }
    }
}
