/*
 * Netduino Plus 2 USART
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "hw/sysbus.h"
#include "sysemu/char.h"
#include "hw/hw.h"
#include "net/net.h"

#define DEBUG_NETUSART

#ifdef DEBUG_NETUSART
#define DPRINTF(fmt, ...) \
do { printf("netduino_usart: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

#define TYPE_NETDUINO_USART "netduino_usart"
#define NETDUINO_USART(obj) \
    OBJECT_CHECK(struct net_usart, (obj), TYPE_NETDUINO_USART)

struct net_usart {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t usart_sr;
    uint32_t usart_dr;
    uint32_t usart_brr;
    uint32_t usart_cr1;
    uint32_t usart_cr2;
    uint32_t usart_cr3;
    uint32_t usart_gtpr;

    NICState *nic;
    CharDriverState *chr;
    qemu_irq irq;
    NICConf conf;
};

static int usart_can_receive(void *opaque)
{
    //struct net_usart *s = opaque;
    //int ret = MAX(RX_FIFO_SIZE, TX_FIFO_SIZE);
    //uint32_t ch_mode = s->r[R_MR] & UART_MR_CHMODE;

    /*if (ch_mode == NORMAL_MODE || ch_mode == ECHO_MODE) {
        ret = MIN(ret, RX_FIFO_SIZE - s->rx_count);
    }
    if (ch_mode == REMOTE_LOOPBACK || ch_mode == ECHO_MODE) {
        ret = MIN(ret, TX_FIFO_SIZE - s->tx_count);
    } */
    return 1;
}

static void usart_receive(void *opaque, const uint8_t *buf, int size)
{
    struct net_usart *s = opaque;
    int i, num;

    num = size > 8 ? 8 : size;
    s->usart_dr = 0;

    for (i = 0; i < num; i++) {
        s->usart_dr |= (buf[i] << i);
        s->usart_sr |= 0xFF;
    }
    DPRINTF("%c\n", s->usart_dr);
}

static void usart_reset(DeviceState *dev)
{
    struct net_usart *s = NETDUINO_USART(dev);

    s->usart_sr = 0x00C00000; /* HACK: The 0x5F isn't the actual reset value */
    s->usart_dr = 0x00000000;
    s->usart_brr = 0x00000000;
    s->usart_cr1 = 0x00000000;
    s->usart_cr2 = 0x00000000;
    s->usart_cr3 = 0x00000000;
    s->usart_gtpr = 0x00000000;
}

static uint64_t netduino_usart_read(void *opaque, hwaddr addr, unsigned int size)
{
    struct net_usart *s = opaque;
    uint64_t returnval;

    //DPRINTF("Read 0x%x\n", (uint) addr);

    switch (addr) {
        case 0x0:
            return s->usart_sr;
        case 0x04:
            DPRINTF("Value: %x, %c\n", s->usart_dr, s->usart_dr);
            returnval = (s->usart_dr & 0xFF);
            usart_reset(DEVICE(s));
            return returnval;
        case 0x08:
            return s->usart_brr;
        case 0x0C:
            return s->usart_cr1;
        case 0x10:
            return s->usart_cr2;
        case 0x14:
            return s->usart_cr3;
        case 0x18:
            return s->usart_gtpr;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "net_usart_read: Bad offset %x\n", (int)addr);
            return 0;
    }

    return 0;
}

static void netduino_usart_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    struct net_usart *s = opaque;
    uint32_t value = (uint32_t) val64;
    unsigned char ch;

    DPRINTF("Write 0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
        case 0x0:
            s->usart_sr = value;
        case 0x04:
            if (value < 0xF000) {
                ch = value;
                if (s->chr) {
                    qemu_chr_fe_write(s->chr, &ch, 1);
                }
                usart_reset(DEVICE(s));
            }
        case 0x08:
            s->usart_brr |= value;
        case 0x0C:
            s->usart_cr1 |= value;
        case 0x10:
            s->usart_cr2 |= value;
        case 0x14:
            s->usart_cr3 |= value;
        case 0x18:
            s->usart_gtpr |= value;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "net_usart_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps netduino_usart_ops = {
    .read = netduino_usart_read,
    .write = netduino_usart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int netduino_usart_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    struct net_usart *s = NETDUINO_USART(dev);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->mmio, OBJECT(s), &netduino_usart_ops, s,
                          TYPE_NETDUINO_USART, 0x2000);
    sysbus_init_mmio(sbd, &s->mmio);

    s->chr = qemu_char_get_next_serial();

    if (s->chr) {
        qemu_chr_add_handlers(s->chr, usart_can_receive, usart_receive,
                              NULL, s);
    }

    return 0;
}

static void netduino_usart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = netduino_usart_init;
    dc->props = NULL;
    dc->reset = usart_reset;
}

static const TypeInfo netduino_usart_info = {
    .name          = TYPE_NETDUINO_USART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct net_usart),
    .class_init    = netduino_usart_class_init,
};

static void netduino_usart_register_types(void)
{
    type_register_static(&netduino_usart_info);
}

type_init(netduino_usart_register_types)
