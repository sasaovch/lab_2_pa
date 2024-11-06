#include "child_work.h"
#include "ipc.h"
#include "pipes_const.h"
#include "work_with_pipes.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>


int init_child_work(void* __child_state) {

    ChildState* child_state = (ChildState *) __child_state;
    local_id child_id = child_state->fork_id;
    int N = child_state->N;

    Info child_info = {.fork_id = child_id, .N = N};
    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                child_info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    timestamp_t time_started = get_physical_time();
    fprintf(elf, log_started_fmt, time_started, child_id, getpid(), getppid(), child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    fflush(elf);

    fprintf(stdout, log_started_fmt, time_started, child_id, getpid(), getppid(), child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    fflush(stdout);

    local_id line = 0;
    local_id column = 0;

    while (line < N) {
        column = 0;
        while (column < N) {
            if (line == column) {
                column++;
            } else {
                int to_close;
                
                if (column != child_id && pm[line][column][0] != -1) {
                    to_close = pm[line][column][0];
                    pm[line][column][0] = -1;
                    close(to_close);                    
                }
                
                if (line != child_id && pm[line][column][1] != -1) {
                    to_close = pm[line][column][1];
                    pm[line][column][1] = -1;
                    close(to_close);
                }
                
                column++;
            }
        }
        line++;
    }

    Message start_msg;
    char start_message[MAX_PAYLOAD_LEN];
    
    sprintf(start_message, log_started_fmt, time_started, child_id, getpid(), getppid(), child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    memset(start_msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    memcpy(start_msg.s_payload, start_message, sizeof(char)*(MAX_PAYLOAD_LEN));

    start_msg.s_header.s_magic = MESSAGE_MAGIC;
    start_msg.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;
    start_msg.s_header.s_type = STARTED;
    start_msg.s_header.s_local_time = time_started;

    send_multicast(&child_info, &start_msg);

    local_id childs = 1;
    while (childs < N) {
        if (childs == child_id) {
            childs++;
            continue;
        }
        Message msg;

        receive(&child_info, childs, &msg);
        if (msg.s_header.s_type == STARTED && msg.s_header.s_payload_len > 0) {
            childs++;
        }
    }

    timestamp_t all_time = get_physical_time();
    fprintf(elf, log_received_all_started_fmt, all_time, child_id);
    fflush(elf);

    fprintf(stdout, log_received_all_started_fmt, all_time, child_id);
    fflush(stdout);

    return 0;
}

void update_state(ChildState* child_state, int sum, timestamp_t current_time) {
    int balance_history_len = child_state->balance_history.s_history_len;

    if (current_time - balance_history_len > 0) {
        balance_t past_balance = child_state->balance_history.s_history[balance_history_len - 1].s_balance;

        int index = balance_history_len;
        while (index < current_time) {
            child_state->balance_history.s_history[index] = (BalanceState) {
                .s_balance = past_balance,
                .s_time = index,
                .s_balance_pending_in = 0,
            };
            
            index++;
        }

        child_state->balance_history.s_history[current_time] = (BalanceState) {
            .s_balance = past_balance + sum,
            .s_time = current_time,
            .s_balance_pending_in = 0,
        };

        child_state->balance_history.s_history_len = current_time + 1;        
    } else if (balance_history_len == current_time) {     
        child_state->balance_history.s_history[balance_history_len] = (BalanceState) {
            .s_balance = child_state->balance_history.s_history[balance_history_len - 1].s_balance + sum,
            .s_time = current_time,
            .s_balance_pending_in = 0,
        };
        
        child_state->balance_history.s_history_len++;
    } else if (balance_history_len - current_time == 1) {
        child_state->balance_history.s_history[balance_history_len - 1].s_balance += sum;

    }  
}

void transfer_handler(void* __child_state, Message* msg) {
    ChildState* child_state = (ChildState *) __child_state;
    local_id child_id = child_state->fork_id;
    int N = child_state->N;

    Info child_info = {.fork_id = child_id, .N = N};
    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                child_info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    timestamp_t current_time = get_physical_time();
    TransferOrder *order = (TransferOrder* ) msg->s_payload;

    if (child_id == order->s_dst) {    
        fprintf(elf, log_transfer_in_fmt, current_time, order->s_dst, order->s_amount, order->s_src);
        fflush(elf);

        fprintf(stdout, log_transfer_in_fmt, current_time, order->s_dst, order->s_amount, order->s_src);
        fflush(stdout);

        Message msg_n;
        msg_n.s_header.s_magic = MESSAGE_MAGIC;
        msg_n.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;
        msg_n.s_header.s_type = ACK;
        msg_n.s_header.s_local_time = current_time;
        
        update_state(child_state, order->s_amount, current_time);  
        send_to_pipe(&child_info, &msg_n, 0);     
    
    } else {         
        fprintf(elf,log_transfer_out_fmt, current_time, order->s_src, order->s_amount, order->s_dst);
        fflush(elf);

        fprintf(stdout,log_transfer_out_fmt, current_time, order->s_src, order->s_amount, order->s_dst);
        fflush(stdout);

        update_state(child_state, -order->s_amount, current_time); 
        transfer(&child_info, order->s_src, order->s_dst, order->s_amount);
    }
}

int handle_transfers(void* __child_state) {
    ChildState* child_state = (ChildState *) __child_state;
    local_id child_id = child_state->fork_id;
    int N = child_state->N;
    
    Info child_info = {.fork_id = child_id, .N = N};
    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                child_info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }


    Message msg_r;
    int16_t type = -1;

    int wait_for_others_to_stop = N - 2;
    while (type != STOP) {
        msg_r.s_header.s_payload_len = 0;
        memset(msg_r.s_payload, '\0', sizeof(char)*MAX_PAYLOAD_LEN);
        type = receive_any(&child_info, &msg_r);
        if (type == TRANSFER) {
            transfer_handler(child_state, &msg_r);
        } else if (type == DONE) {
            wait_for_others_to_stop--;
        }
    }

    Message done_msg;
    char done_message[MAX_PAYLOAD_LEN];
    timestamp_t time = get_physical_time();

    fprintf(elf, log_done_fmt, time, child_id, child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    fflush(elf);

    fprintf(stdout, log_done_fmt, time, child_id, child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    fflush(stdout);
    
    sprintf(done_message, log_done_fmt, time, child_id, child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    memset(done_msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    memcpy(done_msg.s_payload, done_message, sizeof(char)*(MAX_PAYLOAD_LEN));

    done_msg.s_header.s_magic = MESSAGE_MAGIC;
    done_msg.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;
    done_msg.s_header.s_type = DONE;
    done_msg.s_header.s_local_time = time;

    send_multicast(&child_info, &done_msg);
    
    Message msg_d;
    while (wait_for_others_to_stop > 0) {
        msg_d.s_header.s_payload_len = 0;
        memset(msg_d.s_payload, '\0', sizeof(char)*MAX_PAYLOAD_LEN);
        type = receive_any(&child_info, &msg_d);
        if (type == DONE) {
            wait_for_others_to_stop--;
        }
    }

    fprintf(elf, log_received_all_done_fmt, get_physical_time(), child_id);
    fflush(elf);

    fprintf(stdout, log_received_all_done_fmt, get_physical_time(), child_id);
    fflush(stdout);

    Message history_msg;
    timestamp_t history_time = get_physical_time();
    update_state(child_state, 0, history_time);
    
    memset(history_msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    memcpy(history_msg.s_payload, &(child_state->balance_history), sizeof(BalanceHistory));

    history_msg.s_header.s_magic = MESSAGE_MAGIC;
    history_msg.s_header.s_payload_len = sizeof(BalanceHistory);
    history_msg.s_header.s_type = BALANCE_HISTORY;
    history_msg.s_header.s_local_time = history_time;

    send_to_pipe(&child_info, &history_msg, 0);
    return 0;
}
