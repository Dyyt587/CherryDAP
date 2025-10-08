
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <lwip/api.h>
#include <lwip/arch.h>
#include <lwip/opt.h>
#include <lwip/inet.h>
#include <lwip/errno.h>
#include <netdb.h>

#include "shell.h"
#include "utils_getopt.h"
#include "bflb_mtimer.h"

#define TCP_CLIENT_BLOCK_TIMEOUT 1
#define TCP_CLIENT_NOT_DEL_SELF  0
#define TCP_CLIENT_MAX_RECONNECT_ATTEMPTS 5
#define TCP_CLIENT_RECONNECT_DELAY_MS 30

struct arg_param {
    int argc;
    char **argv;
};

// clang-format off
static const uint8_t write_buf[128] = "wifi tcp client test, helloworld!\r\n";
static uint8_t read_buf[128];
// clang-format on

static volatile int wifi_tcp_client_exit;
static volatile int wifi_tcp_client_reconnect = 0;
static shell_sig_func_ptr abort_exec;
static uint64_t total_rx_cnt;
static uint64_t total_tx_cnt;
int tcp_sock =-1;

static void sig_close(int sig)
{
    wifi_tcp_client_exit = 1;

    if (abort_exec) {
        abort_exec(sig);
    }
}

#define PING_USAGE                    \
    "wifi_tcp_test [ip] [port]\r\n"   \
    "\t ip: dest ip or server ip\r\n" \
    "\t port: dest server listen port\r\n"

static void wifi_tcp_client_rx(void *sock_client)
{
    #include "chry_ringbuffer.h"
    printf("tcp client rx task start ...\r\n");


    int ret;
    int timeout_cnt = 0;

    /* read */
    while (1) {
        if(tcp_sock < 0){
            vTaskDelay(20);
            continue;
        }
        ret = read(tcp_sock, read_buf, sizeof(read_buf));

        if (ret >= 0) {
            total_rx_cnt += ret;
            timeout_cnt = 0;
            read_buf[ret]='\0';
            extern chry_ringbuffer_t g_usbrx;
            chry_ringbuffer_write(&g_usbrx, read_buf, ret);
        } else if (ret == ERR_TIMEOUT) {
            if (++timeout_cnt > 3) {
                printf("read failed, timeout_cnt: %d\n\r", timeout_cnt);
                wifi_tcp_client_reconnect = 1;
                //break;
            }
        } else {
            printf("read failed, ret: %d, errno: %d\n\r", ret, errno);
            wifi_tcp_client_reconnect = 1;
            //break;
        }

        vTaskDelay(1);
    }

    /* wait to be deleted */
    #if TCP_CLIENT_NOT_DEL_SELF
    printf("tcp client rx task exiting...\r\n");
    while (1) {
        vTaskDelay(200);
    }
    #else
    vTaskDelete(NULL);
    #endif

    printf("tcp client rx task exit!\r\n");
}
static void wifi_tcp_client_tx(void *sock_client)
{
    printf("tcp client tx task start ...\r\n");

    tcp_sock = *(int*)sock_client;
    int ret;
    int timeout_cnt = 0;

    /* write */
    while (1) {
        if(tcp_sock < 0){
            vTaskDelay(2000);
            continue;
        }
        ret = write(tcp_sock, write_buf, sizeof(write_buf));

        if (ret >= 0) {
            total_tx_cnt += sizeof(write_buf);
            timeout_cnt = 0;
        } else if (ret == ERR_TIMEOUT) {
            if (++timeout_cnt > 3) {
                printf("write failed, timeout_cnt: %d\n\r", timeout_cnt);
                wifi_tcp_client_reconnect = 1;
                //break;
            }
        } else {
            printf("write failed, ret: %d, errno: %d\n\r", ret, errno);
            wifi_tcp_client_reconnect = 1;
           // break;
        }

        vTaskDelay(2000);
    }

    /* wait to be deleted */
    #if TCP_CLIENT_NOT_DEL_SELF
    printf("tcp client tx task exiting...\r\n");
    while (1) {
        vTaskDelay(200);
    }
    #else
    vTaskDelete(NULL);
    #endif

    printf("tcp client tx task exit!\r\n");
}

static void wifi_tcp_client_init(void *input_arg)
{
    printf("tcp client task start ...\r\n");

    char *addr;
    char *port;
    int sock_client = -1;
    struct sockaddr_in remote_addr;
    TaskHandle_t px_tcpclient_rx_task = NULL;
    TaskHandle_t px_tcpclient_tx_task = NULL;
    struct arg_param* arg = (struct arg_param*)input_arg;
    int reconnect_attempts = 0;

    /* check arg */
    if (arg->argc < 3) {
        printf("%s", PING_USAGE);
        goto __exit;
    }

    /* get address (argv[1] if present) */
    addr = arg->argv[1];
    /* get port number (argv[2] if present) */
    port = arg->argv[2];

    /* create socket */
    if ((sock_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("TCP Client create socket error\r\n");
        goto __exit;
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(atoi(port));
    remote_addr.sin_addr.s_addr = inet_addr(addr);
    memset(&(remote_addr.sin_zero), 0, sizeof(remote_addr.sin_zero));
    printf("Server ip Address : %s:%s\r\n", addr, port);

    /* connect socket */
    if (connect(sock_client, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) != 0) {
        printf("TCP client connect server falied!\r\n");
        goto __exit;
    }
    printf("TCP client connect server success!\r\n");

    #if TCP_CLIENT_BLOCK_TIMEOUT
    /* blocking timeout */
    int timeout_ms = 4000;
    #if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int opt_on = timeout_ms;
    #else
    struct timeval opt_on = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms - (opt_on.tv_sec * 1000)) * 1000,
    };
    #endif
    setsockopt(sock_client, SOL_SOCKET, SO_SNDTIMEO, (void *)&opt_on, sizeof(opt_on));
    #endif

    total_rx_cnt = 0;
    total_tx_cnt = 0;
    wifi_tcp_client_exit = 0;
    wifi_tcp_client_reconnect = 0;
    abort_exec = shell_signal(SHELL_SIGINT, sig_close);
    printf("Press CTRL-C to exit before next Shell CMD.\r\n");

reconnect_loop:
    /* fork recv and send task, dont care fail */
    xTaskCreate(wifi_tcp_client_rx, "tcp_client_rx", 512, (void *)&sock_client, 20, &px_tcpclient_rx_task);
    xTaskCreate(wifi_tcp_client_tx, "tcp_client_tx", 512, (void *)&sock_client, 20, &px_tcpclient_tx_task);

    while (!wifi_tcp_client_exit) {
        if (wifi_tcp_client_reconnect) {
            reconnect_attempts++;
            
            if (reconnect_attempts > TCP_CLIENT_MAX_RECONNECT_ATTEMPTS) {
                printf("TCP reconnection failed after %d attempts, giving up.\r\n", TCP_CLIENT_MAX_RECONNECT_ATTEMPTS);
                wifi_tcp_client_exit = 1;
                break;
            }
            
            printf("TCP connection lost, attempting to reconnect... (attempt %d/%d)\r\n", 
                   reconnect_attempts, TCP_CLIENT_MAX_RECONNECT_ATTEMPTS);
            wifi_tcp_client_reconnect = 0;
            
            /* Stop existing tasks */
            if (px_tcpclient_rx_task) {
                vTaskSuspend(px_tcpclient_rx_task);
            }
            if (px_tcpclient_tx_task) {
                vTaskSuspend(px_tcpclient_tx_task);
            }
            
            /* Close old socket */
            if (sock_client >= 0) {
                closesocket(sock_client);
                sock_client = -1;
            }
            
            vTaskDelay(TCP_CLIENT_RECONNECT_DELAY_MS); /* Wait before reconnect */
            
            /* Recreate socket and reconnect */
            if ((sock_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                printf("TCP Client create socket error during reconnect\r\n");
                vTaskDelay(TCP_CLIENT_RECONNECT_DELAY_MS);
                continue;
            }
            
            if (connect(sock_client, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) != 0) {
                printf("TCP client reconnect failed, retrying...\r\n");
                closesocket(sock_client);
                sock_client = -1;
                vTaskDelay(TCP_CLIENT_RECONNECT_DELAY_MS);
                continue;
            }
            
            printf("TCP client reconnected successfully!\r\n");
            reconnect_attempts = 0; /* Reset counter on successful reconnect */
            
            /* Set socket timeout again */
            #if TCP_CLIENT_BLOCK_TIMEOUT
            int timeout_ms = 4000;
            #if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
            int opt_on = timeout_ms;
            #else
            struct timeval opt_on = {
                .tv_sec = timeout_ms / 1000,
                .tv_usec = (timeout_ms - (opt_on.tv_sec * 1000)) * 1000,
            };
            #endif
            setsockopt(sock_client, SOL_SOCKET, SO_SNDTIMEO, (void *)&opt_on, sizeof(opt_on));
            #endif
            
            if (px_tcpclient_rx_task) {
                vTaskResume(px_tcpclient_rx_task);
            }
            if (px_tcpclient_tx_task) {
                vTaskResume(px_tcpclient_tx_task);
            }
            //goto reconnect_loop;
        }
        vTaskDelay(500);
    }

__exit:
    /* exit procedure */
    if (sock_client >= 0) {
        printf("closesocket!\r\n");
        closesocket(sock_client);
    }

    if (px_tcpclient_rx_task || px_tcpclient_tx_task) {
        vTaskDelay(500);
    }

    #if TCP_CLIENT_NOT_DEL_SELF
    if (px_tcpclient_rx_task) {
        vTaskDelete(px_tcpclient_rx_task);
    }
    if (px_tcpclient_tx_task) {
        vTaskDelete(px_tcpclient_tx_task);
    }
    #endif

    if (total_rx_cnt || total_tx_cnt) {
        printf("Total recv data=%lld\r\n", total_rx_cnt);
        printf("Total send data=%lld\r\n", total_tx_cnt);
    }

    printf("tcp_client exit!\r\n");
    vTaskDelete(NULL);
}

#ifdef CONFIG_SHELL
#include <shell.h>

int cmd_wifi_tcp_client(int argc, char **argv)
{
    struct arg_param arg = {argc, argv};

    if (pdPASS != xTaskCreate(wifi_tcp_client_init, "tcp_client", 512, (void *)&arg, 15, NULL)) {
        return -1;
    }

    return 0;
}

SHELL_CMD_EXPORT_ALIAS(cmd_wifi_tcp_client, wifi_tcp_test, wifi tcp test);
#endif
