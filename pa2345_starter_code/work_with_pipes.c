#include "ipc.h"
#include "pipes_const.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

int send_multicast(void *__info, const Message *msg) {
    Info *info = (Info *)__info;

    size_t message_size = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    
    local_id iterator = 0;
    while (iterator < info->N) {
        if (iterator != info->fork_id) {
            write(info->pm[info->fork_id][iterator][1], msg, message_size);
        }
        
        iterator++;
    }
    return 0;
}

int send_to_pipe(void *__info, const Message *msg, local_id pipe_id) {
    Info *info = (Info *)__info;

    size_t message_size = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    
    local_id iterator = 0;
    // while (iterator < info->N) {
        // if (iterator != info->fork_id) {
            write(info->pm[info->fork_id][pipe_id][1], msg, message_size);
        // }
        
        // iterator++;
    // }
    return 0;
}

int receive(void *__info, local_id from, Message *msg) {
  Info *info = (Info *)__info;
  int fd = info->pm[from][info->fork_id][0];
  
  if (read(fd, &msg->s_header, sizeof(MessageHeader)) == -1) {
    return -1;
  }
  if (read(fd, &msg->s_payload, msg->s_header.s_payload_len) == -1) {
    return -1;
  }
  return 0;
}

int receive_any(void * self, Message * msg) {
    Info *info = (Info *) self;
    local_id process_id = info->fork_id;
    int not_received = 1;
    
    while(not_received) {
        for (int from = 0; from < info->N; from++) {
            if (from == process_id) continue;
            if (read(pm[from][process_id][0], &msg->s_header, sizeof(MessageHeader)) <= 0) {
                continue;
            } else {
              fprintf(stdout, "Received\n");
              fflush(stdout);
            }
            if (msg->s_header.s_payload_len > 0){
                // do {
                read(pm[from][process_id][0], &msg->s_payload, msg->s_header.s_payload_len);
                              fprintf(stdout, "Received payload\n");
              fflush(stdout);
                not_received = 0;
                return msg->s_header.s_type;
                // } while (nread == -1 || nread == 0);
            }
        }
    }
    return 1;
}
