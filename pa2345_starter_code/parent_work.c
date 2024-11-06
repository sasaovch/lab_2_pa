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
                }
                
                column++;
                
                if (line != 0 && pm[line][column][1] != -1) {
                    to_close = pm[line][column][1];
                    pm[line][column][1] = -1;
                }
                
                close(to_close);
            }
        }
        line++;
    }

    // fprintf(elf, "%s", msg.s_payload);
    // fflush(elf);
  
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

        // fprintf(elf, "Check payload: %s\n", started_msg.s_payload);
    // fflush(elf);

    receive(&info, child_i, &started_msg);
    if (started_msg.s_header.s_type == STARTED && started_msg.s_header.s_payload_len > 0) {
    fprintf(elf, "Init parent message from %d with payload: %s\n", child_i, started_msg.s_payload);
    fflush(elf);
      child_i++;
    }

    started_msg.s_header.s_payload_len = 0;
    memset(started_msg.s_payload, '\0', sizeof(char)*MAX_PAYLOAD_LEN);
  }

  fprintf(elf, log_received_all_started_fmt, get_physical_time(), 0);
  fflush(elf);
  return 0;
  
//   if (receive_from_children(number_of_theads, DONE)) {
//       fprintf(stdout, "Error while receiving DONE\n");
//   }
  
//   wait_for_children(number_of_theads);
  
//   close_all_pipes(number_of_theads);
//   close_files();
  
//   return 0;
}