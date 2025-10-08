#include "wifi_config_server.h"

#define DBG_TAG "wifi_config_server"
#include "log.h"
#include "web/wifi_conf_page.h"
#include "bflb_mtimer.h"
#include "semphr.h"


typedef enum {
    GET, POST, NOSUPPORT,
}http_method;

#define CONTENT_JSON    "application/json"
#define CONTENT_TEXT_HTML    "text/html"
#define HEAD_CONTENT_LENGTH  "Content-Length"

#define HEAD_SIZE   1024
#define PATH_SIZE   128
#define COMMAND_SIZE    1024
#define HEAD_SIZE   1024
#define READ_BUF_SIZE 1024*2
#define METHOD_SIZE 8

SemaphoreHandle_t httpSemaphor = NULL;

void process_request_data(void* msg);
size_t send_redirect(int socket_fd, char* head_buf, char* target);
int get_request_path(char* http_msg, char* command);
int get_request_method(char* http_msg, char* method);
ssize_t send_http_response(int socket_fd, char* head_buf, const void* buf, ssize_t len, char* content_type, int reponse_code);

response_callback data_callback;

TaskHandle_t http_task_handler;

bool server_running = false;

bool http_server_is_runing()
{
    return server_running;
}

/**
 * @brief http server task
 * @param  args
 */
void http_task(void* args)
{
    int ss, sc;
    data_callback = (response_callback)args;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int err;
    socklen_t addrlen;

    //创建socket
    ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ss < 0)
    {
        LOG_I("create socket error code: %d\n", ss);
        return;
    }

    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(80);

    //端口复用
    int opt_value = 1;
    err = setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt_value, sizeof(opt_value));
    LOG_I("socket id: %d\nsetsockopt ret: %d\r\n", ss, err);
    //绑定端口
    err = bind(ss, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err < 0)
    {
        LOG_I("bind error code: %d\r\n", err);
        return;
    }
    int opt = 1;
    ioctlsocket(ss, FIONBIO, &opt);

    /**
     * @brief 开始监听
     *
     * server刚刚关闭又重新打开容易出现-8错误，
     * 上面设置了SO_REUSEADDR端口复用仍然返回-8错误需要修改sdk中代码
     * 
     */
    err = listen(ss, 100);
    if (err < 0)
    {
        LOG_I("listen error code: %d\r\n", err);
        return;
    }
    addrlen = sizeof(struct sockaddr_in);

    MYPARM parm11;
    while (server_running)
    {
        // LOG_I("accept start\r\r\n");
        //等待客户端连接
        sc = accept(ss, (struct sockaddr*)&client_addr, &addrlen);
        if (sc < 0)
        {
            continue;
        }
        else {
            LOG_I("connection:%d\r\n", sc);
        }
        parm11.sc = sc;
        parm11.buf = NULL;
        //处理客户端请求
        process_request_data(&parm11);
        vTaskDelay(500);
    }
    server_running = false;
    closesocket(ss);
    // err = shutdown(ss, SHUT_RDWR);
    err = close(ss);
    LOG_I("http_config_server exit: %d\r\n\r\n", err);
}

/**
 * @brief 关闭当前http server task
 */
void http_server_close()
{
    if (server_running)
    {
        server_running = false;
        bflb_mtimer_delay_ms(1000);
        vTaskDelete(http_task_handler);
        http_task_handler = NULL;
    }
}
/**
 * @brief 启动http stask
 * @param  _cb          post请求数据回调
 * @return int
 */
int http_server_start(response_callback _cb)
{

    http_server_close();

    taskENTER_CRITICAL();
    server_running = true;
    xTaskCreate(http_task, "http_server", 1024 * 3, _cb, 1, &http_task_handler);
    taskEXIT_CRITICAL();

    return 0;
}

/**
 * GET / HTTP1.1
 * @brief 找到 HTTP GET后面的路径
 * @param  http_msg
 * @param  command
 * @return int
 */
int get_request_path(char* http_msg, char* command)
{
    char* p_end = http_msg;
    char* p_start = http_msg;
    memset(command, 0, sizeof(char) * PATH_SIZE);
    while (*p_start != '/')
    {
        p_start++;
    }
    if (p_start == NULL)
    {
        return -1;
    }
    p_end = strchr(http_msg, '\n');    /* code */
    while (p_end != p_start)
    {
        if (*p_end == ' ')
        {
            break;
        }
        p_end--;
    }

    int len = p_end - p_start;
    if (len > 0)
    {
        strncpy(command, p_start, len);
    }
    return len;
}
/**
 * 参考 https://www.cnblogs.com/weibanggang/p/9454581.html
 * @brief 获取请求方式 POST/GET等
 * @param  http_msg
 * @param  method
 * @return int
 */
int get_request_method(char* http_msg, char* method)
{
    char* end;
    end = strchr(http_msg, ' ');
    int len = end - http_msg;
    strncpy(method, http_msg, len);
    return len;
}
/**
 * 参考 https://www.cnblogs.com/52-IT-y/p/17178194.html
 * @brief 获取header数据
 * @param  header
 * @param  key
 * @param  value
 * @return int
 */
int get_header_value(char* header, char* key, char* value)
{
    int len = -1;
    char* key_pos = strstr(header, key);
    if (key_pos == NULL)
    {
        return len;
    }
    char* value_pos = strchr(key_pos, ':');
    if (value_pos == NULL)
    {
        return len;
    }
    value_pos++;
    while (*value_pos == ' ')
    {
        value_pos++;
    }
    char* end_pos = strstr(value_pos, "\r\n");
    if (end_pos == NULL)
    {
        return len;
    }
    len = end_pos - value_pos;
    strncpy(value, value_pos, len);
    return len;
}
/**
 * @brief 解析http数据
 * @param  msg
 */
void process_request_data(void* msg)
{
    MYPARM* param;
    param = (MYPARM*)msg;

    int client;
    client = param->sc;

    char readbuffer[READ_BUF_SIZE];
    int size = 0;
    char command[COMMAND_SIZE];
    char head_buf[HEAD_SIZE];
    char path[PATH_SIZE];
    char method[METHOD_SIZE];

    memset(method, 0, sizeof(method));
    memset(command, 0, sizeof(command));
    memset(head_buf, 0, sizeof(head_buf));
    memset(readbuffer, 0, sizeof(readbuffer));
    //读取客户端的数据
    size = read(client, readbuffer, READ_BUF_SIZE);
    LOG_D("read len: %d\r\n", size);
    LOG_D("get: %s\r\n", readbuffer);
    if (size <= 0)
    {
        LOG_D("size <= 0\r\n");
        return;
    }
    //获取请求方式和请求路径
    get_request_method(readbuffer, method);
    get_request_path(readbuffer, path);
    LOG_I("request path: %s\r\n,method: %s\r\n", path, method);
    //GET请求
    if (strncmp(method, "GET", 3) == 0)
    {
        LOG_I("METHOD: GET\r\n");
        //请求跟路径返回配网页面
        if (strcmp(path, "/") == 0)
        {
            send_http_response(client, head_buf, WIFI_CONF_HTML, strlen(WIFI_CONF_HTML), CONTENT_TEXT_HTML, 200);
        }
        else if (strcmp(path, "/loading") == 0)
        {
            //返回加载中页面
            send_http_response(client, head_buf, HTML_LOADING, strlen(HTML_LOADING) - 1, CONTENT_TEXT_HTML, 200);
        }
        else {
            LOG_I("response null \r\n\r\n");
            close(client);
        }
    }
    //POST请求
    else if (strncmp(method, "POST", 4) == 0)
    {
        //配网页面提交数据
        if (strncmp("/configwifi", path, 11) == 0)
        {
            LOG_I("METHOD: POST\r\n");
            int content_length = 0;
            //没有 POST数据，不处理
            if (strstr(readbuffer, HEAD_CONTENT_LENGTH) == NULL)
            {
                return;
            }
            char length[8];
            memset(length, 0, sizeof(length));
            //读取post数据的长度
            get_header_value(readbuffer, HEAD_CONTENT_LENGTH, length);
            if (strlen(length) <= 0)
            {
                return;
            }
            printf("%s:%s\r\n", HEAD_CONTENT_LENGTH, length);
            content_length = atoi(length);
            if (content_length <= 0)
            {
                return;
            }
            //拿到post的数据
            char* body = strstr(readbuffer, "\r\n\r\n") + 4;
            if (body == NULL || strlen(body) <= 0)
            {
                LOG_I("NO request body");
                return;
            }
            LOG_I("send data\r\n");
            /**
             * 返回请求，重定向到loading加载中页面
             */
            send_redirect(client, head_buf, "/loading");
            if (NULL != data_callback)
            {
                char* data = (char*)pvPortMalloc(strlen(body) + 1);
                memcpy(data, body, strlen(body));
                data[strlen(body)] = '\0';
                //回调函数，通知解析wifi数据
                data_callback(data);
                vPortFree(data);
            }
        }
        else {
            close(client);
        }
    }
    else {
        LOG_I("METHOD: other\r\n");
        char* body = "method support GET or POST";
        send_http_response(client, head_buf, body, strlen(body) - 1, CONTENT_JSON, 200);
    }

}

/**
 * @brief 发送301重定向相应
 * @param  socket_fd       客户端socket id
 * @param  head_buf        head_buf
 * @param  target          重定向的新地址
 * @return size_t          responses 数据长度
 */
size_t send_redirect(int socket_fd, char* head_buf, char* target)
{
    memset(head_buf, 0, sizeof(char) * HEAD_SIZE);
    sprintf(head_buf, "HTTP/1.1 301 Moved Permanetly\r\nLocation: %s \r\n\r\n", target);
    int ret = write(socket_fd, head_buf, strlen(head_buf));
    if (ret <= 0)
    {
        LOG_I("write head error: %d\r\n", ret);
        close(socket_fd);
        return ret;
    }
    close(socket_fd);
    LOG_I("response successful\r\n");
    return ret;
}
/**
 * @brief 发送http responses
 * @param  socket_fd      客户端socket id
 * @param  head_buf
 * @param  buf
 * @param  len
 * @param  content_type
 * @param  reponse_code
 * @return ssize_t
 */
ssize_t send_http_response(int socket_fd, char* head_buf,
    const void* buf, ssize_t len,
    char* content_type, int reponse_code)
{

    memset(head_buf, 0, sizeof(char) * HEAD_SIZE);
    //responses header
    sprintf(head_buf, "HTTP/1.1 %d OK\r\nContent-Length: %ld\r\nContent-Type: %s;charset=UTF-8\r\n\r\n", reponse_code, len, content_type);
    int ret = write(socket_fd, head_buf, strlen(head_buf));
    if (ret <= 0)
    {
        LOG_I("write head error: %d\r\n", ret);
        close(socket_fd);
        return ret;
    }
    //responses data
    ret = write(socket_fd, buf, len);
    if (ret <= 0)
    {
        LOG_I("write response error: %d", ret);
        close(socket_fd);
        return ret;
    }
    close(socket_fd);
    LOG_I("response successful\r\n");
    return ret;

}