#pragma once
#ifndef  __PARENT_WORK_H
#define  __PARENT_WORK_H

#include "ipc.h"
#include "pipes_const.h"

int init_parent_work(int N);
void do_parent_work(int N);
void print_history_from_all_children(int N);

#endif