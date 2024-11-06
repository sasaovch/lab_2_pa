#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "banking.h"
#include "ipc.h"
#include "pipes_const.h"
#include "parent_work.h"
#include "child_work.h"
#include "work_with_pipes.h"

// TODO: добавить нулевой пайп для общения с родителем
// TODO: отправлять START только родителю и не ждать от других STRART, сразу получать сообщения

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
    Info *info = (Info *)parent_data;
    TransferOrder s_transfer_order = {
        src,
        dst,
        amount
    };

    // Transfer* transfer = (Transfer*) parent_data;
    // transfer->transfer_order = s_transfer_order;
    
    Message msg_transfer = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type = TRANSFER,
            .s_payload_len = sizeof(TransferOrder),
            .s_local_time = get_physical_time(),
        },
    };

    memcpy(
        &msg_transfer.s_payload, 
        &s_transfer_order,
        sizeof(TransferOrder)
    );

    // struct Info info = {.fork_id = 0, .N = N};

    // for(int i = 0; i < 10; i++) {
    //     for(int j = 0; j < 10; j++) {
    //         for(int k = 0; k < 2; k++) {
    //             info.pm[i][j][k] = pm[i][j][k];
    //         }
    //     }
    // }
    fprintf(stdout, "Info id %d\n", info->fork_id);
    fflush(stdout);

    fprintf(elf, "Transfer amount %d from %d to %d\n", amount, src, dst);
    fflush(elf);

    fprintf(stdout, "Transfer amount %d from %d to %d\n", amount, src, dst);
    fflush(stdout);
    if (info->fork_id == 0) {
        send_to_pipe(info, &msg_transfer, src);
        fprintf(elf, "Wanted to send src to %d\n", src);
        fflush(elf);

    }
    if (info->fork_id == src) {
        send_to_pipe(info, &msg_transfer, dst);
        fprintf(elf, "Wanted to send dst to %d\n", dst);
        fflush(elf);
    }
}

// int pipes[9][9][2];

int is_not_child(int fork_id) {
    return fork_id == 0;
}

int main(int argc, char * argv[])
{
    // if (argc < 3 || strcmp(argv[1], "-p") != 0) {
    //     printf("Usage: %s -p N S S S...\n", argv[0]);
    //     return 1;
    // }

    // Чтение N
    //TODO: N - общее число потоков: родитель + дети
    int N = 
    3;
    // atoi(argv[2]) + 1;
    
    // if (argc != N + 3) { // Проверка, что количество аргументов соответствует N
        // printf("Error: expected %d numbers after N\n", N);
        // return 1;
    // }

    // Создание массива длиной N+1
    int *array = (int *) malloc((N) * sizeof(int));
    // if (array == NULL) {
        // printf("Error: memory allocation failed\n");
        // return 1;
    // }

    // Заполнение массива
    // FIXME: тут определиться нужен ли
    array[0] = N; // Первый элемент - N
    for (int i = 1; i < N; i++) array[i] = 
    i;
    // atoi(argv[i + 2]); // Чтение целых чисел S

    // Вывод массива для проверки
    // printf("Array: ");
    // for (int i = 0; i < N + 1; i++) {
        // printf("%d ", array[i]);
    // }
    // printf("\n");

    // Освобождение памяти
    elf = fopen(events_log, "a");
    plf = fopen(pipes_log, "a");

    fprintf(elf, "-------------------\n");
    fflush(elf);

    local_id line = 0;
    local_id column = 0;

    int all_pipes_number = N;

    while (line < all_pipes_number) {
        column = 0;
        while (column < all_pipes_number) {
            if (line == column) {
                pm[line][column][0] = -1;
                pm[line][column][1] = -1;
                column++;
            } else {
                int descriptors[2];
                pipe(descriptors);

                unsigned int flags_for_read = fcntl(descriptors[0], F_GETFL, 0);
                unsigned int flags_for_write = fcntl(descriptors[1], F_GETFL, 0);
                if (fcntl(descriptors[0], F_SETFL, flags_for_read | O_NONBLOCK) < 0)
                    exit(2);
                if (fcntl(descriptors[1], F_SETFL, flags_for_write | O_NONBLOCK) < 0)
                    exit(2);
                for (int i = 0; i < 2; i++) {
                    pm[line][column][i] = descriptors[i]; // 0 - read and 1 - write
                }

                fprintf(plf, "Pipe %d -> %d. Fd %d -> %d\n", line, column, descriptors[0], descriptors[1]);
                fflush(plf);
                column++;
            }
        }
        line++;
    }

    local_id number_id = 1;
    while (number_id < N) {
        int fork_id = fork();
        if (is_not_child(fork_id)) {
            number_id++;
        } else {
            ChildState child_state = {
                .fork_id = number_id,
                .N = N,
                .balance_history = {
                    .s_id = number_id,
                    .s_history_len = 1,
                    .s_history[0] = {
                        .s_balance = array[number_id],
                        .s_time = 0,
                        .s_balance_pending_in = 0,
                    }
                }
            };
            init_child_work(&child_state);
            handle_transfers(&child_state);
            return 0;
        }
    }
    // start_parent
    init_parent_work(N);
    fprintf(stdout, "Started banck robbery\n");
    fflush(stdout);

    Info info = {.fork_id = 0, .N = N};

    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            for(int k = 0; k < 2; k++) {
                info.pm[i][j][k] = pm[i][j][k];
            }
        }
    }

    // FIXME: понять что нужно
    bank_robbery(&info, N - 1);
            fprintf(elf, "Parent Started receiving message\n");
        fflush(elf);
    Message started_msg;
    // Info info = {.fork_id = 0, .N = N};

    // for(int i = 0; i < 10; i++) {
        // for(int j = 0; j < 10; j++) {
            // for(int k = 0; k < 2; k++) {
                // info.pm[i][j][k] = pm[i][j][k];
            // }
        // }
    // }
    int16_t type = -1;
    while (type != ACK) {
        type = receive_any(&info, &started_msg);
        fprintf(elf, "Parent got message with type: %d\n", type);
        fflush(elf);
                fprintf(elf, "Parent got message with payload: %s\n", started_msg.s_payload);
        fflush(elf);

        if (started_msg.s_header.s_type == ACK) {
        //   child_i++;
          fprintf(elf, "Parent received ACK message\n");
          fflush(elf);
          type = ACK;
        }
    }
//     local_id child_i = 1;
//     while (child_i < N) {


//     receive(&info, 0, &started_msg);
//     // fprintf(elf, "Parent received %d message\n", started_msg.s_header.s_type);
//     // fflush(elf);
//   }
    // print_history(all);
    
    free(array);

    return 0;
}

// мултикаст работает
// отправка потоку тоже
// получение от конктетного тоже