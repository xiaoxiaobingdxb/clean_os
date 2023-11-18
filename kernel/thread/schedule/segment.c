#include "segment.h"

#include "../thread.h"
#include "common/cpu/contrl.h"
#include "common/tool/lib.h"

void init_segment(segment_t *seg, size_t count) {
    seg->count = count;
    init_list(&seg->waiters);
}

#include "common/tool/log.h"
void segment_wait(segment_t *seg, pid_t *pid) {
    eflags_t state = enter_intr_protect();
    task_struct *cur = cur_thread();
    ASSERT(cur != NULL);
    if (cur->pid == 0) {
        if (cur == NULL) {
            ASSERT(false);
        }
    }
    *pid = cur->pid;
    if (seg->count > 0) {
        seg->count--;
    } else {
        pushr(&seg->waiters, &cur->waiter_tag);
        set_thread_status(cur, TASK_BLOCKED, true);
    }
    leave_intr_protect(state);
}

bool find_by_pid_visitor(list_node *node, void *arg) {
    task_struct *task = tag2entry(task_struct, waiter_tag, node);
    void **args = (void **)arg;
    pid_t *pid_ptr = (pid_t *)args[0];
    if (task != NULL && task->pid == *pid_ptr) {
        args[1] = (void *)node;
        return false;
    }
    return true;
}
void segment_wakeup(segment_t *seg, pid_t *pid) {
    eflags_t state = enter_intr_protect();
    list_node *tag;
    if (*pid != 0) {
        void *args[2] = {(void *)pid, NULL};
        foreach (&seg->waiters, find_by_pid_visitor, (void *)args)
            ;
        tag = args[1];
    } else {
        tag = popl(&seg->waiters);
    }
    if (tag != NULL) {
        task_struct *task = tag2entry(task_struct, waiter_tag, tag);
        ASSERT(task != NULL);
        *pid = task->pid;
        thread_ready(task, false, true); // dispatch firstly
        // set_thread_status(task, TASK_READY, false);
    } else {
        seg->count++;
    }
    leave_intr_protect(state);
}
size_t segment_count(segment_t *seg) {
    eflags_t state = enter_intr_protect();
    size_t count = seg->count;
    leave_intr_protect(state);
    return count;
}