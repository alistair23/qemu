/*
 * STM32F405xx USART
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

#include "hw/char/stm32f405_usart.h"

#ifndef STM_USART_ERR_DEBUG
#define STM_USART_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_USART_ERR_DEBUG >= lvl) { \
        qemu_log("stm32f405xx_usart: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static int usart_can_receive(void *opaque)
{
    Stm32f405UsartState *s = opaque;

    if (s->usart_cr1 & USART_CR1_UE && s->usart_cr1 & USART_CR1_TE) {
        return 1;
    }

    return 0;
}

static void usart_receive(void *opaque, const uint8_t *buf, int size)
{
    Stm32f405UsartState *s = opaque;

    s->usart_dr = *buf;

    s->usart_sr |= USART_SR_RXNE;

    if (s->usart_cr1 & USART_CR1_RXNEIE) {
        qemu_set_irq(s->irq, 1);
    }

    DB_PRINT("Receiving: %c\n", s->usart_dr);
}

static void usart_reset(DeviceState *dev)
{
    Stm32f405UsartState *s = STM32F405xx_USART(dev);

    s->usart_sr = 0x00C00000;
    s->usart_dr = 0x00000000;
    s->usart_brr = 0x00000000;
    s->usart_cr1 = 0x00000000;
    s->usart_cr2 = 0x00000000;
    s->usart_cr3 = 0x00000000;
    s->usart_gtpr = 0x00000000;
}

static uint64_t stm32f405xx_usart_read(void *opaque, hwaddr addr,
                                       unsigned int size)
{
    Stm32f405UsartState *s = opaque;
    uint64_t retvalue;

    DB_PRINT("Read 0x%"HWADDR_PRIx"\n", addr);

    switch (addr) {
    case USART_SR:
        retvalue = s->usart_sr;
        s->usart_sr &= (USART_SR_TC ^ 0xFFFF);
        return retvalue;
    case USART_DR:
        DB_PRINT("Value: 0x%x, %c\n", s->usart_dr, (char) s->usart_dr);
        s->usart_sr |= USART_SR_TXE;
        return s->usart_dr & 0x3FF;
    case USART_BRR:
        return s->usart_brr;
    case USART_CR1:
        return s->usart_cr1;
    case USART_CR2:
        return s->usart_cr2;
    case USART_CR3:
        return s->usart_cr3;
    case USART_GTPR:
        return s->usart_gtpr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_usart_read: Bad offset " \
                      "0x%"HWADDR_PRIx"\n", addr);
        return 0;
    }

    return 0;
}

static void stm32f405xx_usart_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    Stm32f405UsartState *s = opaque;
    uint32_t value = val64;
    unsigned char ch;

    DB_PRINT("Write 0x%x, 0x%"HWADDR_PRIx"\n", value, addr);

    switch (addr) {
    case USART_SR:
        if (value <= 0x3FF) {
            s->usart_sr = value;
        } else {
            s->usart_sr &= value;
        }
        return;
    case USART_DR:
        if (value < 0xF000) {
            ch = value;
            if (s->chr) {
                qemu_chr_fe_write(s->chr, &ch, 1);
            }
            s->usart_sr |= USART_SR_TC;
        }
        return;
    case USART_BRR:
        s->usart_brr = value;
        return;
    case USART_CR1:
        s->usart_cr1 = value;
        return;
    case USART_CR2:
        s->usart_cr2 = value;
        return;
    case USART_CR3:
        s->usart_cr3 = value;
        return;
    case USART_GTPR:
        s->usart_gtpr = value;
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_usart_write: Bad offset " \
                      "0x%"HWADDR_PRIx"\n", addr);
    }
}

static const MemoryRegionOps stm32f405xx_usart_ops = {
    .read = stm32f405xx_usart_read,
    .write = stm32f405xx_usart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f405xx_usart_init(Object *obj)
{
    Stm32f405UsartState *s = STM32F405xx_USART(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f405xx_usart_ops, s,
                          TYPE_STM32F405_USART, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    s->chr = qemu_char_get_next_serial();

    if (s->chr) {
        qemu_chr_add_handlers(s->chr, usart_can_receive, usart_receive,
                              NULL, s);
    }
}

static void stm32f405xx_usart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = usart_reset;
}

static const TypeInfo stm32f405xx_usart_info = {
    .name          = TYPE_STM32F405_USART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Stm32f405UsartState),
    .instance_init = stm32f405xx_usart_init,
    .class_init    = stm32f405xx_usart_class_init,
};

static void stm32f405xx_usart_register_types(void)
{
    type_register_static(&stm32f405xx_usart_info);
}

type_init(stm32f405xx_usart_register_types)
