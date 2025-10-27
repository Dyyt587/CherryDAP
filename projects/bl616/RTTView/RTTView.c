// #include "pikaScript.h"
#include "swd_host.h"
#include "SEGGER_RTTView.h"
#include "tinyprintf.h"
#include "chry_ringbuffer.h"
#include "freertos.h"
static uint32_t segger_rtt_addr = 0;

#define RTT_MAX_BUFFER_SIZE 4096

__attribute__((aligned(4))) uint8_t rtt_buffer[RTT_MAX_BUFFER_SIZE];

rtt_data_msg_t tRTTMsgObj;

static SEGGER_RTT_CB _SEGGER_RTT;
static uint32_t RTT_wAddr, RTT_wSize = 0x1000, RTT_wChannel;

// static byte_queue_t       s_tByteQueue;
// static fsm(check_string)  s_fsmCheckStr;
// static get_byte_t         s_tGetByte;

// static uint8_t            s_chByteBuf[64];

void RTTView_init(uint32_t wAddr, uint32_t wSize);
void RTTView_Uninit(void);

void read_rtt_and_send_usb(void)
{
    SEGGER_RTT_BUFFER_UP up_buffer;
    uint32_t len = 0;
    static uint16_t read_delay_ms = 1;
    static uint8_t read_err_count = 0;
    static int64_t expected_ticks = 0;

    if (read_err_count > 2 || segger_rtt_addr == 0) {
        read_err_count = 0;
        if (!swd_init_debug()) {
            tfp_printf("swd init error\r\n");
            printf("swd init error\r\n");
            return;
        }
        //RTTView_init(RTT_wAddr, RTT_wSize);
    }
    if (segger_rtt_addr != 0) {
        if ((bflb_mtimer_get_time_us() / 1000) > expected_ticks) {
            do {
                uint32_t up_addr = segger_rtt_addr + offsetof(SEGGER_RTT_CB, aUp[RTT_wChannel]);

                // 读取 RTT UpBuffer 结构
                if (!swd_read_memory(up_addr, (uint8_t *)&up_buffer, sizeof(SEGGER_RTT_BUFFER_UP))) {
                    read_delay_ms = 1000;
                    read_err_count++;
                    break;
                }

                uint32_t buffer_addr = (uint32_t)up_buffer.pBuffer; // RTT 缓冲区地址
                uint32_t size = up_buffer.SizeOfBuffer;
                uint32_t wrOff = up_buffer.WrOff;
                uint32_t rdOff = up_buffer.RdOff;

                // 检查是否有新数据
                if (wrOff != rdOff) {
                    len = (wrOff > rdOff) ? (wrOff - rdOff) : (size - rdOff);

                    if (len > RTT_MAX_BUFFER_SIZE)
                        len = RTT_MAX_BUFFER_SIZE;

                    // **确保不会跨越缓冲区**
                    if (!swd_read_memory(buffer_addr + rdOff, rtt_buffer, len)) {
                        read_delay_ms = 1000;
                        read_err_count++;
                        break;
                    }
                    // **如果 `rdOff + len` 触及缓冲区末尾，继续读取缓冲区头部数据**
                    if (wrOff < rdOff) {
                        uint32_t second_part_len = wrOff;
                        if (second_part_len > RTT_MAX_BUFFER_SIZE - len)
                            second_part_len = RTT_MAX_BUFFER_SIZE - len;

                        if (!swd_read_memory(buffer_addr, rtt_buffer + len, second_part_len)) {
                            read_delay_ms = 1000;
                            read_err_count++;
                            break;
                        }
                        len += second_part_len;
                    }

                    extern chry_ringbuffer_t g_usbshell;
                    chry_ringbuffer_write(&g_usbshell, rtt_buffer, len);

                    // emit(rtt_sig, &tRTTMsgObj,
                    //     args(
                    //         rtt_buffer,
                    //         len
                    //     ));

                    // **逐步更新 RdOff 而不是直接设为 wrOff**
                    rdOff = (rdOff + len) % size;
                    if (!swd_write_word(up_addr + offsetof(SEGGER_RTT_BUFFER_UP, RdOff), rdOff)) {
                        read_delay_ms = 10;
                        read_err_count++;
                        break;
                    }
                }
                read_delay_ms = 1;
                read_err_count = 0;
            } while (0);
            expected_ticks = bflb_mtimer_get_time_us() / 1000 + read_delay_ms;
        }
    }
}

uint32_t write_rtt_and_receive_usb(uint8_t inputChar, uint8_t *usb_tmpbuffer, uint32_t nbytes)
{
    RTT_wChannel = 0;

    uint8_t buffer[16];
    SEGGER_RTT_BUFFER_DOWN down_buffer;
    uint32_t written = 0;

    // 确定要写入的数据源和长度
    uint8_t *write_data = NULL;
    uint32_t write_len = 0;
    uint8_t single_byte_buffer[1];

    if (usb_tmpbuffer != NULL && nbytes > 0) {
        // 多字节写入模式
        write_data = usb_tmpbuffer;
        write_len = nbytes;
    } else {
        // 单字节写入模式
        single_byte_buffer[0] = inputChar;
        write_data = single_byte_buffer;
        write_len = 1;
    }

    // printf("1\r\n");
    if (segger_rtt_addr != 0 && write_len > 0) {
        do {
            uint32_t down_addr = segger_rtt_addr + offsetof(SEGGER_RTT_CB, aDown[RTT_wChannel]);
            // printf("2\r\n");

            // 读取 RTT 控制块，确保 RTT 仍然有效
            if (swd_read_memory(segger_rtt_addr, buffer, 16)) {
                if (memcmp(buffer, RTT_SIGNATURE, 10) != 0) {
                    break;
                }
            }
            // printf("3\r\n");

            // 读取 RTT DownBuffer 结构
            if (!swd_read_memory(down_addr, (uint8_t *)&down_buffer, sizeof(SEGGER_RTT_BUFFER_DOWN))) {
                break;
            }
            // printf("4\r\n");

            uint32_t buffer_addr = (uint32_t)down_buffer.pBuffer;
            uint32_t size = down_buffer.SizeOfBuffer;
            uint32_t wrOff = down_buffer.WrOff;
            uint32_t rdOff = down_buffer.RdOff;
            //         printf("buffer_addr: 0x%x, size: %d, wrOff: %d, rdOff: %d\r\n", buffer_addr, size, wrOff, rdOff);
            // printf("5\r\n");

            // **再次读取 `RdOff` 以确认其是否已更新**
            uint32_t new_rdOff;
            if (!swd_read_word(down_addr + offsetof(SEGGER_RTT_BUFFER_DOWN, RdOff), &new_rdOff)) {
                break;
            }

            if (new_rdOff != rdOff) {
                // 说明 `RdOff` 已更新，重新检查可用空间
                rdOff = new_rdOff;
            }

            // **计算可用空间**
            uint32_t available;
            if (wrOff >= rdOff) {
                available = size - wrOff + rdOff - 1;
            } else {
                available = rdOff - wrOff - 1;
            }

            // **限制写入长度不超过可用空间**
            uint32_t to_write = (write_len < available) ? write_len : available;

            if (to_write == 0) {
                break; // 缓冲区满
            }
            // printf("6\r\n");

            // **写入数据，处理环形缓冲区边界**
            uint32_t first_part = size - wrOff;
            if (to_write <= first_part) {
                // 数据不跨越缓冲区边界
                if (!swd_write_memory(buffer_addr + wrOff, write_data, to_write)) {
                    break;
                }
                wrOff = (wrOff + to_write) % size;
                written = to_write;
            } else {
                // 数据跨越缓冲区边界，分两次写入
                // 第一部分：写到缓冲区末尾
                if (!swd_write_memory(buffer_addr + wrOff, write_data, first_part)) {
                    break;
                }
                // 第二部分：从缓冲区开头写入剩余数据
                uint32_t second_part = to_write - first_part;
                if (!swd_write_memory(buffer_addr, write_data + first_part, second_part)) {
                    break;
                }
                wrOff = second_part;
                written = to_write;
            }
            // printf("7\r\n");

            // **更新 WrOff**
            if (!swd_write_word(down_addr + offsetof(SEGGER_RTT_BUFFER_DOWN, WrOff), wrOff)) {
                break;
            }
        } while (0);
    }
    return written; // 返回实际写入的字节数
}

// static uint16_t receive_usb_get_byte(get_byte_t *ptThis,uint8_t *pchByte, uint16_t hwLength)
// {
//     return peek_queue(&s_tByteQueue, pchByte, hwLength);
// }
#include "shell.h"
int cmd_rttview_start(int argc, char **argv)
{
    if (argc > 4) {
        tfp_printf("Usage: rttview_start <addr> <size> <channel>\r\n");
        printf("Usage: rttview_start <addr> <size> <channel>\r\n");
        return -1;
    }
    if (argc == 1) {
        RTT_wAddr = 0x24000000;
        RTT_wSize = 0x1000;
        RTT_wChannel = 0;
        tfp_printf("Addr = 0x%x, Size = %d, Channel = %d\r\n", RTT_wAddr, RTT_wSize, RTT_wChannel);
        printf("Addr = 0x%x, Size = %d, Channel = %d\r\n", RTT_wAddr, RTT_wSize, RTT_wChannel);
        RTTView_init(RTT_wAddr, RTT_wSize);
        //return 0;
    } else {
        RTT_wAddr = (uint32_t)strtoul(argv[1], NULL, 0x24000000);
        RTT_wSize = (uint32_t)strtoul(argv[2], NULL, 0x10000);
        RTT_wChannel = (uint32_t)strtoul(argv[3], NULL, 0);
        tfp_printf("Addr = 0x%x, Size = %d, Channel = %d\r\n", RTT_wAddr, RTT_wSize, RTT_wChannel);
        printf("Addr = 0x%x, Size = %d, Channel = %d\r\n", RTT_wAddr, RTT_wSize, RTT_wChannel);
        RTTView_init(RTT_wAddr, RTT_wSize);
    }

    if (segger_rtt_addr != 0) {
        swd_read_memory(segger_rtt_addr, (uint8_t *)&_SEGGER_RTT, sizeof(_SEGGER_RTT));
        printf("Find %s addr 0x%x\r\n", _SEGGER_RTT.acID, segger_rtt_addr);
        tfp_printf("Find %s addr 0x%x\r\n", _SEGGER_RTT.acID, segger_rtt_addr);
        for (uint8_t i = 0; i < _SEGGER_RTT.MaxNumUpBuffers; i++) {
            printf("UpBuffer Channel %d Size: %d Mode: %d\r\n", i, _SEGGER_RTT.aUp[i].SizeOfBuffer, _SEGGER_RTT.aUp[i].Flags);
            tfp_printf("UpBuffer Channel %d Size: %d Mode: %d\r\n", i, _SEGGER_RTT.aUp[i].SizeOfBuffer, _SEGGER_RTT.aUp[i].Flags);
        }
        for (uint8_t i = 0; i < _SEGGER_RTT.MaxNumDownBuffers; i++) {
            printf("DownBuffer Channel %d Size: %d Mode: %d\r\n", i, _SEGGER_RTT.aDown[i].SizeOfBuffer, _SEGGER_RTT.aDown[i].Flags);
            tfp_printf("DownBuffer Channel %d Size: %d Mode: %d\r\n", i, _SEGGER_RTT.aDown[i].SizeOfBuffer, _SEGGER_RTT.aDown[i].Flags);
        }

        while (1) {
            read_rtt_and_send_usb();
            //write_rtt_and_receive_usb(0);
            vTaskDelay(1);
        }
    } else {
        tfp_printf("No find _SEGGER_RTT addr\r\n");
        printf("No find _SEGGER_RTT addr\r\n");
    }

    return 0;
}
SHELL_CMD_EXPORT_ALIAS(cmd_rttview_start, rttview_start, rttview start.);

void RTTView_init(uint32_t wAddr, uint32_t wSize)
{
    // if (!swd_init_debug()) {
    //     tfp_printf("swd init error\r\n");
    //     printf("swd init error\r\n");
    //     return;
    // }
    // segger_rtt_addr = 0x2400099c;

    uint8_t buffer[16];
    segger_rtt_addr = 0;
    if (!swd_init_debug()) {
        tfp_printf("swd init error\r\n");
        printf("swd init error\r\n");
        return;
    }
    // queue_init(&s_tByteQueue, s_chByteBuf, sizeof(s_chByteBuf));
    // s_tGetByte.pTarget = &s_tByteQueue;
    // s_tGetByte.fnGetByte = receive_usb_get_byte;
    // init_fsm(check_string, &s_fsmCheckStr, args("RTTView.stop()", &s_tGetByte));
    // clock_cpu_delay_ms(10);
    for (uint32_t rtt_addr = wAddr; rtt_addr < wAddr + wSize; rtt_addr += 4) {
        printf("scan addr: 0x%x\r\n", rtt_addr);
        if (swd_read_memory(rtt_addr, buffer, 16)) {
            // for(uint8_t i = 0 ;i < 16;i++){
            //    tfp_printf("%02x ",buffer[i]);
            //    printf("%02x ",buffer[i]);
            // }
            // tfp_printf("\r\n");
            // printf("\r\n");
            if (memcmp(buffer, RTT_SIGNATURE, 10) == 0) {
                segger_rtt_addr = rtt_addr; // 找到了 RTT 控制块
                tfp_printf("find _SEGGER_RTT addr 0x%x\r\n", segger_rtt_addr);
                printf("find _SEGGER_RTT addr 0x%x\r\n", segger_rtt_addr);
                return;
            }
        }
    }
    tfp_printf("no find _SEGGER_RTT addr\r\n");
    printf("no find _SEGGER_RTT addr\r\n");
}

void RTTView_Uninit(void)
{
    segger_rtt_addr = 0;
}

// void RTTView_start(PikaObj *self, PikaTuple* val)
// {
//     int val_num = pikaTuple_getSize(val);
//     if(val_num == 2){
//       Arg* arg = pikaTuple_getArg(val, 0);
//       RTT_wAddr = (int)arg_getInt(arg);
//       arg = pikaTuple_getArg(val, 1);
//       RTT_wSize = (int)arg_getInt(arg);
//       RTT_wChannel = 0;
//     }
//     if(val_num == 3){
//       Arg* arg = pikaTuple_getArg(val, 0);
//       RTT_wAddr = (int)arg_getInt(arg);
//       arg = pikaTuple_getArg(val, 1);
//       RTT_wSize = (int)arg_getInt(arg);
//       arg = pikaTuple_getArg(val, 2);
//       RTT_wChannel = (int)arg_getInt(arg);
//     }
//     pika_platform_printf("Addr = 0x%x,wSize = %d,Channel = %d\n",RTT_wAddr,RTT_wSize,RTT_wChannel);
//     RTTView_init(RTT_wAddr,RTT_wSize);
//     if(segger_rtt_addr != 0){
//         swd_read_memory(segger_rtt_addr, (uint8_t *)&_SEGGER_RTT, sizeof(_SEGGER_RTT));
//         pika_platform_printf("Find %s addr 0x%x\r\n",_SEGGER_RTT.acID,segger_rtt_addr);
//         for(uint8_t i = 0;i<_SEGGER_RTT.MaxNumUpBuffers;i++){
//             pika_platform_printf("UpBuffer Channel %d Size: %d Mode: %d\r\n",i,_SEGGER_RTT.aUp[i].SizeOfBuffer,_SEGGER_RTT.aUp[i].Flags);
//         }
//         for(uint8_t i = 0;i<_SEGGER_RTT.MaxNumDownBuffers;i++){
//             pika_platform_printf("DownBuffer Channel %d Size: %d Mode: %d\r\n",i,_SEGGER_RTT.aDown[i].SizeOfBuffer,_SEGGER_RTT.aDown[i].Flags);
//         }
//     }
// }

// void RTTView_stop(PikaObj *self, PikaTuple* val)
// {
//     segger_rtt_addr = 0;
// }
