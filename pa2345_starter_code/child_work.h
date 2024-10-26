#pragma once
#ifndef  __CHILD_WORK_H
#define  __CHILD_WORK_H

#include "ipc.h"
#include "banking.h"

typedef struct {
    local_id fork_id;
    int N;
    BalanceHistory balance_history;
} ChildState;

int init_child_work(void* __child_state);

#endif