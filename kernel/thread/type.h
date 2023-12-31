#ifndef THREAD_TYPE_H
#define THREAD_TYPE_H

#include "common/types/basic.h"
typedef uint32_t pid_t;
typedef enum {
    TASK_UNKNOWN,
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status;
#define TASK_NAME_LEN 16
#endif