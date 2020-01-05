/*
 * file:		net/llt.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <share/string.h>

#include <net/nllt.h>
#include <net/ethernet.h>
#include <net/arp.h>
#include <net/network.h>

#include <drivers/rtl8139.h>
#include <drivers/amd79c973.h>

/**
 * NlltSend - 发送数据
 * @buf: 要发送的数据
 * 
 */
int NlltSend(NetBuffer_t *buf)
{
    //printk("NLLT: [send] -data:%x -length:%d\n", buf->data, buf->dataLen);

    //EthernetHeader_t *ethHeader = (EthernetHeader_t *)buf->data;
    
    //DumpEthernetHeader(ethHeader);

    //char *p = (char *)(buf->data + SIZEOF_ETHERNET_HEADER);

    //Rtl8139Transmit(buf->data, buf->dataLen);

    //ArpHeader_t *arpHeader = (ArpHeader_t *)p;
    
    //DumpArpHeader((ArpHeader_t *)arpHeader);
        
    //DumpArpHeader(arpHeader);

    /* 环回测试，自己发送给自己作为测试 */
#ifdef _LOOPBACL_DEBUG

    /* 打印数据结构 */
    uint8_t *src = buf->data;

    EthernetHeader_t *ethHeader = (EthernetHeader_t *)src;
    DumpEthernetHeader(ethHeader);

    src += SIZEOF_ETHERNET_HEADER;

    ArpHeader_t *arpHeader = (ArpHeader_t *)src;
    DumpArpHeader((ArpHeader_t *)arpHeader);
    
    NlltReceive(buf->data, buf->dataLen);
#else 
    #ifdef _NIC_AMD79C973
    Amd79c973Send(buf->data, buf->dataLen);
    #endif 

    #ifdef _NIC_RTL8139
    Rtl8139Transmit(buf->data, buf->dataLen);
    #endif 
#endif
    return 0;
}

/**
 * NlltReceive - 接收数据
 * @data: 要接收的数据
 * @length: 数据长度
 */
int NlltReceive(unsigned char *data, unsigned int length)
{
    printk("NLLT: [receive] -data:%x -length:%d\n", data, length);

    /* 以太网接受数据 */
    EthernetReceive(data, length);

    return 0;
}
