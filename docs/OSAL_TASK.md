# OSAL Task API

Tài liệu này mô tả kỹ thuật các API task trong [osapi-task.h](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/inc/osapi-task.h).

## 1. Task API trong OSAL dùng để làm gì

Task API là lớp trừu tượng để tạo và điều khiển luồng thực thi song song theo cách portable giữa nhiều hệ điều hành.

Application dùng Task API khi cần:
- chạy nhiều công việc đồng thời
- tách một công việc nền khỏi luồng chính
- đồng bộ với queue, semaphore, mutex, timer
- tra cứu và quản lý vòng đời task theo `osal_id_t`

OSAL che phần khác biệt giữa các backend như POSIX, RTEMS, VxWorks. Application chỉ làm việc với API chung thay vì gọi trực tiếp `pthread_create()`, `taskSpawn()`, `rtems_task_create()`...

## 2. Các khái niệm chính

### 2.1 `osal_id_t`

Đây là ID logic của object trong OSAL. Với task, `osal_id_t` là cách OSAL nhận diện một task đã tạo.

Application nên dùng `osal_id_t` để:
- lưu ID task
- xóa task
- đổi priority
- truy vấn thông tin task

### 2.2 `osal_task_entry`

Đây là kiểu hàm entry của task:

```c
typedef osal_task((*osal_task_entry)(void));
```

Task entry trong OSAL không nhận tham số và không trả giá trị.

Ví dụ:

```c
void worker_task(void)
{
    while (1)
    {
        OS_TaskDelay(1000);
    }
}
```

### 2.3 Priority của OSAL

OSAL dùng priority ngược kiểu VxWorks:
- `0` là cao nhất
- `255` là thấp nhất

Nghĩa là số càng nhỏ thì task càng quan trọng.

### 2.4 Stack pointer và stack size

Khi tạo task, có 2 cách cấp stack:
- đưa stack buffer của riêng application
- để OSAL tự cấp bằng `OSAL_TASK_STACK_ALLOCATE`

Khuyến nghị thực tế:
- ưu tiên `OSAL_TASK_STACK_ALLOCATE`
- chỉ tự cấp stack khi thật sự cần kiểm soát thủ công

Trên POSIX, nếu application tự truyền stack buffer thì application phải tự chịu phần overhead native thread. Đây là lý do nhiều ví dụ chạy ổn hơn khi dùng `OSAL_TASK_STACK_ALLOCATE`.

## 3. Tổng quan vòng đời task

Luồng điển hình:

1. `OS_TaskCreate()` tạo task và trả về `osal_id_t`
2. task bắt đầu chạy entry function
3. task thực hiện công việc, có thể delay, đồng bộ, gửi nhận queue
4. task kết thúc theo một trong hai cách:
   - tự thoát bằng `OS_TaskExit()` hoặc return khỏi entry
   - bị task khác xóa bằng `OS_TaskDelete()`

Điểm quan trọng:
- `OS_TaskExit()` là self-exit
- `OS_TaskDelete()` là external delete
- delete handler chỉ đi với đường `OS_TaskDelete()` thành công, không đi với self-exit

## 4. Mô tả chi tiết từng API

### 4.1 `OS_TaskCreate()`

Prototype:

```c
int32 OS_TaskCreate(osal_id_t      *task_id,
                    const char     *task_name,
                    osal_task_entry function_pointer,
                    osal_stackptr_t stack_pointer,
                    size_t          stack_size,
                    osal_priority_t priority,
                    uint32          flags);
```

API này dùng để tạo và start một task mới.

Khi nào dùng:
- cần chạy một worker task
- cần tạo task nền để xử lý queue/event
- cần chia nhỏ công việc song song

Tại sao cần API này:
- application không cần biết backend đang là POSIX, RTEMS hay VxWorks
- OSAL trả về `osal_id_t` để các API khác dùng tiếp
- OSAL quản lý tên task, stack size, priority, object table

Behavior chính ở shared layer:
- kiểm tra tham số
- cấp một object ID mới
- ghi thông tin task vào bảng OSAL
- gọi backend implementation để tạo native thread/task
- finalize object nếu tạo thành công

Implementation nằm ở [osapi-task.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/shared/src/osapi-task.c:164).

Ví dụ:

```c
osal_id_t worker_id;
int32 status;

status = OS_TaskCreate(&worker_id,
                       "Worker",
                       worker_task,
                       OSAL_TASK_STACK_ALLOCATE,
                       4096,
                       OSAL_PRIORITY_C(100),
                       0);
```

Ví dụ thực tế trong repo: [tasking-example.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/examples/tasking-example/tasking-example.c:138)

Lưu ý:
- tên task phải unique
- `stack_size` phải khác `0`
- ưu tiên `OSAL_TASK_STACK_ALLOCATE`

### 4.2 `OS_TaskDelete()`

Prototype:

```c
int32 OS_TaskDelete(osal_id_t task_id);
```

API này dùng để xóa một task từ bên ngoài.

Khi nào dùng:
- supervisor task muốn dừng worker task
- test code cần cleanup task đã tạo
- application reload/shutdown cần chắc rằng task đã dừng

Tại sao cần API này:
- một task đôi khi không tự kết thúc được đúng lúc
- caller cần chủ động stop task khác bằng `osal_id_t`

Behavior chính:
- lookup `task_id`
- lấy delete hook đã cài nếu có
- gọi backend delete implementation
- xóa object task khỏi bảng OSAL
- nếu delete thành công và có hook thì gọi hook

Implementation nằm ở [osapi-task.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/shared/src/osapi-task.c:219).

Ví dụ:

```c
int32 status;

status = OS_TaskDelete(worker_id);
if (status != OS_SUCCESS)
{
    OS_printf("Delete task failed: %ld\n", (long)status);
}
```

Ví dụ thực tế trong repo: [tasking-example.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/examples/tasking-example/tasking-example.c:346)

Lưu ý kỹ thuật:
- trên POSIX, delete thường là cooperative cancel, không phải force kill tuyệt đối
- task đích cần ở trạng thái có thể bị dừng đúng cách
- nếu task đã tự exit trước đó thì `OS_TaskDelete()` có thể trả `OS_ERR_INVALID_ID`

### 4.3 `OS_TaskExit()`

Prototype:

```c
void OS_TaskExit(void);
```

API này dùng để chính task hiện tại tự kết thúc.

Khi nào dùng:
- task nhận stop command và muốn thoát sạch
- task chạy xong một việc rồi tự kết thúc
- self-managed worker cần shutdown chủ động

Tại sao cần API này:
- task biết rõ nhất khi nào nó nên kết thúc
- thường an toàn hơn external delete
- phù hợp với mô hình queue/cờ stop/semaphore để task tự shutdown

Behavior chính:
- lấy `task_id` hiện tại
- detach native task nếu cần
- xóa object task khỏi bảng OSAL
- gọi backend exit implementation

Implementation nằm ở [osapi-task.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/shared/src/osapi-task.c:258).

Ví dụ:

```c
void worker_task(void)
{
    while (!stop_requested)
    {
        OS_TaskDelay(100);
    }

    OS_TaskExit();
}
```

Điểm rất quan trọng:
- `OS_TaskExit()` không gọi delete handler
- nó chỉ kết thúc task hiện tại
- các queue/mutex/semaphore do task tạo ra không tự bị xóa chỉ vì task đã exit

### 4.4 `OS_TaskInstallDeleteHandler()`

Prototype:

```c
int32 OS_TaskInstallDeleteHandler(osal_task_entry function_pointer);
```

API này dùng để đăng ký một callback cleanup cho task hiện tại khi task đó bị xóa bằng `OS_TaskDelete()`.

Khi nào dùng:
- task tự tạo thêm resource riêng và muốn cleanup khi bị xóa từ bên ngoài
- muốn log lại sự kiện task bị delete
- muốn giải phóng object phụ trước khi task biến mất

Tại sao cần API này:
- `OS_TaskDelete()` có thể được gọi bởi task khác
- task bị xóa có thể cần một chỗ cleanup logic riêng

Behavior chính:
- lấy `task_id` của task hiện tại
- lưu function pointer vào `delete_hook_pointer` của task đó

Implementation nằm ở [osapi-task.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/shared/src/osapi-task.c:403).

Ví dụ:

```c
static void cleanup_hook(void)
{
    OS_printf("Worker delete hook invoked\n");
}

void worker_task(void)
{
    OS_TaskInstallDeleteHandler(cleanup_hook);

    while (1)
    {
        OS_TaskDelay(1000);
    }
}
```

Ví dụ thực tế trong repo: [tasking-example.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/examples/tasking-example/tasking-example.c:186)

Lưu ý rất quan trọng:
- hook chỉ được gọi khi `OS_TaskDelete()` thành công
- hook không chạy khi task tự `OS_TaskExit()`
- hook không nhận tham số
- mỗi task chỉ giữ một hook; cài lần sau sẽ ghi đè lần trước

### 4.5 `OS_TaskDelay()`

Prototype:

```c
int32 OS_TaskDelay(uint32 millisecond);
```

API này dùng để cho task ngủ trong một khoảng thời gian.

Khi nào dùng:
- polling định kỳ
- loop chờ mềm
- tạo nhịp cho worker task

Tại sao cần API này:
- tránh busy loop làm tốn CPU
- portable giữa các backend

Ví dụ:

```c
while (1)
{
    do_periodic_work();
    OS_TaskDelay(1000);
}
```

### 4.6 `OS_TaskSetPriority()`

Prototype:

```c
int32 OS_TaskSetPriority(osal_id_t task_id, osal_priority_t new_priority);
```

API này dùng để đổi priority của một task đã tồn tại.

Khi nào dùng:
- cần tăng ưu tiên cho task xử lý gấp
- cần hạ ưu tiên task nền
- test scheduler behavior

Tại sao cần API này:
- priority có thể cần thay đổi theo runtime state
- backend native priority khác nhau, OSAL lo phần mapping

Behavior chính:
- lookup task theo `task_id`
- gọi backend implementation để đổi native priority
- nếu thành công thì cập nhật priority trong bảng OSAL

Implementation nằm ở [osapi-task.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/shared/src/osapi-task.c:296).

Ví dụ:

```c
OS_TaskSetPriority(worker_id, OSAL_PRIORITY_C(50));
```

### 4.7 `OS_TaskGetId()`

Prototype:

```c
osal_id_t OS_TaskGetId(void);
```

API này dùng để lấy `osal_id_t` của task hiện tại.

Khi nào dùng:
- task muốn biết chính mình là ai
- log/debug
- cất ID của chính task hiện tại sang nơi khác

Tại sao cần API này:
- caller không cần biết native thread ID
- chỉ cần một ID portable của OSAL

Ví dụ:

```c
osal_id_t self_id;

self_id = OS_TaskGetId();
OS_printf("Current task id = %lu\n", OS_ObjectIdToInteger(self_id));
```

Lưu ý:
- API này chỉ trả ID của task đang gọi nó
- nó không dùng để hỏi ID của task khác

### 4.8 `OS_TaskGetIdByName()`

Prototype:

```c
int32 OS_TaskGetIdByName(osal_id_t *task_id, const char *task_name);
```

API này dùng để tìm ID task theo tên.

Khi nào dùng:
- đã biết tên task, cần lấy ID để thao tác tiếp
- startup logic muốn nối vào task đã có sẵn
- debug và quản lý object theo tên

Tại sao cần API này:
- nhiều khi chỉ biết tên task do thiết kế hệ thống đặt sẵn
- không có sẵn `osal_id_t` tại chỗ đang chạy

Ví dụ:

```c
osal_id_t task_id;
int32 status;

status = OS_TaskGetIdByName(&task_id, "Worker");
```

Lưu ý:
- chỉ dùng được khi biết đúng tên task
- tên phải đúng với lúc `OS_TaskCreate()`

### 4.9 `OS_TaskGetInfo()`

Prototype:

```c
int32 OS_TaskGetInfo(osal_id_t task_id, OS_task_prop_t *task_prop);
```

API này dùng để lấy metadata của task.

Thông tin trả về trong `OS_task_prop_t` gồm:
- tên task
- creator
- stack size
- priority

Khi nào dùng:
- debug
- log
- kiểm tra cấu hình task runtime

Tại sao cần API này:
- caller cần biết task được tạo với thông số gì
- tránh phải giữ riêng các thông tin này ở application

Ví dụ:

```c
OS_task_prop_t info;
int32 status;

status = OS_TaskGetInfo(worker_id, &info);
if (status == OS_SUCCESS)
{
    OS_printf("Task %s priority=%u stack=%lu\n",
              info.name,
              (unsigned int)info.priority,
              (unsigned long)info.stack_size);
}
```

### 4.10 `OS_TaskFindIdBySystemData()`

Prototype:

```c
int32 OS_TaskFindIdBySystemData(osal_id_t *task_id, const void *sysdata, size_t sysdata_size);
```

API này dùng để tra ngược từ dữ liệu task do hệ điều hành cung cấp sang `osal_id_t`.

Khi nào dùng:
- callback thấp tầng của OS/BSP chỉ đưa ra native task handle
- exception handler/signal handler/debug hook cần biết native thread đó tương ứng task OSAL nào
- code hiện tại không có tên task và cũng không đang chạy trong chính task cần tra cứu

Tại sao cần API này:
- `OS_TaskGetId()` chỉ biết task hiện tại
- `OS_TaskGetIdByName()` yêu cầu phải biết tên task
- có những trường hợp chỉ có native handle, ví dụ `pthread_t`

Behavior chính:
- validate dữ liệu native theo backend
- search trong bảng task OSAL
- nếu tìm thấy thì trả `osal_id_t`

Implementation nằm ở [osapi-task.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/os/shared/src/osapi-task.c:433).

Ví dụ minh họa trên POSIX:

```c
pthread_t native_task;
osal_id_t found_task_id;
int32 status;

native_task = pthread_self();
status = OS_TaskFindIdBySystemData(&found_task_id, &native_task, sizeof(native_task));
```

Ví dụ thực tế trong repo: [tasking-example.c](/home/quangtv/Workspace/SDK/Sources/Platform/osal/src/examples/tasking-example/tasking-example.c:194)

Điểm cần hiểu rõ:
- API này không đăng ký một native thread mới vào OSAL
- nó chỉ tra cứu task mà OSAL đã quản lý sẵn
- nếu app tự tạo thread ngoài OSAL thì thường sẽ không tra ra được `osal_id_t`

## 5. Chọn API nào trong tình huống nào

### Muốn tạo task mới

Dùng:
- `OS_TaskCreate()`

### Muốn task tự kết thúc sạch

Dùng:
- queue/cờ/semaphore để báo dừng
- `OS_TaskExit()`

### Muốn task khác xóa task hiện có

Dùng:
- `OS_TaskDelete()`

### Muốn cleanup khi task bị xóa từ bên ngoài

Dùng:
- `OS_TaskInstallDeleteHandler()`

### Muốn biết ID của chính task đang chạy

Dùng:
- `OS_TaskGetId()`

### Muốn tìm task theo tên

Dùng:
- `OS_TaskGetIdByName()`

### Muốn lấy thông tin cấu hình của task

Dùng:
- `OS_TaskGetInfo()`

### Chỉ có native thread handle do OS đưa ra

Dùng:
- `OS_TaskFindIdBySystemData()`

## 6. Khuyến nghị sử dụng

- Ưu tiên `OSAL_TASK_STACK_ALLOCATE` thay vì tự truyền stack buffer.
- Ưu tiên mô hình task tự shutdown bằng queue/cờ rồi `OS_TaskExit()`.
- Chỉ dùng `OS_TaskDelete()` khi thật sự cần external delete.
- Không giả định delete handler sẽ chạy khi task tự exit.
- Không giả định `OS_TaskFindIdBySystemData()` sẽ nhận ra thread do app tự tạo ngoài OSAL.

## 7. Ví dụ tổng hợp ngắn

```c
static osal_id_t worker_id;
static volatile bool stop_requested;

static void worker_delete_hook(void)
{
    OS_printf("Worker deleted by external caller\n");
}

void worker_task(void)
{
    OS_TaskInstallDeleteHandler(worker_delete_hook);

    while (!stop_requested)
    {
        OS_TaskDelay(100);
    }

    OS_TaskExit();
}

void start_worker(void)
{
    OS_TaskCreate(&worker_id,
                  "Worker",
                  worker_task,
                  OSAL_TASK_STACK_ALLOCATE,
                  4096,
                  OSAL_PRIORITY_C(100),
                  0);
}

void stop_worker_clean(void)
{
    stop_requested = true;
}

void stop_worker_force(void)
{
    OS_TaskDelete(worker_id);
}
```

Ý nghĩa:
- `start_worker()` tạo task
- `stop_worker_clean()` để task tự thoát
- `stop_worker_force()` xóa task từ bên ngoài
- delete hook chỉ có ý nghĩa với đường force delete thành công
