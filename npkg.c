#include <npkg.h>
#include <stdlib.h>
#include <string.h>

/* 发送缓冲区操作 */
/************************** 缓冲区操作 ******************************/
static void send_buf_clean(npkg_t *handle)
{
    handle->send_buf_len = 0;
}
static void send_buf_add(npkg_t *handle, uint8_t dat)
{
    handle->send_buf[handle->send_buf_len++] = dat;
}
/************************** 把发送缓冲区的数据发送出去 ******************************/
static void package_send_buf(npkg_t *handle)
{
    handle->npkg_serial_send(handle->send_buf, handle->send_buf_len);
    send_buf_clean(handle);
}
/************************** CRC ******************************/
static const uint8_t auchCRCHi[] = {
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40
} ;
static const uint8_t auchCRCLo[] = {
    0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,
    0x07,0xC7,0x05,0xC5,0xC4,0x04,0xCC,0x0C,0x0D,0xCD,
    0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,
    0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,
    0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,0x14,0xD4,
    0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,
    0x11,0xD1,0xD0,0x10,0xF0,0x30,0x31,0xF1,0x33,0xF3,
    0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,
    0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,
    0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,
    0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,
    0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,
    0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,
    0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,
    0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,
    0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,
    0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,
    0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,
    0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,
    0x70,0xB0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,
    0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,
    0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,
    0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4B,0x8B,
    0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
    0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
    0x43,0x83,0x41,0x81,0x80,0x40
} ;
static uint16_t CRC16 (uint8_t *puchMsg, uint16_t usDataLen )
{
    uint8_t uchCRCHi = 0xFF ; /* high byte of CRC initialized */
    uint8_t uchCRCLo = 0xFF ; /* low byte of CRC initialized */
    uint8_t uIndex ; /* will index into CRC lookup table */
    while (usDataLen--) /* pass through message buffer */
    {
        uIndex = uchCRCLo ^ *puchMsg++ ; /* calculate the CRC */
        uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex] ;
        uchCRCHi = auchCRCLo[uIndex] ;
    }
    return (((uint16_t)uchCRCHi) << 8 | ((uint16_t)uchCRCLo));
}


npkg_t *npkg_alloc(uint16_t recv_buf_size,
                   uint16_t send_buf_size, 
                   void (*npkg_serial_send)(uint8_t *dat, uint16_t len),
                   void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args),
                   void *args) {
    npkg_t *p = malloc(sizeof(npkg_t));
    memset(p, 0, sizeof(npkg_t));
    p->recv_buf_size = recv_buf_size;
    p->send_buf_size = send_buf_size;
    p->recv_buf = malloc(p->recv_buf_size);
    memset(p->recv_buf, 0, p->recv_buf_size);
    p->send_buf = malloc(p->send_buf_size);
    memset(p->send_buf, 0, p->send_buf_size);
    p->npkg_serial_send = npkg_serial_send;
    p->npkg_callback = npkg_callback;
    p->args = args;
    p->esc[0] = 0xA1;
    p->esc[1] = 0xB2;
    p->esc[2] = 0xC3;
    p->esc_len = 3;
    p->head = 0xD4;
    p->dat = 0xE5;
    p->end = 0xF6;
    p->flag_crc = NPKG_CRC_ON;
    return p;
}
npkg_t *npkg_alloc_static(uint8_t *handle_buf,  //大小应当等于sizeof(npkg_t)
                          uint8_t *recv_buf, uint16_t recv_buf_size,
                          uint8_t *send_buf, uint16_t send_buf_size,
                          void (*npkg_serial_send)(uint8_t *dat, uint16_t len),
                          void (*npkg_callback)(uint8_t *dat, uint16_t len, void *args),
                          void *args) {
    npkg_t *p = (npkg_t*)handle_buf;
    memset(p, 0, sizeof(npkg_t));
    p->recv_buf_size = recv_buf_size;
    p->send_buf_size = send_buf_size;
    p->recv_buf = recv_buf;
    memset(p->recv_buf, 0, p->recv_buf_size);
    p->send_buf = send_buf;
    memset(p->send_buf, 0, p->send_buf_size);
    p->npkg_serial_send = npkg_serial_send;
    p->npkg_callback = npkg_callback;
    p->args = args;
    p->esc[0] = 0xA1;
    p->esc[1] = 0xB2;
    p->esc[2] = 0xC3;
    p->esc_len = 3;
    p->head = 0xD4;
    p->dat = 0xE5;
    p->end = 0xF6;
    p->flag_crc = NPKG_CRC_ON;
    return p;    
}
void npkg_free(npkg_t *handle) {
    if(handle) {
        if(handle->send_buf) {
            free(handle->send_buf);
        }
        if(handle->recv_buf) {
            free(handle->recv_buf);
        }
        free(handle);
    }
}
uint16_t npkg_send(npkg_t *handle, uint8_t *dat, uint16_t len) {
    send_buf_clean(handle);
    //包头
    for(uint8_t i = 0; i < handle->esc_len; i++) {
        send_buf_add(handle, handle->esc[i]);
    }
    send_buf_add(handle, handle->head);
    //数据
    uint8_t esc_i = 0;
    for(uint16_t i = 0; i < len; i++) {
        send_buf_add(handle, dat[i]);
        if(dat[i] == handle->esc[esc_i]) {      //本字节和esc顺序匹配
            esc_i ++;
            if(esc_i == handle->esc_len) {      //发送的数据里出现了esc
                send_buf_add(handle, handle->dat);
                esc_i = 0;
            }
        } else {
            esc_i = 0;
        }
    }
    //CRC
    if(handle->flag_crc == NPKG_CRC_ON) {
        uint16_t crc = CRC16(dat, len);
        send_buf_add(handle, (uint8_t)(crc>>8));
        send_buf_add(handle, (uint8_t)crc);
    }
    //包尾
    for(uint8_t i = 0; i < handle->esc_len; i++) {
        send_buf_add(handle, handle->esc[i]);
    }
    send_buf_add(handle, handle->end);
    package_send_buf(handle);
    return handle->send_buf_len;
}
void npkg_get_byte(npkg_t *handle, uint8_t dat) {
    if(handle->flag_recv_data == 1) {        //只要接收数据开关打开，不管是不是转义字符都会保存数据
        handle->recv_buf[handle->recv_buf_len++] = dat;
    }

    switch(handle->esc_step) {
    case 0:     //0-CU: 没检测到ESC，当前等待ESC。EC：检测到esc。ED：
        if(dat == handle->esc[handle->esc_i]) { //当前字节与esc顺序匹配
            handle->esc_i ++;
            if(handle->esc_i == handle->esc_len) {      //匹配到了esc
                handle->esc_step = 1;
            }
        } else if(dat == handle->esc[0]) {      //与esc顺序不匹配，但是与esc第一个字节匹配，可能还是接收到esc的第一个字节
            handle->esc_i = 1;
        } else {
            handle->esc_i = 0;
        }
        break;
    case 1:     //1-CU：当前检测到了ESC，当前等待ESC后的数据
        if(dat == handle->head) {       //检测到包头，下个字节开始接收数据，如果当前已经在接收数据了则舍弃已经接收的数据
            handle->recv_buf_len = 0;
            handle->flag_recv_data = 1;
            
            handle->esc_i = 0;
            handle->esc_step = 0;
        } else if(dat == handle->dat) { //检测ESC+DAT，当前没有开启接收数据则忽略，当前开启了接收数据则应当舍弃刚刚接收的数据
            if(handle->flag_recv_data == 1) {
                handle->recv_buf_len --;
            }
            
            handle->esc_i = 0;
            handle->esc_step = 0;
        } else if(dat == handle->end) { //检测到了包尾，应当舍弃刚刚接收到的ESC，然后进行包解析
            if(handle->flag_recv_data == 1) {
                handle->recv_buf_len -= handle->esc_len + 1;
                //到这里，数据包放在了handle->recv_buf，长度是handle->recv_buf_len（含CRC）
                if(handle->flag_crc == NPKG_CRC_ON) {
                    uint16_t crc_get = (uint16_t)(handle->recv_buf[handle->recv_buf_len-1]) + (((uint16_t)(handle->recv_buf[handle->recv_buf_len-2]))<<8);
                    uint16_t crc_calc = CRC16(handle->recv_buf, handle->recv_buf_len-2);
                    if(crc_get == crc_calc) {
                        handle->npkg_callback(handle->recv_buf, handle->recv_buf_len-2, handle->args);
                    }
                } else {
                    handle->npkg_callback(handle->recv_buf, handle->recv_buf_len, handle->args);
                }
                handle->esc_step = 0;
                handle->esc_i = 0;
                handle->recv_buf_len = 0;
                handle->flag_recv_data = 0;
            }
        } else {        //严格来说不会出现这种情况，除非在客户的宽松发送下可能会出现，处理方式是当做数据记录下来（不做删除处理）
            handle->esc_i = 0;
            handle->esc_step = 0;
            //仍然判断是否是esc
            if(dat == handle->esc[handle->esc_i]) {
                handle->esc_i ++;
                if(handle->esc_i == handle->esc_len) {
                    handle->esc_step = 1;
                }
            } else {
                handle->esc_i = 0;
            }
        }
        break;
    }
}
void npkg_get_buf(npkg_t *handle, uint8_t *dat, uint16_t len) {
    for(uint16_t i = 0; i < len; i++) {
        npkg_get_byte(handle, dat[i]);
    }
}


/*
  下面是把npkg封装成ncmd，npkg的前2个字节作为cmd
 */
static void npkg_callback(uint8_t *dat, uint16_t len, void *args){
    uint16_t cmd = (((uint16_t)(dat[0]))<<8) + (uint16_t)(dat[1]);
    ncmd_t *handle = (ncmd_t*)args;
    handle->ncmd_callback(cmd, dat+2, len-2, handle->args);
}
uint16_t ncmd_send(ncmd_t *handle, uint16_t cmd, uint8_t *dat, uint16_t len) {
    uint8_t cmd0, cmd1;
    cmd0 = (uint8_t)cmd;
    cmd1 = (uint8_t)(cmd>>8);
    handle->cmdbuf[0] = cmd1;
    handle->cmdbuf[1] = cmd0;
    memcpy(handle->cmdbuf+2, dat, len);
    return npkg_send(handle->npkg, handle->cmdbuf, len+2);
}
void ncmd_get_byte(ncmd_t *handle, uint8_t dat) {
    npkg_get_byte(handle->npkg, dat);
}
void ncmd_get_buf(ncmd_t *handle, uint8_t *dat, uint16_t len) {
    npkg_get_buf(handle->npkg, dat, len);
}
ncmd_t *ncmd_alloc(uint16_t recv_buf_size,
                   uint16_t send_buf_size,
                   void (*ncmd_serial_send)(uint8_t *dat, uint16_t len),
                   void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
                   void *args) {
    ncmd_t *p = malloc(sizeof(ncmd_t));
    memset(p, 0, sizeof(ncmd_t));
    p->cmdbuf_size = send_buf_size;
    p->cmdbuf = malloc(p->cmdbuf_size);
    memset(p->cmdbuf, 0, p->cmdbuf_size);
    p->ncmd_callback = ncmd_callback;
    p->args = args;
    p->npkg = npkg_alloc(recv_buf_size, send_buf_size, ncmd_serial_send, npkg_callback, (void*)p);
    return p;
}
ncmd_t *ncmd_alloc_static(uint8_t *cmd_handle_buf,
                          uint8_t *pkg_handle_buf, 
                          uint8_t *recv_buf, uint16_t recv_buf_size,
                          uint8_t *send_buf_2, uint16_t send_buf_size,  //需要两倍send_buf_size的内存，一个用于cmd层，一个用于pkg层
                          void (*ncmd_serial_send)(uint8_t *dat, uint16_t len),
                          void (*ncmd_callback)(uint16_t cmd, uint8_t *dat, uint16_t len, void *args),
                          void *args) {
    ncmd_t *p = (ncmd_t*)cmd_handle_buf;
    memset(p, 0, sizeof(ncmd_t));
    p->cmdbuf_size = send_buf_size;
    p->cmdbuf = send_buf_2;
    memset(p->cmdbuf, 0, p->cmdbuf_size);
    p->ncmd_callback = ncmd_callback;
    p->args = args;
    p->npkg = npkg_alloc_static(pkg_handle_buf, recv_buf, recv_buf_size, send_buf_2+send_buf_size, send_buf_size,
                                ncmd_serial_send, npkg_callback, (void*)p);
    return p;    
}
void ncmd_free(ncmd_t *handle) {
    if(handle) {
        if(handle->npkg) {
            npkg_free(handle->npkg);
        }
        if(handle->cmdbuf) {
            free(handle->cmdbuf);
        }
        free(handle);
    }
}


