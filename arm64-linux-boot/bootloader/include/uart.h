#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_put_hex64(uint64_t value);
void uart_put_u64_dec(uint64_t value);

#endif
