/*
 * STM32F405xx GPIO
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

#include "hw/char/stm32f405xx_gpio.h"

static const VMStateDescription vmstate_stm32f405xx_gpio = {
    .name = "stm32f405xx_gpio",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(gpio_moder, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_otyper, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_ospeedr, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_pupdr, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_idr, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_odr, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_bsrr, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_lckr, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_afrl, STM32F405GPIOState),
        VMSTATE_UINT32(gpio_afrh, STM32F405GPIOState),
        VMSTATE_END_OF_LIST()
    }
};

static void gpio_reset(DeviceState *dev)
{
    STM32F405GPIOState *s = STM32F405xx_GPIO(dev);

    if (s->gpio_letter == 'a') {
        s->gpio_moder = 0xA8000000;
        s->gpio_pupdr = 0x64000000;
        s->gpio_ospeedr = 0x00000000;
    } else if (s->gpio_letter == 'b') {
        s->gpio_moder = 0x00000280;
        s->gpio_pupdr = 0x00000100;
        s->gpio_ospeedr = 0x000000C0;
    } else {
        s->gpio_moder = 0x00000000;
        s->gpio_pupdr = 0x00000000;
        s->gpio_ospeedr = 0x00000000;
    }

    s->gpio_otyper = 0x00000000;
    s->gpio_idr = 0x00000000;
    s->gpio_odr = 0x00000000;
    s->gpio_bsrr = 0x00000000;
    s->gpio_lckr = 0x00000000;
    s->gpio_afrl = 0x00000000;
    s->gpio_afrh = 0x00000000;
    s->gpio_direction = 0x0000;
}

static uint64_t stm32f405xx_gpio_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    STM32F405GPIOState *s = (STM32F405GPIOState *)opaque;

    DB_PRINT("Read 0x%x\n", (uint) offset);

    switch (offset) {
    case GPIO_MODER:
        return s->gpio_moder;
    case GPIO_OTYPER:
        return s->gpio_otyper;
    case GPIO_OSPEEDR:
        return s->gpio_ospeedr;
    case GPIO_PUPDR:
        return s->gpio_pupdr;
    case GPIO_IDR:
        /* This register changes based on the external GPIO pins */
        return s->gpio_idr & s->gpio_direction;
    case GPIO_ODR:
        return s->gpio_odr;
    case GPIO_BSRR_HIGH:
        return 0x0000;
    case GPIO_BSRR:
        return 0x00000000;
    case GPIO_LCKR:
        return s->gpio_lckr;
    case GPIO_AFRL:
        return s->gpio_afrl;
    case GPIO_AFRH:
        return s->gpio_afrh;
    }
    return 0;
}

static void gpio_set_irq(void * opaque, int irq, int level)
{
    STM32F405GPIOState *s = (STM32F405GPIOState *)opaque;

    DB_PRINT("Interrupt In\n");
}

static void gpio_set_outputs(STM32F405GPIOState *s)
{
    int i;

    for (i = 0; i < 15; i++){
        qemu_set_irq(s->gpio_out[i], s->gpio_odr & (1U << i) & (0xFFFF ^ s->gpio_direction));
    }
}

static void stm32f405xx_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    STM32F405GPIOState *s = (STM32F405GPIOState *)opaque;
    int i, mask;

    DB_PRINT("Write 0x%x, 0x%x\n", (uint) value, (uint) offset);

    switch (offset) {
    case GPIO_MODER:
        s->gpio_moder = (uint32_t) value;
        for (i = 0; i < 15; i++) {
            /* Two bits determine the I/O direction/mode */
            mask = 3U << (i * 2);

            if ((s->gpio_moder & mask) == GPIO_MODER_INPUT) {
                s->gpio_direction |= (1 << i);
            } else if ((s->gpio_moder & mask) == GPIO_MODER_GENERAL_OUT) {
                s->gpio_direction &= (0xFFFF ^ (1 << i));
            } else {
                /* Not supported at the moment */
            }
        }
        return;
    case GPIO_OTYPER:
        s->gpio_otyper = (uint32_t) value;
        return;
    case GPIO_OSPEEDR:
        s->gpio_ospeedr = (uint32_t) value;
        return;
    case GPIO_PUPDR:
        s->gpio_pupdr = (uint32_t) value;
        return;
    case GPIO_IDR:
        /* Read Only Register */
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_gpio%c_write: Read Only Register 0x%x\n",
                      s->gpio_letter, (int)offset);
        return;
    case GPIO_ODR:
        s->gpio_odr = ((uint32_t) value & (s->gpio_direction ^ 0xFFFF));
        gpio_set_outputs(s);
        return;
    case GPIO_BSRR_HIGH:
        /* Reset the output value */
        s->gpio_odr &= (uint32_t) (value ^ 0xFFFF);
        s->gpio_bsrr = (uint32_t) (value << 16);
        gpio_set_outputs(s);
        DB_PRINT("Output: 0x%x\n", s->gpio_odr);
        return;
    case GPIO_BSRR:
        /* Top 16 bits are "write one to clear output" */
        s->gpio_odr &= (uint32_t) ((value >> 16) ^ 0xFFFF);
        /* Bottom 16 bits are "write one to set output" */
        s->gpio_odr |= (uint32_t) (value & 0xFFFF);
        s->gpio_bsrr = (uint32_t) value;
        gpio_set_outputs(s);
        DB_PRINT("Output: 0x%x\n", s->gpio_odr);
        return;
    case GPIO_LCKR:
        s->gpio_lckr = (uint32_t) value;
        /* Unimplemented */
        return;
    case GPIO_AFRL:
        s->gpio_afrl = (uint32_t) value;
        return;
    case GPIO_AFRH:
        s->gpio_afrh = (uint32_t) value;
        return;
    }
}

static const MemoryRegionOps stm32f405xx_gpio_ops = {
    .read = stm32f405xx_gpio_read,
    .write = stm32f405xx_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property stm32f405_gpio_properties[] = {
    DEFINE_PROP_UINT8("gpio-letter", STM32F405GPIOState, gpio_letter,
                      (uint) 'a'),
    DEFINE_PROP_END_OF_LIST(),
};


static void stm32f405xx_gpio_initfn(Object *obj)
{
    STM32F405GPIOState *s = STM32F405xx_GPIO(obj);

    memory_region_init_io(&s->iomem, obj, &stm32f405xx_gpio_ops, s,
                          "stm32f405xx_gpio", 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    qdev_init_gpio_in(DEVICE(obj), gpio_set_irq, 15);
    qdev_init_gpio_out(DEVICE(obj), s->gpio_out, 15);
}

static void stm32f405xx_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_stm32f405xx_gpio;
    dc->props = stm32f405_gpio_properties;
    dc->reset = gpio_reset;
}

static const TypeInfo stm32f405xx_gpio_info = {
    .name          = TYPE_STM32F405xx_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F405GPIOState),
    .instance_init = stm32f405xx_gpio_initfn,
    .class_init    = stm32f405xx_gpio_class_init,
};

static void stm32f405xx_gpio_register_types(void)
{
    type_register_static(&stm32f405xx_gpio_info);
}

type_init(stm32f405xx_gpio_register_types)
