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

//FIXME: зачем этот метод
void recieve_from_others(int receiver, int pipes[][2], int n, int* buffer) {
    for (int i = 0; i < n; i++) {
        if (i != receiver) {  // Не получать от самого себя
            read(pipes[i][0], &buffer[i], sizeof(int));  // Чтение по pipe
            printf("Process %d received message %d from process %d\n", receiver, buffer[i], i);
        }
    }
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