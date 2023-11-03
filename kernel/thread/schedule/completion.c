#include "./completion.h"

#include "../thread.h"

void init_completion(completion *completion) {
    completion->done = 0;
    init_list(&completion->wait);
}

#define SCHEDULE_TIMEOUT_MAX ((long)0 - 1)
long schedule_timeout(signed long timeout) {
    if (timeout == SCHEDULE_TIMEOUT_MAX) {
        if (timeout < 0) {
            return 0;
        }
        return timeout;
    }
    return timeout - 100;
}

typedef struct {
    task_struct *task;
    list_node tag;
} swait_node;

#define __SWAITQUEUE_INIT(name) \
    { .task = cur_thread() }

#define DECLARE_SWAITQUEUE(name) swait_node name = __SWAITQUEUE_INIT(name)

long do_wait_for_common(completion *x, long (*action)(long), long timeout,
                        task_status state) {
    if (!x->done) {
        DECLARE_SWAITQUEUE(wait);
        do {
            wait.task = cur_thread();
            pushr(&x->wait, &wait.tag);
            thread_block(cur_thread(), state);
            timeout = action(timeout);
        } while (!x->done && timeout);
        if (!x->done) {
            return timeout;
        }
    }
    if (x->done != (0 - 1)) {
        x->done--;
    }
    if (timeout == 0) {
        return 1;
    }
    return timeout;
}

void wait_for_completion(completion *completion) {
    do_wait_for_common(completion, schedule_timeout, SCHEDULE_TIMEOUT_MAX,
                       TASK_WAITING);
}

void complete(completion *completion) {
    if (completion->done != (0 - 1)) {
        completion->done++;
    }
    if (completion->wait.size == 0) {
        return;
    }
    list_node *cur = popl(&completion->wait);
    swait_node *wait = tag2entry(swait_node, tag, cur);
    set_thread_status(wait->task, TASK_READY, false);
}

void complete_all(completion *completion) {
    completion->done = (0 - 1);
    if (completion->wait.size == 0) {
        return;
    }
    for (list_node *cur = popl(&completion->wait); cur != NULL;
         cur = popl(&completion->wait)) {
        swait_node *wait = tag2entry(swait_node, tag, cur);
        set_thread_status(wait->task, TASK_READY, false);
    }
}
