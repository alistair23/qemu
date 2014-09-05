/*
 * STM32F405xx EXTI
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
#include "hw/hw.h"

#ifndef ST_EXTI_ERR_DEBUG
#define ST_EXTI_ERR_DEBUG 1
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (ST_EXTI_ERR_DEBUG >= lvl) { \
        fprintf(stderr, "stm32f405xx_exti: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#define EXTI_IMR   0x00
#define EXTI_EMR   0x04
#define EXTI_RTSR  0x08
#define EXTI_FTSR  0x0C
#define EXTI_SWIER 0x10
#define EXTI_PR    0x14

#define NUM_GPIO_EVENT_IN_LINES (15 * 9)

#define NUM_INTERRUPT_OUT_LINES 15

#define TYPE_STM32F405xx_EXTI "stm32f405xx-exti"
#define STM32F405xx_EXTI(obj) \
    OBJECT_CHECK(STM32F405Exti, (obj), TYPE_STM32F405xx_EXTI)

#define TYPE_STM32F405xx_SYSCFG "stm32f405xx-syscfg"
#define STM32F405xx_SYSCFG(obj) \
    OBJECT_CHECK(Stn32f405Syscfg, (obj), TYPE_STM32F405xx_SYSCFG)

typedef struct {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t exti_imr;
    uint32_t exti_emr;
    uint32_t exti_rtsr;
    uint32_t exti_ftsr;
    uint32_t exti_swier;
    uint32_t exti_pr;

    qemu_irq* irq;
} STM32F405Exti;

static void exti_reset(DeviceState *dev)
{
    STM32F405Exti *s = STM32F405xx_EXTI(dev);

    s->exti_imr = 0x00000000;
    s->exti_emr = 0x00000000;
    s->exti_rtsr = 0x00000000;
    s->exti_ftsr = 0x00000000;
    s->exti_swier = 0x00000000;
    s->exti_pr = 0x00000000;
}

static void exti_set_irq(void * opaque, int irq, int level)
{
    STM32F405Exti *s = opaque;
    DeviceState *syscfg = NULL;

    syscfg = qdev_find_recursive(sysbus_get_default(),
                                 qemu_opt_get(opts, "stm32f405xx-syscfg"));

    STM32F405xx_SYSCFG(syscfg);

    DB_PRINT("Interrupt In\n");
}

static uint64_t stm32f405xx_exti_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F405Exti *s = opaque;

    DB_PRINT("0x%x\n", (uint) addr);

    switch (addr) {
    case EXTI_IMR:
        return s->exti_imr;
    case EXTI_EMR:
        return s->exti_emr;
    case EXTI_RTSR:
        return s->exti_rtsr;
    case EXTI_FTSR:
        return s->exti_ftsr;
    case EXTI_SWIER:
        return s->exti_swier;
    case EXTI_PR:
        return s->exti_pr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_exti_read: Bad offset %x\n", (int)addr);
        return 0;
    }
    return 0;
}

static void stm32f405xx_exti_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F405Exti *s = opaque;
    uint32_t value = (uint32_t) val64;

    DB_PRINT("0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
    case EXTI_IMR:
        s->exti_imr = value;
        return;
    case EXTI_EMR:
        s->exti_emr = value;
        return;
    case EXTI_RTSR:
        s->exti_rtsr = value;
        return;
    case EXTI_FTSR:
        s->exti_ftsr = value;
        return;
    case EXTI_SWIER:
        s->exti_swier = value;
        return;
    case EXTI_PR:
        s->exti_pr = value;
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_exti_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps stm32f405xx_exti_ops = {
    .read = stm32f405xx_exti_read,
    .write = stm32f405xx_exti_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f405xx_exti_init(Object *obj)
{
    STM32F405Exti *s = STM32F405xx_EXTI(obj);
    int i;

    s->irq = g_new0(qemu_irq, NUM_INTERRUPT_OUT_LINES);
    for (i = 0; i < NUM_INTERRUPT_OUT_LINES; i++) {
        sysbus_init_irq(SYS_BUS_DEVICE(obj), s->irq);
    }

    memory_region_init_io(&s->mmio, obj, &stm32f405xx_exti_ops, s,
                          TYPE_STM32F405xx_EXTI, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    qdev_init_gpio_in(DEVICE(obj), exti_set_irq, NUM_GPIO_EVENT_IN_LINES);
}

// 10 0000 0000 0000 -> PC[7] -> D0 -> EXTI7

static void stm32f405xx_exti_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = exti_reset;
}

static const TypeInfo stm32f405xx_exti_info = {
    .name          = TYPE_STM32F405xx_EXTI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F405Exti),
    .instance_init = stm32f405xx_exti_init,
    .class_init    = stm32f405xx_exti_class_init,
};

static void stm32f405xx_exti_register_types(void)
{
    type_register_static(&stm32f405xx_exti_info);
}

type_init(stm32f405xx_exti_register_types)
