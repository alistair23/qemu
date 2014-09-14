/*
 * NRF24L01+ SPI Slave
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

#include "hw/ssi.h"

#ifndef NRF24L01PLUS_ERR_DEBUG
#define NRF24L01PLUS_ERR_DEBUG 1
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (NRF24L01PLUS_ERR_DEBUG >= lvl) { \
        qemu_log("NRF24L01+: %s:" fmt, __func__, ## args); \
    } \
} while (0);

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

typedef struct {
    SSISlave ssidev;
} nrf24l01plus_state;

static uint32_t nrf24l01plus_transfer(SSISlave *dev, uint32_t val)
{
    return 0;
}

static int nrf24l01plus_init(SSISlave *d)
{
    //DeviceState *dev = DEVICE(d);
    //nrf24l01plus_state *s = FROM_SSI_SLAVE(nrf24l01plus_state, d);

    return 0;
}

static void nrf24l01plus_class_init(ObjectClass *klass, void *data)
{
    SSISlaveClass *k = SSI_SLAVE_CLASS(klass);

    k->init = nrf24l01plus_init;
    k->transfer = nrf24l01plus_transfer;
    k->cs_polarity = SSI_CS_LOW;
}

static const TypeInfo nrf24l01plus_info = {
    .name          = "nrf24l01plus",
    .parent        = TYPE_SSI_SLAVE,
    .instance_size = sizeof(nrf24l01plus_state),
    .class_init    = nrf24l01plus_class_init,
};

static void nrf24l01plus_register_types(void)
{
    type_register_static(&nrf24l01plus_info);
}

type_init(nrf24l01plus_register_types)
