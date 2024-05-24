/*
这是一个把单字节数据流封装成 数据包(npkg) 或 指令包(ncmd) 的模块。
单字节数据流在进行通信时不方便制定协议，本模块封装了：
* 数据包：即有序数据列表，类似于udp协议传输的数据包。
* 指令包：基于数据包再次封装，形成 cmd + data 形式的包。
基本工作方式是：
1. 发送过程：
   调用本模块的send接口，传入数据包或指令包。
   本模块会对数据重新编码，对外输出编码后的单字节数据流。
2. 接收过程：
   把接收到的单字节数据流送到本模块的接口函数，本模块解析到合法的数据包或指令包后，
   会还原原始数据包或指令包内容，并通过回调函数输出。


零、模块客制化
  1. 客制化NPKG_ESC_MAX_LEN，这是转义字符的最大长度
一、npkg核心使用：
  【准备接口函数】
      1. 外部定义串口发送数据接口函数 void (*npkg_serial_send)(uint8_t *dat, uint16_t len);
      2. 外部定义npkg解析到数据后的回调函数 void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args);
  【申请句柄和客制化句柄参数】
      3. 通过npkg_alloc申请npkg句柄，同时传入参数
         npkg_t *npkg_alloc(uint16_t recv_buf_size,
                            uint16_t send_buf_size,
                            void (*npkg_serial_send)(uint8_t *dat, uint16_t len),
                            void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args),
                            void *args);
         a. send_buf_size和recv_buf_size，会分别申请一个缓存。
            可能各个句柄需要的缓存大小不同，所以交给用户创建句柄时确定缓存大小。
         b. 上面定义的发送数据接口函数
         c. 上面定义的回调函数 void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args);
         d. 钩子参数args

         也可以通过npkg_alloc_static()静态申请句柄，例子如下
             npkg_t *npkg2 = NULL;                              //npkg句柄，alloc后会指向下面的缓存
             uint8_t npkg2_handle_buf[sizeof(npkg_t)];          //npkg句柄的缓存
             #define NPKG2_RECV_BUF_SIZE     (128)              //接收缓冲区大小
             uint8_t npkg2_recv_buf[NPKG2_RECV_BUF_SIZE] = {0}; //接收缓冲区缓存
             #define NPKG2_SEND_BUF_SIZE     (128)              //发送缓冲区大小
             uint8_t npkg2_send_buf[NPKG2_SEND_BUF_SIZE] = {0}; //发送缓冲区缓存
             void npkg2_serial_send(uint8_t *dat, uint16_t len) {
                 serial_send(s2, dat, len);
             }
             void npkg2_callback(uint8_t *dat, uint16_t len, void *args) {
                 printf("npkg2 callback get dat, len = %d, ", len);
             }
             void npkg2_check(void) {
                 uint8_t buf[128] = {0};
                 uint16_t len = serial_read(s2, buf, serial_avail(s2));
                 npkg_get_buf(npkg2, buf, len);
             }

             npkg2 = npkg_alloc_static(npkg2_handle_buf,
                                       npkg2_recv_buf, NPKG2_RECV_BUF_SIZE, npkg2_send_buf, NPKG2_SEND_BUF_SIZE,
                                       npkg2_serial_send, npkg2_callback, NULL);
      4. 客制化句柄参数：
         a. CRC功能默认开启，如果要关闭crc则设置npkg句柄参数：npkg->flag_crc = NPKG_CRC_OFF
         b. 客制化npkg句柄的转义字符 esc 和转义字符长度 esc_len，转义字符可以是多字节。
            不设置的话默认esc = {0xA1, 0xB2, 0xC3}, esc_len = 3
         c. 客制化npkg句柄的head、dat、end参数。
            不设置的话默认 head = 0xD4, dat = 0xE5, end = 0xF6
         ！！！注：esc、head、dat、end的各个数据尽量不要相等，可能影响判断逻辑。
  【check】
      5. 外部不断地读取串口数据，并往npkg模块里送
             void npkg_get_byte(npkg_t *handle, uint8_t get);
             void npkg_get_buf(npkg_t *handle, uint8_t *dat, uint16_t len);
         不同的平台读取串口数据方法不同，一般用一个进程来不断地check数据，保证对接收到的数据及时进行解析。
  【发送】
      6. 严格发送：uint16_t npkg_send(npkg_t *handle, uint8_t *dat, uint16_t len);
                   通过 npkg_send() 函数来发送一个数据包，发送函数会识别数据有没有和ESC重复，重复的话会转义
                   严格发送绝对不会出错，收到的数据包会和发送的数据包一样。
      7. 宽松发送：这个方式不关心数据内容和ESC冲突，只管包头、包尾、CRC即可。
                   [esc+head] + [dat(任意字节)] + [dat的CRC高字节+dat的CRC低字节] + [esc+end]
                   ！！！如果dat里恰好出现 esc+head、esc+dat、esc+head 组合时，则会导致数据出错，其他情况都没问题。
                   本模块默认esc设置得比较长就会降低数据里出现esc的概率。
                   宽松发送的好处是对于不使用本模块的人也可以很方便地发送数据给本模块。

二、ncmd核心使用
  【准备接口函数】
      1. 外部定义串口发送数据接口函数 void (*ncmd_serial_send)(uint8_t *dat, uint16_t len);
      2. 外部定义ncmd解析到数据后的回调函数 void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
  【申请句柄和客制化句柄参数】
      3. 通过ncmd_alloc申请ncmd句柄，同时传入参数
         ncmd_t *ncmd_alloc(uint16_t recv_buf_size,
                            uint16_t send_buf_size,      //会申请2个send_buf_size的缓存，一个cmd层，一个pkg层
                            void (*ncmd_serial_send)(uint8_t *dat, uint16_t len),
                            void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
                            void *args);
         a. send_buf_size和recv_buf_size，会分别申请一个缓存。
            可能各个句柄需要的缓存大小不同，所以交给用户创建句柄时确定缓存大小。
         b. 上面定义的发送数据接口函数
         c. 上面定义的回调函数 void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
         d. 钩子参数args

         也可以通过ncmd_alloc_static()静态申请句柄，例子如下：
             ncmd_t *ncmd2 = NULL;                              //ncmd句柄，alloc后将会指向下面的缓存
             uint8_t ncmd2_cmd_handle_buf[sizeof(ncmd_t)];      //ncmd句柄的缓存
             uint8_t ncmd2_pkg_handle_buf[sizeof(npkg_t)];      //mpkg句柄的缓存，ncmd是基于npkg的，所以需要一个npkg缓存
             #define NCMD2_RECV_BUF_SIZE     (128)              //接收缓冲区大小
             uint8_t ncmd2_recv_buf[NCMD2_RECV_BUF_SIZE] = {0}; //接收缓冲区缓存
             #define NCMD2_SEND_BUF_SIZE     (128)              //发送缓冲区大小
             uint8_t ncmd2_send_buf_2[NCMD2_SEND_BUF_SIZE*2] = {0};     //发送缓冲区缓存。！！注意发送缓冲区缓存大小是2份
             void ncmd2_serial_send(uint8_t *dat, uint16_t len) {
                 serial_send(s2, dat, len);
             }
             void ncmd2_callback(uint16_t cmd, uint8_t *dat, uint16_t len, void *args) {
                 printf("ncmd2 callback, cmd=%d, get dat, len = %d, ", cmd, len);
             }
             void ncmd2_check(void) {
                 uint8_t buf[128] = {0};
                 uint16_t len = serial_read(s2, buf, serial_avail(s2));
                 ncmd_get_buf(ncmd2, buf, len);
             }

             ncmd2 = ncmd_alloc_static(ncmd2_cmd_handle_buf, ncmd2_pkg_handle_buf,
                                       ncmd2_recv_buf, NCMD2_RECV_BUF_SIZE, ncmd2_send_buf_2, NCMD2_SEND_BUF_SIZE,
                                       ncmd2_serial_send, ncmd2_callback, NULL);
      4. 客制化句柄参数：
         a. CRC功能默认开启，如果要关闭crc则设置ncmd句柄参数：
            ncmd->npkg->flag_crc = NPKG_CRC_OFF
         b. 客制化ncmd句柄的转义字符 ncmd->npkg->esc 和转义字符长度 ncmd->npkg->esc_len，转义字符可以是多字节。
            不设置的话默认 ncmd->npkg->esc = {0xA1, 0xB2, 0xC3}, ncmd->npkg->esc_len = 3
         c. 客制化npkg句柄的ncmd->npkg->head、ncmd->npkg->dat、ncmd->npkg->end参数。
            不设置的话默认 head = 0xD4, dat = 0xE5, end = 0xF6
         ！！！注：esc、head、dat、end的各个数据尽量不要相等，可能影响判断逻辑。
  【check】
      5. 外部不断地读取串口数据，并往npkg模块里送
             void ncmd_get_byte(ncmd_t *handle, uint8_t dat);
             void ncmd_get_buf(ncmd_t *handle, uint8_t *dat, uint16_t len);
         不同的平台读取串口数据方法不同，一般用一个进程来不断地check数据，保证对接收到的数据及时进行解析。
  【发送】
      6. 严格发送：uint16_t ncmd_send(ncmd_t *handle, uint16_t cmd, uint8_t *dat, uint16_t len) 
                   通过 ncmd_send() 函数来发送一个指令包，发送函数会识别数据有没有和ESC重复，重复的话会转义。
                   严格发送绝对不会出错，收到的数据包会和发送的数据包一样。
      7. 宽松发送：这个方式不关心数据内容和ESC冲突，只管包头、包尾、CRC即可。
                   [esc+head] + [cmd高字节+cmd低字节] + [dat(任意字节)] + [包括cmd的CRC的高字节+包括cmd的CRC的低字节] + [esc+end]
                   ！！！如果cmd:dat里恰好出现 esc+head、esc+dat、esc+head 组合时，则会导致数据出错，其他情况都没问题。
                   本模块默认esc设置得比较长就会降低数据里出现esc的概率。
                   宽松发送的好处是对于不使用本模块的人也可以很方便地发送数据给本模块。  
 */
#ifndef __NPKG_H__
#define __NPKG_H__

#include <stdint.h>

#define NPKG_ESC_MAX_LEN        (10)    //转义字符的最大长度

typedef enum {
    NPKG_CRC_OFF = 0,
    NPKG_CRC_ON
} npkg_crc_t;

typedef struct npkg {
    /* 平台接口 */
    void (*npkg_serial_send)(uint8_t *dat, uint16_t len);    //发送数据接口
    /* 模块接口 */
    void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args);
    void *args;
    /* 协议数据结构 */
    //转义字符
    uint8_t esc[NPKG_ESC_MAX_LEN];     //转义字符
    uint8_t esc_len;       //转义字符长度
    uint8_t head, dat, end;
    //crc功能
    npkg_crc_t flag_crc;   //默认打开。打开的话则会在数据包增加2字节作为CRC
    //buf
    uint8_t *send_buf;    //发送buf
    uint16_t send_buf_size;
    uint16_t send_buf_len;              //send_buf里当前有效数据量
    uint8_t *recv_buf;    //接收到的数据包buf
    uint16_t recv_buf_size;
    uint16_t recv_buf_len;              //当前接收到的数据长度
    //逻辑变量
    uint8_t esc_step;       /* 0-CU: 没检测到ESC，当前等待ESC。EC：检测到esc。ED：
                               1-CU：当前检测到了ESC，当前等待ESC后的数据
                                  EC1：接收到了head
                                  EC2：接收到了dat
                                  EC3：接收到了end
                         */
    uint8_t esc_i;      //用于esc计数
    uint8_t flag_recv_data;  //标志当前是否保存数据，1-是，0-否
} npkg_t;
npkg_t *npkg_alloc(uint16_t recv_buf_size,
                   uint16_t send_buf_size,
                   void (*npkg_serial_send)(uint8_t *dat, uint16_t len),
                   void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args),
                   void *args);
npkg_t *npkg_alloc_static(uint8_t *handle_buf,  //大小应当等于sizeof(npkg_t)
                          uint8_t *recv_buf, uint16_t recv_buf_size,
                          uint8_t *send_buf, uint16_t send_buf_size,
                          void (*npkg_serial_send)(uint8_t *dat, uint16_t len),
                          void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args),
                          void *args);
void npkg_free(npkg_t *handle);
uint16_t npkg_send(npkg_t *handle, uint8_t *dat, uint16_t len); //发送一个数据包，返回实际发送的数据量
void npkg_get_byte(npkg_t *handle, uint8_t get);
void npkg_get_buf(npkg_t *handle, uint8_t *dat, uint16_t len);


typedef struct ncmd {
    /* 继承 */
    npkg_t *npkg;
    /* 模块接口 */
    void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args);
    void *args;
    //buf
    uint8_t *cmdbuf;    //用来重新把cmd包整理成pkg包
    uint16_t cmdbuf_size;
} ncmd_t;
ncmd_t *ncmd_alloc(uint16_t recv_buf_size,
                   uint16_t send_buf_size,      //会申请2个send_buf_size的缓存，一个cmd层，一个pkg层
                   void (*ncmd_serial_send)(uint8_t *dat, uint16_t len),
                   void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
                   void *args);
ncmd_t *ncmd_alloc_static(uint8_t *cmd_handle_buf,      //cmd的handle buf，大小等于sizeof(ncmd_t)
                          uint8_t *pkg_handle_buf,      //pkg的handle buf，大小等于sizeof(npkg_t)
                          uint8_t *recv_buf, uint16_t recv_buf_size,
                          uint8_t *send_buf_2, uint16_t send_buf_size,  //需要两倍send_buf_size的内存，一个用于cmd层，一个用于pkg层
                          void (*ncmd_serial_send)(uint8_t *dat, uint16_t len),
                          void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
                          void *args);
                          
void ncmd_free(ncmd_t *handle);
uint16_t ncmd_send(ncmd_t *handle, uint16_t cmd, uint8_t *dat, uint16_t len);   //发送cmd
void ncmd_get_byte(ncmd_t *handle, uint8_t dat);
void ncmd_get_buf(ncmd_t *handle, uint8_t *dat, uint16_t len);


#endif
