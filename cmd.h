#ifndef __CMD_H__
#define __CMD_H__

/*
  一、包协议
  基本格式：
    包头 + 数据包长度高8位 + 数据包长度低8位 + 数据包
  包头格式：
    CMD_ESC+CMD_HEAD      表示包头
    CMD_ESC+CMD_DAT       表示是数据，数据值就是CMD_ESC
  发送端：
    包头    CMD_ESC+CMD_HEAD
    数据    检测并发送，当检测到数据是CMD_ESC时，则发送CMD_ESC+CMD_DAT
  接收端：
    检测到非CMD_ESC     认为是数据
    检测到CMD_ESC       进行标记，等待下一个数据
      下一个数据检测到CMD_HEAD    认为是包头
      下一个数据检测到CMD_DAT     认为是数据CMD_ESC
  总体数据格式：
    包头 + 数据包长度高位 + 数据包长度低位 + 数据包
    发送端和接收端在收发数据包长度时也会进行转义字符检测，防止数据正好和转义字符相同
  二、cmd协议
  cmd在包协议基础上，把包的内容进行封装，形成如下形式：
    命令高8位 + 命令低8位 + 数据长度高8位 + 数据长度低8位 + 数据
  发送端先根据cmd协议整合数据包内容，再通过数据包协议发送出去；
  接收端根据数据包协议解析，解析到数据包后再根据cmd协议解析出cmd，再回调函数。

使用：
  一、客制化部分
      1、客制化转义字符、包头、数据
      2、客制化发送缓冲区和接收的包缓冲区大小
  二、代码使用
      1、设置CMD_PLATFORM宏，并调通相应串口接口函数：发送接口函数package_uart_send()，接收接口函数cmd_check_data()
      1、用 cmd_set_call_back 接口设置接收到cmd后的回调函数
      2、cmd_send_cmd(uint16_t cmd, uint16_t len, uint8_t *p) 来发送一个cmd
      3、把串口接收到的数据传给 cmd_get_byte 或 cmd_get_buf，函数会对数据解析，当解析到一个完整的命令时会回调上面的函数
         或者直接调用本模块的cmd_check_data()函数来检测串口数据。
  长度：
      cmd是16位，2字节，最多支持65536个指令
      len是16位，2字节，最多支持长度是65536字节
 */
#define CMD_PLATFORM    1       /*0-上层linux系统
                                  1-stm32系统
                                 */

#if CMD_PLATFORM == 0
#include <stdint.h>     //linux上此头文件可支持uint8_t等数据类型
#elif CMD_PLATFORM == 1
#include "stm32f4xx_hal.h"    //stm32上
#endif

#define CMD_SEND_BUF_SIZE 256
#define CMD_PACKAGE_BUF_SIZE 256
#define CMD_ESC     0x12    //转义字符
#define CMD_DAT     0x00    //数据的转义，CMD_ESC+CMD_DAT 表示接收到数据CMD_ESC
#define CMD_HEAD    0x34    //包头的转义，CMD_ESC+CMD_HEAD 表示接收到包头

/* 设置回调函数，要求函数类型必须是 void func(uint16_t cmd, uint16_t len, uint8_t *dat) */
void cmd_set_call_back(void (*f)(uint16_t cmd, uint16_t len, uint8_t *dat));

/* 发送一个指令，返回发送缓冲区实际数据量，也就是实际要通过串口发送的数据长度 */
uint16_t cmd_send_cmd(uint16_t cmd, uint16_t len, uint8_t *dat);

/* 接收到的一个字节传给这个函数，这个函数会进行解析，当解析到一个完成的命令包时会回调上面的函数。 */
void cmd_get_byte(uint8_t get);

/* 上面的cmd_get_byte可以一个一个字节解析，在外部使用时一个一个字节传入很麻烦，
   而且外部可能使用DMA方式接受，一次接收到的是一个buf。
   这里直接提供buf数据传入，传入一串数据，函数自动将其内容一个一个传给cmd_get_byte函数解析。*/
void cmd_get_buf(uint8_t *p, uint16_t len);

/*
  既然发送函数已经把串口接口指定了，那么完全可以把接收部分检查串口数据也包含到驱动里。
  本函数检测串口，并把缓冲区里的数据送给cmd_get_buf解析。
  主程序中周期调用此函数即可。 */
void cmd_check_data(void);

#endif
