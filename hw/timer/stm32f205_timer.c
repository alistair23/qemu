/*
 * STM32F205 Timer
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

#include "hw/timer/stm32f205_timer.h"

#ifndef STM_TIMER_ERR_DEBUG
#define STM_TIMER_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_TIMER_ERR_DEBUG >= lvl) { \
        qemu_log("stm32f205_timer: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f205_timer_interrupt(void *opaque)
{
    STM32f205TimerState *s = opaque;

    DB_PRINT("Interrupt in: %s\n", __func__);

    if (s->tim_dier & TIM_DIER_UIE && s->tim_cr1 & TIM_CR1_CEN) {
        s->tim_sr |= 1;
        qemu_irq_pulse(s->irq);
    }
}

static void stm32f205_timer_set_alarm(STM32f205TimerState *s)
{
    uint32_t ticks;
    int64_t now;

    DB_PRINT("Alarm raised in: %s at 0x%x\n", __func__, s->tim_cr1);

    now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    ticks = s->tim_arr - (s->tick_offset + (now / get_ticks_per_sec())) *
            (s->tim_psc + 1);

    DB_PRINT("Alarm set in %d ticks\n", ticks);

    if (ticks == 0) {
        timer_del(s->timer);
        stm32f205_timer_interrupt(s);
    } else {
        timer_mod(s->timer, (now + (int64_t) ticks));
        DB_PRINT("Wait Time: %u\n", (uint32_t) (now + ticks));
    }
}

static void stm32f205_timer_reset(DeviceState *dev)
{
    STM32f205TimerState *s = STM32F205TIMER(dev);

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

static uint64_t stm32f205_timer_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    STM32f205TimerState *s = opaque;

    DB_PRINT("Read 0x%"HWADDR_PRIx"\n", offset);

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
                      "STM32F205_timer_write: Bad offset %x\n", (int) offset);
    }

    return 0;
}

static void stm32f205_timer_write(void *opaque, hwaddr offset,
                        uint64_t val64, unsigned size)
{
    STM32f205TimerState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("Write 0x%x, 0x%"HWADDR_PRIx"\n", value, offset);

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
        stm32f205_timer_set_alarm(s);
        return;
    case TIM_EGR:
        s->tim_egr = value;
        if (s->tim_egr & 1) {
            /* Re-init the counter */
            stm32f205_timer_reset(DEVICE(s));
        }
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
        stm32f205_timer_set_alarm(s);
        return;
    case TIM_PSC:
        s->tim_psc = value;
        return;
    case TIM_ARR:
        s->tim_arr = value;
        stm32f205_timer_set_alarm(s);
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
                      "STM32F205_timer_write: Bad offset %x\n", (int) offset);
    }
}

static const MemoryRegionOps stm32f205_timer_ops = {
    .read = stm32f205_timer_read,
    .write = stm32f205_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f205_timer_init(Object *obj)
{
    STM32f205TimerState *s = STM32F205TIMER(obj);
    struct tm tm;

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->iomem, obj, &stm32f205_timer_ops, s,
                          "stm32f205_timer", 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);

    qemu_get_timedate(&tm, 0);
    s->tick_offset = mktimegm(&tm) -
        qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) / get_ticks_per_sec();

    s->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, stm32f205_timer_interrupt, s);
}

static const VMStateDescription vmstate_stm32f205_timer = {
    .name = "stm32f205_timer",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(tick_offset_vmstate, STM32f205TimerState),
        VMSTATE_UINT32(tim_cr1, STM32f205TimerState),
        VMSTATE_UINT32(tim_cr2, STM32f205TimerState),
        VMSTATE_UINT32(tim_smcr, STM32f205TimerState),
        VMSTATE_UINT32(tim_dier, STM32f205TimerState),
        VMSTATE_UINT32(tim_sr, STM32f205TimerState),
        VMSTATE_UINT32(tim_egr, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccmr1, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccmr1, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccer, STM32f205TimerState),
        VMSTATE_UINT32(tim_cnt, STM32f205TimerState),
        VMSTATE_UINT32(tim_psc, STM32f205TimerState),
        VMSTATE_UINT32(tim_arr, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccr1, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccr2, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccr3, STM32f205TimerState),
        VMSTATE_UINT32(tim_ccr4, STM32f205TimerState),
        VMSTATE_UINT32(tim_dcr, STM32f205TimerState),
        VMSTATE_UINT32(tim_dmar, STM32f205TimerState),
        VMSTATE_UINT32(tim_or, STM32f205TimerState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f205_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_stm32f205_timer;
    dc->reset = stm32f205_timer_reset;
}

static const TypeInfo stm32f205_timer_info = {
    .name          = TYPE_STM32F205_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32f205TimerState),
    .instance_init = stm32f205_timer_init,
    .class_init    = stm32f205_timer_class_init,
};

static void stm32f205_timer_register_types(void)
{
    type_register_static(&stm32f205_timer_info);
}

type_init(stm32f205_timer_register_types)
