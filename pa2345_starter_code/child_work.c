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
