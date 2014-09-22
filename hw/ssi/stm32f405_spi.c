/*
 * STM32F405 SPI
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

#include "hw/ssi/stm32f405_spi.h"

#ifndef STM_SPI_ERR_DEBUG
#define STM_SPI_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_SPI_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f405_spi_reset(DeviceState *dev)
{
    STM32f405SpiState *s = STM32F405_SPI(dev);

    s->spi_cr1 = 0x00000000;
    s->spi_cr2 = 0x00000000;
    s->spi_sr = 0x0000000A;
    s->spi_dr = 0x0000000C;
    s->spi_crcpr = 0x00000007;
    s->spi_rxcrcr = 0x00000000;
    s->spi_txcrcr = 0x00000000;
    s->spi_i2scfgr = 0x00000000;
    s->spi_i2spr = 0x00000002;
}

static void stm32f405_spi_transfer(STM32f405SpiState *s)
{
    DB_PRINT("Data to sent: 0x%x\n", s->spi_dr);

    s->spi_dr = ssi_transfer(s->ssi, s->spi_dr);
    s->spi_sr |= SPI_SR_RXNE;

    DB_PRINT("Data recieved: 0x%x\n", s->spi_dr);
}

static uint64_t stm32f405_spi_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32f405SpiState *s = opaque;
    uint32_t retval;

    DB_PRINT("0x%x\n", (uint) addr);

    switch (addr) {
    case SPI_CR1:
        return s->spi_cr1;
    case SPI_CR2:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_read: " \
                      "Interrupts and DMA are not implemented\n");
        return s->spi_cr2;
    case SPI_SR:
        retval = s->spi_sr;
        return retval;
    case SPI_DR:
        s->spi_sr |= SPI_SR_BSY;
        stm32f405_spi_transfer(s);
        s->spi_sr &= ~SPI_SR_RXNE;
        s->spi_sr &= ~SPI_SR_BSY;
        return s->spi_dr;
    case SPI_CRCPR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_read: " \
                      "CRC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->spi_crcpr;
    case SPI_RXCRCR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_read: " \
                      "CRC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->spi_rxcrcr;
    case SPI_TXCRCR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_read: " \
                      "CRC is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->spi_txcrcr;
    case SPI_I2SCFGR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_read: " \
                      "I2S is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->spi_i2scfgr;
    case SPI_I2SPR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_read: " \
                      "I2S is not implemented, the registers are " \
                      "includded for compatability\n");
        return s->spi_i2spr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405_spi_read: Bad offset %x\n", (int)addr);
        return 0;
    }

    return 0;
}

static void stm32f405_spi_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32f405SpiState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("0x%x, 0x%x\n", value, (uint) addr);

    switch (addr) {
    case SPI_CR1:
        s->spi_cr1 = value;
        return;
    case SPI_CR2:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_write: " \
                      "Interrupts and DMA are not implemented\n");
        s->spi_cr2 = value;
        return;
    case SPI_SR:
        /* Read only register, except for clearing the CRCERR bit, which
         * is not supported
         */
        return;
    case SPI_DR:
        s->spi_sr |= SPI_SR_BSY;
        s->spi_sr &= ~SPI_SR_TXE;
        s->spi_dr = value;
        stm32f405_spi_transfer(s);
        s->spi_sr |= SPI_SR_TXE;
        s->spi_sr &= ~SPI_SR_BSY;
        return;
    case SPI_CRCPR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_write: " \
                      "CRC is not implemented\n");
        return;
    case SPI_RXCRCR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405_spi_write: Read only register: " \
                      "0x%"HWADDR_PRIx"\n", addr);
        return;
    case SPI_TXCRCR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405_spi_write: Read only register: " \
                      "0x%"HWADDR_PRIx"\n", addr);
        return;
    case SPI_I2SCFGR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_write: " \
                      "I2S is not implemented\n");
        return;
    case SPI_I2SPR:
        qemu_log_mask(LOG_UNIMP,"STM32F405_spi_write: " \
                      "I2S is not implemented\n");
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "STM32F405_spi_write: Bad offset %x\n", (int)addr);
    }
}

static const MemoryRegionOps stm32f405_spi_ops = {
    .read = stm32f405_spi_read,
    .write = stm32f405_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f405_spi_init(Object *obj)
{
    STM32f405SpiState *s = STM32F405_SPI(obj);
    DeviceState *dev = DEVICE(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f405_spi_ops, s,
                          TYPE_STM32F405_SPI, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    s->ssi = ssi_create_bus(dev, "ssi");
}

static void stm32f405_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f405_spi_reset;
}

static const TypeInfo stm32f405_spi_info = {
    .name          = TYPE_STM32F405_SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32f405SpiState),
    .instance_init = stm32f405_spi_init,
    .class_init    = stm32f405_spi_class_init,
};

static void stm32f405_spi_register_types(void)
{
    type_register_static(&stm32f405_spi_info);
}

type_init(stm32f405_spi_register_types)
