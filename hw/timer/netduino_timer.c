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
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#define DEBUG_NETTIMER

#ifdef DEBUG_NETTIMER
#define DPRINTF(fmt, ...) \
do { printf("netduino_timer: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

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

#define TYPE_NETTIMER "netduino_timer"
#define NETTIMER(obj) OBJECT_CHECK(NETTIMERState, (obj), TYPE_NETTIMER)

typedef struct NETTIMERState {
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
} NETTIMERState;

static void netduino_timer_update(NETTIMERState *s)
{
    s->tim_sr |= 1;
    qemu_set_irq(s->irq, 1);
    qemu_set_irq(s->irq, 0);
}

static void netduino_timer_interrupt(void * opaque)
{
    NETTIMERState *s = (NETTIMERState *)opaque;

    DPRINTF("INterrupt\n");

    if (s->tim_dier == 0x01 && s->tim_cr1 & TIM_CR1_CEN) {
        netduino_timer_update(s);
    }
}

static uint32_t netduino_timer_get_count(NETTIMERState *s)
{
    int64_t now = qemu_clock_get_ns(rtc_clock);
    return s->tick_offset + now / get_ticks_per_sec();
}

static void netduino_timer_set_alarm(NETTIMERState *s)
{
    uint32_t ticks;

    DPRINTF("Alarm raised: 0x%x\n", s->tim_cr1);

    ticks = (uint32_t) (s->tim_arr - netduino_timer_get_count(s)/(s->tim_psc + 1));
    DPRINTF("Alarm set in %u ticks\n", ticks);
    if (ticks <= 0) {
        timer_del(s->timer);
        netduino_timer_interrupt(s);
    } else {
        int64_t now = qemu_clock_get_ns(rtc_clock);
        timer_mod(s->timer, now + (int64_t)ticks);
        DPRINTF("Wait Time: 0x%x\n", now + (int64_t)ticks);
    }
}

static void netduino_timer_reset(DeviceState *dev)
{
    struct NETTIMERState *s = NETTIMER(dev);

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

static uint64_t netduino_timer_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    NETTIMERState *s = (NETTIMERState *)opaque;

    DPRINTF("Read 0x%x\n", (uint) offset);

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
    }

    return 0;
}

static void netduino_timer_write(void * opaque, hwaddr offset,
                        uint64_t val64, unsigned size)
{
    NETTIMERState *s = (NETTIMERState *)opaque;
    uint32_t value = (uint32_t) val64;

    DPRINTF("Write 0x%x, 0x%x\n", value, (uint) offset);

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
            netduino_timer_set_alarm(s);
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
            netduino_timer_set_alarm(s);
            return;
        case TIM_PSC:
            s->tim_psc = value;
            return;
        case TIM_ARR:
            s->tim_arr = value;
            netduino_timer_set_alarm(s);
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
    }
}

static const MemoryRegionOps netduino_timer_ops = {
    .read = netduino_timer_read,
    .write = netduino_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int netduino_timer_init(SysBusDevice *dev)
{
    NETTIMERState *s = NETTIMER(dev);
    struct tm tm;

    sysbus_init_irq(dev, &s->irq);

    memory_region_init_io(&s->iomem, OBJECT(s), &netduino_timer_ops, s,
                          "netduino_timer", 0x2000);
    sysbus_init_mmio(dev, &s->iomem);

    qemu_get_timedate(&tm, 0);
    s->tick_offset = mktimegm(&tm) -
        qemu_clock_get_ns(rtc_clock) / get_ticks_per_sec();

    s->timer = timer_new_ns(rtc_clock, netduino_timer_interrupt, s);

    return 0;
}

static void netduino_timer_pre_save(void *opaque)
{
    NETTIMERState *s = (NETTIMERState *)opaque;

    /* tick_offset is base_time - rtc_clock base time.  Instead, we want to
     * store the base time relative to the QEMU_CLOCK_VIRTUAL for backwards-compatibility.  */
    int64_t delta = qemu_clock_get_ns(rtc_clock) - qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->tick_offset_vmstate = s->tick_offset + delta / get_ticks_per_sec();
}

static int netduino_timer_post_load(void *opaque, int version_id)
{
    NETTIMERState *s = (NETTIMERState *)opaque;

    int64_t delta = qemu_clock_get_ns(rtc_clock) - qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->tick_offset = s->tick_offset_vmstate - delta / get_ticks_per_sec();
    netduino_timer_set_alarm(s);
    return 0;
}

static const VMStateDescription vmstate_netduino_timer = {
    .name = "netduino_timer",
    .version_id = 1,
    .minimum_version_id = 1,
    .pre_save = netduino_timer_pre_save,
    .post_load = netduino_timer_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(tick_offset_vmstate, NETTIMERState),
        VMSTATE_UINT32(tim_cr1, NETTIMERState),
        VMSTATE_UINT32(tim_cr2, NETTIMERState),
        VMSTATE_UINT32(tim_smcr, NETTIMERState),
        VMSTATE_UINT32(tim_dier, NETTIMERState),
        VMSTATE_UINT32(tim_sr, NETTIMERState),
        VMSTATE_UINT32(tim_egr, NETTIMERState),
        VMSTATE_UINT32(tim_ccmr1, NETTIMERState),
        VMSTATE_UINT32(tim_ccmr1, NETTIMERState),
        VMSTATE_UINT32(tim_ccer, NETTIMERState),
        VMSTATE_UINT32(tim_cnt, NETTIMERState),
        VMSTATE_UINT32(tim_psc, NETTIMERState),
        VMSTATE_UINT32(tim_arr, NETTIMERState),
        VMSTATE_UINT32(tim_ccr1, NETTIMERState),
        VMSTATE_UINT32(tim_ccr2, NETTIMERState),
        VMSTATE_UINT32(tim_ccr3, NETTIMERState),
        VMSTATE_UINT32(tim_ccr4, NETTIMERState),
        VMSTATE_UINT32(tim_dcr, NETTIMERState),
        VMSTATE_UINT32(tim_dmar, NETTIMERState),
        VMSTATE_UINT32(tim_or, NETTIMERState),
        VMSTATE_END_OF_LIST()
    }
};

static void netduino_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = netduino_timer_init;
    dc->vmsd = &vmstate_netduino_timer;
    dc->reset = netduino_timer_reset;
}

static const TypeInfo netduino_timer_info = {
    .name          = TYPE_NETTIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NETTIMERState),
    .class_init    = netduino_timer_class_init,
};

static void netduino_timer_register_types(void)
{
    type_register_static(&netduino_timer_info);
}

type_init(netduino_timer_register_types)
