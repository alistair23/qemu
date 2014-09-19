/*
 * STM32F205 USART
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

#include "hw/char/stm32f205_usart.h"

#ifndef STM_USART_ERR_DEBUG
#define STM_USART_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_USART_ERR_DEBUG >= lvl) { \
        qemu_log("stm32f205_usart: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static int stm32f205_usart_can_receive(void *opaque)
{
    STM32f205UsartState *s = opaque;

    if (!(s->usart_sr & USART_SR_RXNE)) {
        return 1;
    }

    return 0;
}

static void stm32f205_usart_receive(void *opaque, const uint8_t *buf, int size)
{
    STM32f205UsartState *s = opaque;

    s->usart_dr = *buf;

    if (!(s->usart_cr1 & USART_CR1_UE && s->usart_cr1 & USART_CR1_RE)) {
        /* USART not enabled - drop the chars */
        DB_PRINT("Dropping the chars\n");
        return;
    }

    s->usart_sr |= USART_SR_RXNE;

    if (s->usart_cr1 & USART_CR1_RXNEIE) {
        qemu_set_irq(s->irq, 1);
    }

    DB_PRINT("Receiving: %c\n", s->usart_dr);
}

static void stm32f205_usart_reset(DeviceState *dev)
{
    STM32f205UsartState *s = STM32F205_USART(dev);

    s->usart_sr = USART_SR_RESET;
    s->usart_dr = 0x00000000;
    s->usart_brr = 0x00000000;
    s->usart_cr1 = 0x00000000;
    s->usart_cr2 = 0x00000000;
    s->usart_cr3 = 0x00000000;
    s->usart_gtpr = 0x00000000;
}

static uint64_t stm32f205_usart_read(void *opaque, hwaddr addr,
                                       unsigned int size)
{
    STM32f205UsartState *s = opaque;
    uint64_t retvalue;

    DB_PRINT("Read 0x%"HWADDR_PRIx"\n", addr);

    switch (addr) {
    case USART_SR:
        retvalue = s->usart_sr;
        s->usart_sr &= ~USART_SR_TC;
        if (s->chr) {
            qemu_chr_accept_input(s->chr);
        }
        return retvalue;
    case USART_DR:
        DB_PRINT("Value: 0x%" PRIx32 ", %c\n", s->usart_dr, (char) s->usart_dr);
        s->usart_sr |= USART_SR_TXE;
        s->usart_sr &= ~USART_SR_RXNE;
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
                      "STM32F205_usart_read: Bad offset " \
                      "0x%"HWADDR_PRIx"\n", addr);
        return 0;
    }

    return 0;
}

static void stm32f205_usart_write(void *opaque, hwaddr addr,
                                  uint64_t val64, unsigned int size)
{
    STM32f205UsartState *s = opaque;
    uint32_t value = val64;
    unsigned char ch;

    DB_PRINT("Write 0x%" PRIx32 ", 0x%"HWADDR_PRIx"\n", value, addr);

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
                qemu_chr_fe_write_all(s->chr, &ch, 1);
            }
            s->usart_sr |= USART_SR_TC;
            s->usart_sr &= ~USART_SR_TXE;
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
                      "STM32F205_usart_write: Bad offset " \
                      "0x%"HWADDR_PRIx"\n", addr);
    }
}

static const MemoryRegionOps stm32f205_usart_ops = {
    .read = stm32f205_usart_read,
    .write = stm32f205_usart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f205_usart_init(Object *obj)
{
    STM32f205UsartState *s = STM32F205_USART(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f205_usart_ops, s,
                          TYPE_STM32F205_USART, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    s->chr = qemu_char_get_next_serial();

    if (s->chr) {
        qemu_chr_add_handlers(s->chr, stm32f205_usart_can_receive,
                              stm32f205_usart_receive, NULL, s);
    }
}

static void stm32f205_usart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f205_usart_reset;
}

static const TypeInfo stm32f205_usart_info = {
    .name          = TYPE_STM32F205_USART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32f205UsartState),
    .instance_init = stm32f205_usart_init,
    .class_init    = stm32f205_usart_class_init,
};

static void stm32f205_usart_register_types(void)
{
    type_register_static(&stm32f205_usart_info);
}

type_init(stm32f205_usart_register_types)
