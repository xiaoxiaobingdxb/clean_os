#if !defined(SCHEDULE_SEGMENT_H)
#define SCHEDULE_SEGMENT_H

#include "common/types/basic.h"
#include "common/lib/list.h"
#include "../type.h"

typedef struct {
    size_t count;
    list waiters;
} segment_t;

void init_segment(segment_t*, size_t);
void segment_wait(segment_t*, pid_t *pid);
void segment_wakeup(segment_t*, pid_t *pid);
size_t segment_count(segment_t*);

#endif // SCHEDULE_SEGMENT_H
