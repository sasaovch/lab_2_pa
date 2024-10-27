#include "child_work.h"
#include "pipes_const.h"
#include "common.h"
#include "pa2345.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>


//TODO: N - это общее количество
int init_child_work(void* __child_state) {
    ChildState* child_state = (ChildState *) __child_state;
    local_id child_id = child_state->fork_id;
    int N = child_state->N;
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
                }
                
                column++;
                
                if (line != child_id && pm[line][column][1] != -1) {
                    to_close = pm[line][column][1];
                    pm[line][column][1] = -1;
                }
                
                close(to_close);
            }
        }
        line++;
    }

    // char start_message[MAX_PAYLOAD_LEN + 1];
    // memset(start_message, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    // sprintf(log_started_fmt, STARTED, child_id, getpid(), getppid());

    // Message started_msg = new_message_contructor(STARTED);/

    //FIXME: rename
    Message msg;
    char start_message[MAX_PAYLOAD_LEN];
    timestamp_t time = get_physical_time();
    
    sprintf(start_message, log_started_fmt, time, child_id, getpid(), getppid(), child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    memset(msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    memcpy(msg.s_payload, start_message, sizeof(char)*(MAX_PAYLOAD_LEN));

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;
    msg.s_header.s_type = STARTED;
    msg.s_header.s_local_time = time;

    Info info = {.fork_id = child_id, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    //FIXME: remove
    // write_to_event_file_and_stdout(started_msg.s_payload);
    fprintf(elf, "%s", msg.s_payload);
    fflush(elf);

    fprintf(stdout, "%s", msg.s_payload);
    fflush(stdout);

    // fprintf(stdout, "%s", s);
    // fflush(stdout);
    // сделать инлайн
    send_multicast(&info, &msg);
    
        //     receive_from_all_children(&local, &msg, num_children);
        // received_all_started();

//FIXME: change to receive_id
    local_id childs = 1;
    while (childs < N) {
        if (childs == child_id) {
            childs++;
            continue;
        }
        Message msg;
        //FIXME: rename Info
        Info info = {.fork_id = child_id, .N = N};
        
        for(int i = 0; i < 10; i++) {
            for(int j = 0; j < 10; j++) {
                for(int k = 0; k < 2; k++) {
                    info.pm[i][j][k] = pm[i][j][k];
                }
            }
        }

        receive(&info, childs, &msg);
        childs++;
    }

    fprintf(elf, log_received_all_started_fmt, get_physical_time(), child_id);

    return 0;
}

void update_state(ChildState* child_state, int sum, timestamp_t time) {
    // if (time <= MAX_T) {
        int len = child_state->balance_history.s_history_len;
        if (len - time == 1) {
            // current time
            child_state->balance_history.s_history[len - 1].s_balance += sum;
        } else if (len == time) {
            // next time
            balance_t new_balance = child_state->balance_history.s_history[len - 1].s_balance + sum;
            len++;            
            child_state->balance_history.s_history[len - 1] = (BalanceState) {
                .s_balance = new_balance,
                .s_time = time,
                .s_balance_pending_in = 0,
            };
            child_state->balance_history.s_history_len++;
        } else if (time - len > 0) {                   
            balance_t last_balance = child_state->balance_history.s_history[len - 1].s_balance;
            for (int index = len; index < time; index++) {
                child_state->balance_history.s_history[index] = (BalanceState) {
                    .s_balance = last_balance,
                    .s_time = index,
                    .s_balance_pending_in = 0,
                };
            }            
            child_state->balance_history.s_history[time] = (BalanceState) {
                .s_balance = last_balance + sum,
                .s_time = time,
                .s_balance_pending_in = 0,
            };
            child_state->balance_history.s_history_len = time + 1;
        }
    // }    
}

void transfer_handler(void* __child_state, Message* msg) {
    ChildState* child_state = (ChildState *) __child_state;
    local_id child_id = child_state->fork_id;
    int N = child_state->N;

    timestamp_t current_time = get_physical_time();
    TransferOrder *transfer_order = (TransferOrder* ) msg->s_payload;
    printf("Ttransfer_handler__________________________ amount - %d; from - %d; to -%d\n", transfer_order->s_amount, transfer_order->s_src, transfer_order->s_dst);
    if (child_id == transfer_order->s_dst) {    
        printf("Ttransfer_handler_dst id - %d; from - %d; to -%d\n", transfer_order->s_amount, transfer_order->s_src, transfer_order->s_dst);
        update_state(child_state, transfer_order->s_amount, current_time);  

        fprintf(elf, log_transfer_in_fmt, current_time, transfer_order->s_dst, transfer_order->s_amount, transfer_order->s_src);
        fflush(elf);

        fprintf(stdout, log_transfer_in_fmt, current_time, transfer_order->s_dst, transfer_order->s_amount, transfer_order->s_src);
        fflush(stdout);
        
        Message msg = {
            .s_header =
                {
                    .s_magic = MESSAGE_MAGIC,
                    .s_type = ACK,
                    .s_local_time = current_time,
                    .s_payload_len = 0,
                },
        };
        msg.s_header.s_payload_len = strlen(msg.s_payload);

        Info info = {.fork_id = child_id, .N = N};

        for(int i = 0; i < 10; i++) {
            for(int j = 0; j < 10; j++) {
                for(int k = 0; k < 2; k++) {
                    info.pm[i][j][k] = pm[i][j][k];
                }
            }
        }
        send_to_pipe(&info, &msg, 0);                                       
    } else {         
        printf("Ttransfer_handler_src id - %d; from - %d; to -%d\n", transfer_order->s_amount, transfer_order->s_src, transfer_order->s_dst);
        update_state(child_state, -transfer_order->s_amount, current_time); 
        fprintf(
            elf,
            log_transfer_out_fmt, 
            current_time, 
            transfer_order->s_dst, transfer_order->s_amount, transfer_order->s_src);
            fflush(elf);

        fprintf(
            stdout,
            log_transfer_out_fmt, 
            current_time, 
            transfer_order->s_dst, transfer_order->s_amount, transfer_order->s_src);
            fflush(stdout);
        

            Info info = {.fork_id = child_id, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }
        transfer(&info, transfer_order->s_src, transfer_order->s_dst, transfer_order->s_amount);
    }
}

int handle_transfers(void* __child_state) {
    ChildState* child_state = (ChildState *) __child_state;
    local_id child_id = child_state->fork_id;
    int N = child_state->N;
    
    int16_t type = -1;
    Info info = {.fork_id = child_id, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    Message msg;
    while (type != STOP) {
        type = receive_any(&info, &msg);
        fprintf(stdout, "Got message with type: %d", type);
        fflush(stdout);
        switch (type) {
            case TRANSFER:
                transfer_handler(&child_state, &msg);                
                break;
            // case STOP:
            //     send_done_to_all(&local);
            //     done();
            //     break;
            // case DONE:
            //     count_child_proc = count_child_proc - 1;
            //     break;                    
        }
    }
    // type = -1;
    // while(count_child_proc > 0) {
    //     type = receive_any(&local, &msg);                       
    //     switch (type) {
    //         case TRANSFER:
    //             transfer_handler(&local, &msg);                
    //             break;               
    //         case DONE:
    //             count_child_proc = count_child_proc - 1;
    //             break;                    
    //     }
    // }
    // received_all_done(); 
    Message msg;
    char start_message[MAX_PAYLOAD_LEN];
    timestamp_t time = get_physical_time();
    
    sprintf(start_message, log_started_fmt, time, child_id, getpid(), getppid(), child_state->balance_history.s_history[child_state->balance_history.s_history_len - 1].s_balance);
    memset(msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    memcpy(msg.s_payload, start_message, sizeof(char)*(MAX_PAYLOAD_LEN));

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;
    msg.s_header.s_type = DONE;
    msg.s_header.s_local_time = time;

    Info info = {.fork_id = child_id, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    //FIXME: remove
    // write_to_event_file_and_stdout(started_msg.s_payload);
    fprintf(elf, "%s", msg.s_payload);
    fflush(elf);

    fprintf(stdout, "%s", msg.s_payload);
    fflush(stdout);

    // fprintf(stdout, "%s", s);
    // fflush(stdout);
    // сделать инлайн
    send_multicast(&info, &msg);
    
    local_id childs = 1;
    while (childs < N) {
        if (childs == child_id) {
            childs++;
            continue;
        }
        Message msg;
        //FIXME: rename Info
        Info info = {.fork_id = child_id, .N = N};
        
        for(int i = 0; i < 10; i++) {
            for(int j = 0; j < 10; j++) {
                for(int k = 0; k < 2; k++) {
                    info.pm[i][j][k] = pm[i][j][k];
                }
            }
        }

        receive(&info, childs, &msg);
        childs++;
    }

    fprintf(elf, log_received_all_started_fmt, get_physical_time(), child_id);
}