#include "banking.h"
#include "common.h"
#include "banking.h"
#include "pipes_const.h"
#include "work_with_pipes.h"

#include <string.h>

void transfer(void *data, local_id src, local_id dst, balance_t amount) {
    TransferOrder order;
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount;
    
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = sizeof(TransferOrder);
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_physical_time();

    memcpy(&msg.s_payload, &order,sizeof(TransferOrder));

    Info *info = (Info *) data;
    if (info->fork_id == 0) {
        Message received_message;
        int16_t type = -1;
        Info child_info = {.fork_id = 0, .N = info->N};
        for(int i = 0; i < 10; i++) {
            for(int j = 0; j < 10; j++) {
                for(int k = 0; k < 2; k++) {
                    child_info.pm[i][j][k] = pm[i][j][k];
                }
            }
        }

        send_to_pipe(info, &msg, src);
        type = receive_any(&child_info, &received_message);
            
        fprintf(elf, "Parent got message with type: %d\n", type);
        fflush(elf);
        fprintf(elf, "Parent got message with payload: %s\n", received_message.s_payload);
        fflush(elf);

        if (received_message.s_header.s_type == ACK) {
            fprintf(elf, "Parent received ACK message\n");
            fflush(elf);
            type = ACK;
            return;
        }
    }
    if (info->fork_id == src) {
        send_to_pipe(info, &msg, dst);
    }
}
