/*
 * STM32F405 SYSCFG
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

#include "hw/misc/stm32f405_syscfg.h"

#ifndef STM_SYSCFG_ERR_DEBUG
#define STM_SYSCFG_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_SYSCFG_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f405_syscfg_reset(DeviceState *dev)
{
    STM32f405SyscfgState *s = STM32F405_SYSCFG(dev);

    s->syscfg_memrmp = 0x00000000;
    s->syscfg_pmc = 0x00000000;
    s->syscfg_exticr1 = 0x00000000;
    s->syscfg_exticr2 = 0x00000000;
    s->syscfg_exticr3 = 0x00000000;
    s->syscfg_exticr4 = 0x00000000;
    s->syscfg_cmpcr = 0x00000000;
}

static void stm32f405_syscfg_set_irq(void * opaque, int irq, int level)
{
    STM32f405SyscfgState *s = opaque;
    uint8_t config;

    DB_PRINT("Interupt: GPIO: %d, Line: %d; Level: %d\n", irq / 16,
             irq % 16, level);

    config = irq / 16;

    switch (irq % 16) {
        case 0:
            if ((s->syscfg_exticr1 & 0xF) == config) {
                qemu_set_irq(s->gpio_out[0], level);
                DB_PRINT("Pulse EXTI: 0\n");
            }
            break;
        case 1:
            if (((s->syscfg_exticr1 & 0xF0) >> 4) == config) {
                qemu_set_irq(s->gpio_out[1], level);
                DB_PRINT("Pulse EXTI: 1\n");
            }
            break;
        case 2:
            if (((s->syscfg_exticr1 & 0xF00) >> 8) == config) {
                qemu_set_irq(s->gpio_out[2], level);
                DB_PRINT("Pulse EXTI: 2\n");
            }
            break;
        case 3:
            if (((s->syscfg_exticr1 & 0xF000) >> 12) == config) {
                qemu_set_irq(s->gpio_out[3], level);
                DB_PRINT("Pulse EXTI: 3\n");
            }
            break;
        case 4:
            if ((s->syscfg_exticr2 & 0xF) == config) {
                qemu_set_irq(s->gpio_out[4], level);
                DB_PRINT("Pulse EXTI: 4\n");
            }
            break;
        case 5:
            if (((s->syscfg_exticr2 & 0xF0) >> 4) == config) {
                qemu_set_irq(s->gpio_out[5], level);
                DB_PRINT("Pulse EXTI: 5\n");
            }
            break;
        case 6:
            if (((s->syscfg_exticr2 & 0xF00) >> 8) == config) {
                qemu_set_irq(s->gpio_out[6], level);
                DB_PRINT("Pulse EXTI: 6\n");
            }
            break;
        case 7:
            if (((s->syscfg_exticr2 & 0xF000) >> 12) == config) {
                qemu_set_irq(s->gpio_out[7], level);
                DB_PRINT("Pulse EXTI: 7\n");
            }
            break;
        case 8:
            if ((s->syscfg_exticr3 & 0xF) == config) {
                qemu_set_irq(s->gpio_out[8], level);
                DB_PRINT("Pulse EXTI: 8\n");
            }
            break;
        case 9:
            if (((s->syscfg_exticr3 & 0xF0) >> 4) == config) {
                qemu_set_irq(s->gpio_out[9], level);
                DB_PRINT("Pulse EXTI: 9\n");
            }
            break;
        case 10:
            if (((s->syscfg_exticr3 & 0xF00) >> 8) == config) {
                qemu_set_irq(s->gpio_out[10], level);
                DB_PRINT("Pulse EXTI: 10\n");
            }
            break;
        case 11:
            if (((s->syscfg_exticr3 & 0xF000) >> 12) == config) {
                qemu_set_irq(s->gpio_out[11], level);
                DB_PRINT("Pulse EXTI: 11\n");
            }
            break;
        case 12:
            if ((s->syscfg_exticr4 & 0xF) == config) {
                qemu_set_irq(s->gpio_out[12], level);
                DB_PRINT("Pulse EXTI: 12\n");
            }
            break;
        case 13:
            if (((s->syscfg_exticr4 & 0xF0) >> 4) == config) {
                qemu_set_irq(s->gpio_out[13], level);
                DB_PRINT("Pulse EXTI: 13\n");
            }
            break;
        case 14:
            if (((s->syscfg_exticr4 & 0xF00) >> 8) == config) {
                qemu_set_irq(s->gpio_out[14], level);
                DB_PRINT("Pulse EXTI: 14\n");
            }
            break;
        case 15:
            if (((s->syscfg_exticr4 & 0xF000) >> 12) == config) {
                qemu_set_irq(s->gpio_out[15], level);
                DB_PRINT("Pulse EXTI: 15\n");
            }
            break;
    }
}

static uint64_t stm32f405_syscfg_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32f405SyscfgState *s = opaque;

    DB_PRINT("0x%x\n", (uint) addr);

    switch (addr) {
    case SYSCFG_MEMRMP:
        return s->syscfg_memrmp;
    case SYSCFG_PMC:
        return s->syscfg_pmc;
    case SYSCFG_EXTICR1:
        return s->syscfg_exticr1;
    case SYSCFG_EXTICR2:
        return s->syscfg_exticr2;
    case SYSCFG_EXTICR3:
        return s->syscfg_exticr3;
    case SYSCFG_EXTICR4:
        return s->syscfg_exticr4;
    case SYSCFG_CMPCR:
        return s->syscfg_cmpcr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405_syscfg_read: Bad offset %x\n", (int)addr);
        return 0;
    }

    return 0;
}

static void stm32f405_syscfg_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32f405SyscfgState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
    case SYSCFG_MEMRMP:
        qemu_log_mask(LOG_UNIMP,
                      "STM32F405_syscfg_write: Changeing the memory mapping " \
                      "isn't supported in QEMU\n");
        return;
    case SYSCFG_PMC:
        qemu_log_mask(LOG_UNIMP,
                      "STM32F405_syscfg_write: Peripheral mode configuration " \
                      "isn't supported in QEMU\n");
        return;
    case SYSCFG_EXTICR1:
        s->syscfg_exticr1 = (value & 0xFFFF);
        return;
    case SYSCFG_EXTICR2:
        s->syscfg_exticr2 = (value & 0xFFFF);
        return;
    case SYSCFG_EXTICR3:
        s->syscfg_exticr3 = (value & 0xFFFF);
        return;
    case SYSCFG_EXTICR4:
        s->syscfg_exticr4 = (value & 0xFFFF);
        return;
    case SYSCFG_CMPCR:
        s->syscfg_cmpcr = value;
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405_syscfg_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps stm32f405_syscfg_ops = {
    .read = stm32f405_syscfg_read,
    .write = stm32f405_syscfg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f405_syscfg_init(Object *obj)
{
    STM32f405SyscfgState *s = STM32F405_SYSCFG(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f405_syscfg_ops, s,
                          TYPE_STM32F405_SYSCFG, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    qdev_init_gpio_in(DEVICE(obj), stm32f405_syscfg_set_irq, 16 * 9);
    qdev_init_gpio_out(DEVICE(obj), s->gpio_out, 16);
}

static void stm32f405_syscfg_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f405_syscfg_reset;
}

static const TypeInfo stm32f405_syscfg_info = {
    .name          = TYPE_STM32F405_SYSCFG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32f405SyscfgState),
    .instance_init = stm32f405_syscfg_init,
    .class_init    = stm32f405_syscfg_class_init,
};

static void stm32f405_syscfg_register_types(void)
{
    type_register_static(&stm32f405_syscfg_info);
}

type_init(stm32f405_syscfg_register_types)
