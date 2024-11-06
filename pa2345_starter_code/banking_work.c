#include "banking.h"
#include "common.h"
#include "banking.h"
#include "pipes_const.h"
#include "work_with_pipes.h"

#include <string.h>

void transfer(void *data, local_id src, local_id dst, balance_t amount) {
    TransferOrder order = {
        src,
        dst,
        amount
    };

    
    Message msg = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type = TRANSFER,
            .s_payload_len = sizeof(TransferOrder),
            .s_local_time = get_physical_time(),
        },
    };

    memcpy(
        &msg.s_payload, 
        &order,
        sizeof(TransferOrder)
    );

    Info *info = (Info *)data;
    if (info->fork_id == 0) {
        send_to_pipe(info, &msg, src);
    }
    if (info->fork_id == src) {
        send_to_pipe(info, &msg, dst);
    }
}
