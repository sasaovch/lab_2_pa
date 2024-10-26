#include "pipes_const.h"
#include "ipc.h"
#include "pa2345.h"
#include "banking.h"

// Message new_message_contructor(MessageType msg_type) {
//   Message msg;
//   char start_message[MAX_PAYLOAD_LEN + 1];
  
//   sprintf(start_message, log_started_fmt, get_physical_time(), , getpid(), getppid());
//   memset(msg.s_payload, '\0', sizeof(char)*(MAX_PAYLOAD_LEN));
//   memcpy(msg.s_payload, start_message, strlen(start_message));
//   msg.s_header.s_payload_len = MAX_PAYLOAD_LEN + 1;

//   msg.s_header.s_type = msg_type;
//   msg.s_header.s_magic = MESSAGE_MAGIC;

//   return msg;
// }