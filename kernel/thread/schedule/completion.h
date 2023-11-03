#ifndef SCHEDULE_COMPLETION_H
#define SCHEDULE_COMPLETION_H
#include "common/lib/list.h"
typedef struct completion_t {
    unsigned int done;
    list wait;
} completion;

void init_completion(completion*);

/**
 * @brief decleare and init a completion easily
*/
#define DECLARE_COMPLETION(var_name) \
    completion var_name;              \
    init_completion(&var_name);

/**
 * @brief add a waiter
*/
void wait_for_completion(completion*);

/**
 * @brief wake up a waiter
*/
void complete(completion*);
/**
 * @brief wake up all waiters
*/
void complete_all(completion*);

#endif