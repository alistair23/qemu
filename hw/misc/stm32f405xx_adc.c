/*
 * STM32F405xx ADC
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

#ifndef ST_ADC_ERR_DEBUG
#define ST_ADC_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (ST_ADC_ERR_DEBUG >= lvl) { \
        fprintf(stderr, "stm32f405xx_adc: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#define TYPE_STM32F405xx_ADC "stm32f405xx-adc"
#define STM32F405xx_ADC(obj) \
    OBJECT_CHECK(STM32f405AdcState, (obj), TYPE_STM32F405xx_ADC)

#define ADC_SR    0x00
#define ADC_CR1   0x04
#define ADC_CR2   0x08
#define ADC_SMPR1 0x0C
#define ADC_SMPR2 0x10
#define ADC_JOFR1 0x14
#define ADC_JOFR2 0x18
#define ADC_JOFR3 0x1C
#define ADC_JOFR4 0x20
#define ADC_HTR   0x24
#define ADC_LTR   0x28
#define ADC_SQR1  0x2C
#define ADC_SQR2  0x30
#define ADC_SQR3  0x34
#define ADC_JSQR  0x38
#define ADC_JDR1  0x3C
#define ADC_JDR2  0x40
#define ADC_JDR3  0x44
#define ADC_JDR4  0x48
#define ADC_DR    0x4C

#define ADC_CR2_ADON    0x01
#define ADC_CR2_CONT    0x02
#define ADC_CR2_SWSTART 0x40000000

typedef struct {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t adc_sr;
    uint32_t adc_cr1;
    uint32_t adc_cr2;
    uint32_t adc_smpr1;
    uint32_t adc_smpr2;
    uint32_t adc_jofr1;
    uint32_t adc_jofr2;
    uint32_t adc_jofr3;
    uint32_t adc_jofr4;
    uint32_t adc_htr;
    uint32_t adc_ltr;
    uint32_t adc_sqr1;
    uint32_t adc_sqr2;
    uint32_t adc_sqr3;
    uint32_t adc_jsqr;
    uint32_t adc_jdr1;
    uint32_t adc_jdr2;
    uint32_t adc_jdr3;
    uint32_t adc_jdr4;
    uint32_t adc_dr;

    qemu_irq irq;
} STM32f405AdcState;

static void stm32f405xx_adc_reset(DeviceState *dev)
{
    STM32f405AdcState *s = STM32F405xx_ADC(dev);

    s->adc_sr = 0x00000000;
    s->adc_cr1 = 0x00000000;
    s->adc_cr2 = 0x00000000;
    s->adc_smpr1 = 0x00000000;
    s->adc_smpr2 = 0x00000000;
    s->adc_jofr1 = 0x00000000;
    s->adc_jofr2 = 0x00000000;
    s->adc_jofr3 = 0x00000000;
    s->adc_jofr4 = 0x00000000;
    s->adc_htr = 0x00000FFF;
    s->adc_ltr = 0x00000000;
    s->adc_sqr1 = 0x00000000;
    s->adc_sqr2 = 0x00000000;
    s->adc_sqr3 = 0x00000000;
    s->adc_jsqr = 0x00000000;
    s->adc_jdr1 = 0x00000000;
    s->adc_jdr2 = 0x00000000;
    s->adc_jdr3 = 0x00000000;
    s->adc_jdr4 = 0x00000000;
    s->adc_dr = 0x00000000;
}

static uint32_t stm32f405xx_adc_generate_value(STM32f405AdcState *s)
{
    /* Attempts to fake some ADC values */
    s->adc_dr = s->adc_dr + rand();
    if (s->adc_dr > 0x7FFFFFFF) {
        s->adc_dr = 0;
    }
    return s->adc_dr;
}

static uint64_t stm32f405xx_adc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32f405AdcState *s = opaque;

    DB_PRINT("0x%x\n", (uint) addr);

    if (addr >= 0x100) {
        qemu_log_mask(LOG_UNIMP,
                "STM32F405xx_adc_read: ADC Common Register Unsupported\n");
    }

    switch (addr) {
    case ADC_SR:
        return s->adc_sr;
    case ADC_CR1:
        return s->adc_cr1;
    case ADC_CR2:
        return s->adc_cr2;
    case ADC_SMPR1:
        return s->adc_smpr1;
    case ADC_SMPR2:
        return s->adc_smpr2;
    case ADC_JOFR1:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jofr1;
    case ADC_JOFR2:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jofr2;
    case ADC_JOFR3:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jofr3;
    case ADC_JOFR4:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jofr4;
    case ADC_HTR:
        return s->adc_htr;
    case ADC_LTR:
        return s->adc_ltr;
    case ADC_SQR1:
        return s->adc_sqr1;
    case ADC_SQR2:
        return s->adc_sqr2;
    case ADC_SQR3:
        return s->adc_sqr3;
    case ADC_JSQR:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jsqr;
    case ADC_JDR1:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jdr1 - s->adc_jofr1;
    case ADC_JDR2:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jdr2 - s->adc_jofr2;
    case ADC_JDR3:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jdr3 - s->adc_jofr3;
    case ADC_JDR4:
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_read: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->adc_jdr4 - s->adc_jofr4;
    case ADC_DR:
        if ((s->adc_cr2 & ADC_CR2_ADON) && (s->adc_cr2 & ADC_CR2_SWSTART)) {
            if (!(s->adc_cr2 & ADC_CR2_CONT)) {
                s->adc_cr2 ^= ADC_CR2_ADON;
            }
            s->adc_cr2 ^= ADC_CR2_SWSTART;
            return stm32f405xx_adc_generate_value(s);
        } else {
            return 0x00000000;
        }
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_adc_read: Bad offset %x\n", (int)addr);
        return 0;
    }

    return 0;
}

static void stm32f405xx_adc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32f405AdcState *s = opaque;
    uint32_t value = (uint32_t) val64;

    DB_PRINT("0x%x, 0x%x\n", value, (uint) addr);

    if (addr >= 0x100) {
        qemu_log_mask(LOG_UNIMP,
                "STM32F405xx_adc_write: ADC Common Register Unsupported\n");
    }

    switch (addr) {
    case ADC_SR:
        s->adc_sr &= (value & 0x3F);
        break;
    case ADC_CR1:
        s->adc_cr1 = value;
        break;
    case ADC_CR2:
        s->adc_cr2 = value;
        break;
    case ADC_SMPR1:
        s->adc_smpr1 = value;
        break;
    case ADC_SMPR2:
        s->adc_smpr2 = value;
        break;
    case ADC_JOFR1:
        s->adc_jofr1 = (value & 0xFFF);
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JOFR2:
        s->adc_jofr2 = (value & 0xFFF);
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JOFR3:
        s->adc_jofr3 = (value & 0xFFF);
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JOFR4:
        s->adc_jofr4 = (value & 0xFFF);
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_HTR:
        s->adc_htr = value;
        break;
    case ADC_LTR:
        s->adc_ltr = value;
        break;
    case ADC_SQR1:
        s->adc_sqr1 = value;
        break;
    case ADC_SQR2:
        s->adc_sqr2 = value;
        break;
    case ADC_SQR3:
        s->adc_sqr3 = value;
        break;
    case ADC_JSQR:
        s->adc_jsqr = value;
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JDR1:
        s->adc_jdr1 = value;
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JDR2:
        s->adc_jdr2 = value;
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JDR3:
        s->adc_jdr3 = value;
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    case ADC_JDR4:
        s->adc_jdr4 = value;
        qemu_log_mask(LOG_UNIMP,"STM32F405xx_adc_write: " \
                      "Injection ADC is not implemented, the registers are " \
                      "includded for compatability\n");
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_adc_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps stm32f405xx_adc_ops = {
    .read = stm32f405xx_adc_read,
    .write = stm32f405xx_adc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f405xx_adc_init(Object *obj)
{
    STM32f405AdcState *s = STM32F405xx_ADC(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f405xx_adc_ops, s,
                          TYPE_STM32F405xx_ADC, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f405xx_adc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f405xx_adc_reset;
}

static const TypeInfo stm32f405xx_adc_info = {
    .name          = TYPE_STM32F405xx_ADC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32f405AdcState),
    .instance_init = stm32f405xx_adc_init,
    .class_init    = stm32f405xx_adc_class_init,
};

static void stm32f405xx_adc_register_types(void)
{
    type_register_static(&stm32f405xx_adc_info);
}

type_init(stm32f405xx_adc_register_types)
