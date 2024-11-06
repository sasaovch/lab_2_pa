#include "ipc.h"
#include "pipes_const.h"
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

int init_parent_work(int N) {
    fprintf(elf, log_started_fmt, get_physical_time(), 0, getpid(), getppid(), 0);
    fflush(elf);

    fprintf(stdout, log_started_fmt, get_physical_time(), 0, getpid(), getppid(), 0);
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
                
                if (column != 0 && pm[line][column][0] != -1) {
                    to_close = pm[line][column][0];
                    pm[line][column][0] = -1;
                    close(to_close);                    
                }
                
                if (line != 0 && pm[line][column][1] != -1) {
                    to_close = pm[line][column][1];
                    pm[line][column][1] = -1;
                    close(to_close);
                }
                
                column++;
            }
        }
        line++;
    }
  
    local_id child_i = 1;
    while (child_i < N) {
        Info info = {.fork_id = 0, .N = N};

        for(int i = 0; i < 10; i++) {
            for(int j = 0; j < 10; j++) {
                for(int k = 0; k < 2; k++) {
                    info.pm[i][j][k] = pm[i][j][k];
                }
            }
        }
        Message started_msg;

        receive(&info, child_i, &started_msg);
        if (started_msg.s_header.s_type == STARTED && started_msg.s_header.s_payload_len > 0) {
            child_i++;
        }

        started_msg.s_header.s_payload_len = 0;
        memset(started_msg.s_payload, '\0', sizeof(char)*MAX_PAYLOAD_LEN);
    }

    fprintf(elf, log_received_all_started_fmt, get_physical_time(), 0);
    fflush(elf);

    fprintf(stdout, log_received_all_started_fmt, get_physical_time(), 0);
    fflush(stdout);
    return 0;
}

void do_parent_work(int N) {
   Info parent_info = {.fork_id = 0, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                parent_info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    bank_robbery(&parent_info, N - 1);

    Message stop_msg;
    char message[MAX_PAYLOAD_LEN];
    timestamp_t time = get_physical_time();
    
    sprintf(message, "STOP children\n");
    memset(stop_msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
    memcpy(stop_msg.s_payload, message, sizeof(char)*(MAX_PAYLOAD_LEN));

    stop_msg.s_header.s_magic = MESSAGE_MAGIC;
    stop_msg.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;
    stop_msg.s_header.s_type = STOP;
    stop_msg.s_header.s_local_time = time;

    send_multicast(&parent_info, &stop_msg);

    local_id child_i = 1;
    while (child_i < N) {
        Message receive_msg;
        receive(&parent_info, child_i, &receive_msg);
        if (receive_msg.s_header.s_type == DONE && receive_msg.s_header.s_payload_len > 0) {
            fprintf(elf, "Stop parent message from %d with payload: %s\n", child_i, receive_msg.s_payload);
            fflush(elf);
            child_i++;
        }

        receive_msg.s_header.s_payload_len = 0;
        receive_msg.s_header.s_type = 0;
        memset(receive_msg.s_payload, '\0', sizeof(char)*MAX_PAYLOAD_LEN);
    }

    fprintf(elf, log_received_all_done_fmt, get_physical_time(), 0);
    fflush(elf);

    fprintf(stdout, log_received_all_done_fmt, get_physical_time(), 0);
    fflush(stdout);
}

void print_history_from_all_children(int N) {
    AllHistory a_hs;
    a_hs.s_history_len = N - 1;

    Info parent_info = {.fork_id = 0, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                parent_info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    Message msg_hs;
    local_id child = 1;
    while (child < N) {
        receive(&parent_info, child, &msg_hs);
        
        if (msg_hs.s_header.s_type == BALANCE_HISTORY) {
            BalanceHistory* balance_history = (BalanceHistory*) msg_hs.s_payload;
            fprintf(elf, "Check history: %d\n", balance_history->s_id);
            fflush(elf);
            a_hs.s_history[balance_history->s_id - 1] = *balance_history;
            child++;

            msg_hs.s_header.s_payload_len = 0;
            msg_hs.s_header.s_type = 0;
            memset(msg_hs.s_payload, '\0', sizeof(char)*MAX_PAYLOAD_LEN);
        } else {
            fprintf(elf, "Debug with payload: %s\n", msg_hs.s_payload);
            fflush(elf);
        }
    }
    
    print_history(&a_hs);
}

void parent_are_waiting(int N) {
    local_id child = 1;
    while (child < N) {
        wait(NULL);
        child++;
    }
}
