/*
 * STM32F405xx Timer 2 to 5
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
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#ifndef ST_TIM2_5_ERR_DEBUG
#define ST_TIM2_5_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (ST_TIM2_5_ERR_DEBUG >= lvl) { \
        fprintf(stderr, "stm32f405xx_timer: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#define TIM_CR1      0x00
#define TIM_CR2      0x04
#define TIM_SMCR     0x08
#define TIM_DIER     0x0C
#define TIM_SR       0x10
#define TIM_EGR      0x14
#define TIM_CCMR1    0x18
#define TIM_CCMR2    0x1C
#define TIM_CCER     0x20
#define TIM_CNT      0x24
#define TIM_PSC      0x28
#define TIM_ARR      0x2C
#define TIM_CCR1     0x34
#define TIM_CCR2     0x38
#define TIM_CCR3     0x3C
#define TIM_CCR4     0x40
#define TIM_DCR      0x48
#define TIM_DMAR     0x4C
#define TIM_OR       0x50

#define TIM_CR1_CEN   1

#define TYPE_STM32F405xxTIMER "stn32f405xx-timer"
#define STM32F405xxTIMER(obj) OBJECT_CHECK(STM32F405Timer, (obj), TYPE_STM32F405xxTIMER)

typedef struct STM32F405Timer {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    QEMUTimer *timer;
    qemu_irq irq;

    uint32_t tick_offset_vmstate;
    uint32_t tick_offset;

    uint32_t tim_cr1;
    uint32_t tim_cr2;
    uint32_t tim_smcr;
    uint32_t tim_dier;
    uint32_t tim_sr;
    uint32_t tim_egr;
    uint32_t tim_ccmr1;
    uint32_t tim_ccmr2;
    uint32_t tim_ccer;
    uint32_t tim_cnt;
    uint32_t tim_psc;
    uint32_t tim_arr;
    uint32_t tim_ccr1;
    uint32_t tim_ccr2;
    uint32_t tim_ccr3;
    uint32_t tim_ccr4;
    uint32_t tim_dcr;
    uint32_t tim_dmar;
    uint32_t tim_or;
} STM32F405Timer;

static void stn32f405xx_timer_update(STM32F405Timer *s)
{
    s->tim_sr |= 1;
    qemu_irq_pulse(s->irq);
}

static void stn32f405xx_timer_interrupt(void *opaque)
{
    STM32F405Timer *s = (STM32F405Timer *)opaque;

    DB_PRINT("Interrupt in: %s\n", __func__);

    if (s->tim_dier == 0x01 && s->tim_cr1 & TIM_CR1_CEN) {
        stn32f405xx_timer_update(s);
    }
}

static uint32_t stn32f405xx_timer_get_count(STM32F405Timer *s)
{
    int64_t now = qemu_clock_get_ns(rtc_clock);
    return s->tick_offset + now / get_ticks_per_sec();
}

static void stn32f405xx_timer_set_alarm(STM32F405Timer *s)
{
    uint32_t ticks;

    DB_PRINT("Alarm raised in: %s at 0x%x\n", __func__, s->tim_cr1);

    ticks = s->tim_arr - stn32f405xx_timer_get_count(s)/
                         (s->tim_psc + 1);
    DB_PRINT("Alarm set in %u ticks\n", ticks);

    if (ticks == 0) {
        timer_del(s->timer);
        stn32f405xx_timer_interrupt(s);
    } else {
        int64_t now = qemu_clock_get_ns(rtc_clock) / get_ticks_per_sec();
        timer_mod(s->timer, now + (int64_t)ticks);
        DB_PRINT("Wait Time: 0x%x\n", (uint32_t) (now + ticks));
    }
}

static void stn32f405xx_timer_reset(DeviceState *dev)
{
    struct STM32F405Timer *s = STM32F405xxTIMER(dev);

    s->tim_cr1 = 0;
    s->tim_cr2 = 0;
    s->tim_smcr = 0;
    s->tim_dier = 0;
    s->tim_sr = 0;
    s->tim_egr = 0;
    s->tim_ccmr1 = 0;
    s->tim_ccmr2 = 0;
    s->tim_ccer = 0;
    s->tim_cnt = 0;
    s->tim_psc = 0;
    s->tim_arr = 0;
    s->tim_ccr1 = 0;
    s->tim_ccr2 = 0;
    s->tim_ccr3 = 0;
    s->tim_ccr4 = 0;
    s->tim_dcr = 0;
    s->tim_dmar = 0;
    s->tim_or = 0;
}

static uint64_t stn32f405xx_timer_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    STM32F405Timer *s = (STM32F405Timer *)opaque;

    DB_PRINT("Read 0x%x\n", (uint) offset);

    switch (offset) {
    case TIM_CR1:
        return s->tim_cr1;
    case TIM_CR2:
        return s->tim_cr2;
    case TIM_SMCR:
        return s->tim_smcr;
    case TIM_DIER:
        return s->tim_dier;
    case TIM_SR:
        return s->tim_sr;
    case TIM_EGR:
        return s->tim_egr;
    case TIM_CCMR1:
        return s->tim_ccmr1;
    case TIM_CCMR2:
        return s->tim_ccmr2;
    case TIM_CCER:
        return s->tim_ccer;
    case TIM_CNT:
        return s->tim_cnt;
    case TIM_PSC:
        return s->tim_psc;
    case TIM_ARR:
        return s->tim_arr;
    case TIM_CCR1:
        return s->tim_ccr1;
    case TIM_CCR2:
        return s->tim_ccr2;
    case TIM_CCR3:
        return s->tim_ccr3;
    case TIM_CCR4:
        return s->tim_ccr4;
    case TIM_DCR:
        return s->tim_dcr;
    case TIM_DMAR:
        return s->tim_dmar;
    case TIM_OR:
        return s->tim_or;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_timer2_5_write: Bad offset %x\n", (int) offset);
    }

    return 0;
}

static void stn32f405xx_timer_write(void *opaque, hwaddr offset,
                        uint64_t val64, unsigned size)
{
    STM32F405Timer *s = (STM32F405Timer *)opaque;
    uint32_t value = (uint32_t) val64;

    DB_PRINT("Write 0x%x, 0x%x\n", value, (uint) offset);

    switch (offset) {
    case TIM_CR1:
        s->tim_cr1 = value;
        return;
    case TIM_CR2:
        s->tim_cr2 = value;
        return;
    case TIM_SMCR:
        s->tim_smcr = value;
        return;
    case TIM_DIER:
        s->tim_dier = value;
        return;
    case TIM_SR:
        s->tim_sr &= value;
        stn32f405xx_timer_set_alarm(s);
        return;
    case TIM_EGR:
        s->tim_egr = value;
        return;
    case TIM_CCMR1:
        s->tim_ccmr1 = value;
        return;
    case TIM_CCMR2:
        s->tim_ccmr2 = value;
        return;
    case TIM_CCER:
        s->tim_ccer = value;
        return;
    case TIM_CNT:
        s->tim_cnt = value;
        stn32f405xx_timer_set_alarm(s);
        return;
    case TIM_PSC:
        s->tim_psc = value;
        return;
    case TIM_ARR:
        s->tim_arr = value;
        stn32f405xx_timer_set_alarm(s);
        return;
    case TIM_CCR1:
        s->tim_ccr1 = value;
        return;
    case TIM_CCR2:
        s->tim_ccr2 = value;
        return;
    case TIM_CCR3:
        s->tim_ccr3 = value;
        return;
    case TIM_CCR4:
        s->tim_ccr4 = value;
        return;
    case TIM_DCR:
        s->tim_dcr = value;
        return;
    case TIM_DMAR:
        s->tim_dmar = value;
        return;
    case TIM_OR:
        s->tim_or = value;
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405xx_timer2_5_write: Bad offset %x\n", (int) offset);
    }
}

static const MemoryRegionOps stn32f405xx_timer_ops = {
    .read = stn32f405xx_timer_read,
    .write = stn32f405xx_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stn32f405xx_timer_init(Object *obj)
{
    STM32F405Timer *s = STM32F405xxTIMER(obj);
    struct tm tm;

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->iomem, obj, &stn32f405xx_timer_ops, s,
                          "stn32f405xx_timer", 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);

    qemu_get_timedate(&tm, 0);
    s->tick_offset = mktimegm(&tm) -
        qemu_clock_get_ns(rtc_clock) / get_ticks_per_sec();

    s->timer = timer_new_ns(rtc_clock, stn32f405xx_timer_interrupt, s);
}

static void stn32f405xx_timer_pre_save(void *opaque)
{
    STM32F405Timer *s = (STM32F405Timer *)opaque;

    int64_t delta = qemu_clock_get_ns(rtc_clock) -
                    qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->tick_offset_vmstate = s->tick_offset + delta / get_ticks_per_sec();
}

static int stn32f405xx_timer_post_load(void *opaque, int version_id)
{
    STM32F405Timer *s = (STM32F405Timer *)opaque;

    int64_t delta = qemu_clock_get_ns(rtc_clock) -
                    qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->tick_offset = s->tick_offset_vmstate - delta / get_ticks_per_sec();
    stn32f405xx_timer_set_alarm(s);
    return 0;
}

static const VMStateDescription vmstate_stn32f405xx_timer = {
    .name = "stn32f405xx_timer",
    .version_id = 1,
    .minimum_version_id = 1,
    .pre_save = stn32f405xx_timer_pre_save,
    .post_load = stn32f405xx_timer_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(tick_offset_vmstate, STM32F405Timer),
        VMSTATE_UINT32(tim_cr1, STM32F405Timer),
        VMSTATE_UINT32(tim_cr2, STM32F405Timer),
        VMSTATE_UINT32(tim_smcr, STM32F405Timer),
        VMSTATE_UINT32(tim_dier, STM32F405Timer),
        VMSTATE_UINT32(tim_sr, STM32F405Timer),
        VMSTATE_UINT32(tim_egr, STM32F405Timer),
        VMSTATE_UINT32(tim_ccmr1, STM32F405Timer),
        VMSTATE_UINT32(tim_ccmr1, STM32F405Timer),
        VMSTATE_UINT32(tim_ccer, STM32F405Timer),
        VMSTATE_UINT32(tim_cnt, STM32F405Timer),
        VMSTATE_UINT32(tim_psc, STM32F405Timer),
        VMSTATE_UINT32(tim_arr, STM32F405Timer),
        VMSTATE_UINT32(tim_ccr1, STM32F405Timer),
        VMSTATE_UINT32(tim_ccr2, STM32F405Timer),
        VMSTATE_UINT32(tim_ccr3, STM32F405Timer),
        VMSTATE_UINT32(tim_ccr4, STM32F405Timer),
        VMSTATE_UINT32(tim_dcr, STM32F405Timer),
        VMSTATE_UINT32(tim_dmar, STM32F405Timer),
        VMSTATE_UINT32(tim_or, STM32F405Timer),
        VMSTATE_END_OF_LIST()
    }
};

static void stn32f405xx_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_stn32f405xx_timer;
    dc->reset = stn32f405xx_timer_reset;
}

static const TypeInfo stn32f405xx_timer_info = {
    .name          = TYPE_STM32F405xxTIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F405Timer),
    .instance_init = stn32f405xx_timer_init,
    .class_init    = stn32f405xx_timer_class_init,
};

static void stn32f405xx_timer_register_types(void)
{
    type_register_static(&stn32f405xx_timer_info);
}

type_init(stn32f405xx_timer_register_types)
