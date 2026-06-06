#include "osapi.h"
#include "common_types.h"

#define TASK_STACK_SIZE 4096
#define WAITER_PRIORITY OSAL_PRIORITY_C(100)
#define SIGNAL_PRIORITY OSAL_PRIORITY_C(110)

static osal_id_t bin_sem_id;
static osal_id_t waiter_task_id;
static osal_id_t signal_task_id;

void waiter_task(void)
{
    while (1)
    {
        int32 status;

        OS_printf("waiter: waiting for event...\n");

        // status = OS_BinSemTake(bin_sem_id);
        status = OS_BinSemTimedWait(bin_sem_id, 1000);
        if (status == OS_SUCCESS)
        {
            OS_printf("waiter: event received\n");
            OS_TaskExit();
        }
        else
        {
            OS_printf("waiter: take failed, status=%ld\n", (long)status);
        }
    }
}

void signal_task(void)
{
    int32 status;

    OS_TaskDelay(2000);

    OS_printf("signal: sending event\n");
    status = OS_BinSemGive(bin_sem_id);
    OS_printf("signal: give status=%ld\n", (long)status);

    OS_TaskExit();
}

void OS_Application_Startup(void)
{
    int32 status;

    OS_API_Init();

    status = OS_BinSemCreate(&bin_sem_id, "EventSem", OS_SEM_EMPTY, 0);
    if (status != OS_SUCCESS)
    {
        OS_printf("create semaphore failed: %ld\n", (long)status);
        return;
    }

    status = OS_TaskCreate(&waiter_task_id,
                           "Waiter",
                           waiter_task,
                           OSAL_TASK_STACK_ALLOCATE,
                           TASK_STACK_SIZE,
                           WAITER_PRIORITY,
                           0);
    if (status != OS_SUCCESS)
    {
        OS_printf("create waiter failed: %ld\n", (long)status);
        return;
    }

    status = OS_TaskCreate(&signal_task_id,
                           "Signaler",
                           signal_task,
                           OSAL_TASK_STACK_ALLOCATE,
                           TASK_STACK_SIZE,
                           SIGNAL_PRIORITY,
                           0);
    if (status != OS_SUCCESS)
    {
        OS_printf("create signaler failed: %ld\n", (long)status);
        return;
    }
}