#include "banking.h"
#include "io.h"
#include "ipc.h"


/** Transfer amount from src to dst.
 *
 * @param parent_data Any data structure implemented by students to perform I/O
 */
void transfer(void * parent_data, local_id src, local_id dst, balance_t amount)
{
    if (parent_data == NULL)
        return;
    IO* io = (IO*) parent_data;
    if (src <= 0 || dst <= 0 || src > io->proc_number || dst > proc_number)
        return;
    if (src == dst)
        return;

    // init TransferOrder
    TransferOrder transfer = (TransferOrder) {
        .s_src = src,
        .s_dst = dst,
        s_amount = amount
    }

    // init msg
    Message msg = {{0}};
    msg.s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_payload_len = sizeof(transfer),
        .s_type = TRANSFER,
        .s_local_time = get_physical_time()
    }
    size_t payload_len = sizeof(TransferOrder);
    char* ret = memcpy(msg.s_payload, &transfer, payload_len);
#ifdef DEBUG
    if (ret == NULL) {
        fprintf(stderr, "memcpy error in transfer of parent! from %d to %d.\n",
            src, dst);
    }
#endif

    send((void*)io, src, (const Message *)&msg);

    // wait for ACK
    Message ret_msg = {{0}};
    while(1) {
        res = receive((void*)io, dst, &ret_msg);
        if (res != 0)
            continue;
        if (ret_msg->s_header.s_type == ACK) {
            fprintf(stderr, "%d: process 0 received ACK from %d.\n",
                get_physical_time(), dst);
            break;
        } else {
            // do whatever is needed if received message isn't ACK
        }
    }
}
