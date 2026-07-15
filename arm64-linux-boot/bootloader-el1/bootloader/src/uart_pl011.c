#include <stdint.h>
#include "platform.h"
#include "uart.h"

#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x18))

#define UARTFR_TXFF (1U << 5)

void uart_init(void)
{
}

void uart_putc(char c)
{
    while (UARTFR & UARTFR_TXFF) {
    }
    UARTDR = (uint32_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void uart_put_hex64(uint64_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    for (int i = 15; i >= 0; --i) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        uart_putc(hex[nibble]);
    }
}

void uart_put_u64_dec(uint64_t value)
{
    char buf[21];
    int i = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value > 0) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
    }
}
