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
#define ST_ADC_ERR_DEBUG 1
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (ST_ADC_ERR_DEBUG >= lvl) { \
        fprintf(stderr, "stm32f405xx_adc: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

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
#define ADC_CSR   0x300
#define ADC_CCR   0x304
#define ADC_CDR   0x308

#define TYPE_STM32F405xx_ADC "stm32f405xx-adc"
#define STM32F405xx_ADC(obj) \
    OBJECT_CHECK(Stm32f405AdcState, (obj), TYPE_STM32F405xx_ADC)

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

    uint32_t adc_csr;
    uint32_t adc_ccr;
    uint32_t adc_cdr;

    qemu_irq irq;
} Stm32f405AdcState;

static void adc_reset(DeviceState *dev)
{
    Stm32f405AdcState *s = STM32F405xx_ADC(dev);

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

    s->adc_csr = 0x00000000;
    s->adc_ccr = 0x00000000;
    s->adc_cdr = 0x00000000;
}

static uint64_t stm32f405xx_adc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    Stm32f405AdcState *s = opaque;

    DB_PRINT("0x%x\n", (uint) addr);

    switch (addr) {
    case ADC_MEMRMP:
        return s->adc_memrmp;
    case ADC_PMC:
        return s->adc_pmc;
    case ADC_EXTICR1:
        return s->adc_exticr1;
    case ADC_EXTICR2:
        return s->adc_exticr2;
    case ADC_EXTICR3:
        return s->adc_exticr3;
    case ADC_EXTICR4:
        return s->adc_exticr4;
    case ADC_CMPCR:
        return s->adc_cmpcr;
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
    Stm32f405AdcState *s = opaque;
    uint32_t value = (uint32_t) val64;

    DB_PRINT("0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
    case ADC_MEMRMP:
        qemu_log_mask(LOG_UNIMP,
                      "STM32F405xx_adc_write: Changeing the memory mapping " \
                      "isn't supported in QEMU\n");
        return;
    case ADC_PMC:
        qemu_log_mask(LOG_UNIMP,
                      "STM32F405xx_adc_write: Peripheral mode configuration " \
                      "isn't supported in QEMU\n");
        return;
    case ADC_EXTICR1:
        s->adc_exticr1 = (value & 0xFF);
        return;
    case ADC_EXTICR2:
        s->adc_exticr2 = (value & 0xFF);
        return;
    case ADC_EXTICR3:
        s->adc_exticr3 = (value & 0xFF);
        return;
    case ADC_EXTICR4:
        s->adc_exticr4 = (value & 0xFF);
        return;
    case ADC_CMPCR:
        s->adc_cmpcr = value;
        return;
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
    Stm32f405AdcState *s = STM32F405xx_ADC(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f405xx_adc_ops, s,
                          TYPE_STM32F405xx_ADC, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f405xx_adc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = adc_reset;
}

static const TypeInfo stm32f405xx_adc_info = {
    .name          = TYPE_STM32F405xx_ADC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Stm32f405AdcState),
    .instance_init = stm32f405xx_adc_init,
    .class_init    = stm32f405xx_adc_class_init,
};

static void stm32f405xx_adc_register_types(void)
{
    type_register_static(&stm32f405xx_adc_info);
}

type_init(stm32f405xx_adc_register_types)
