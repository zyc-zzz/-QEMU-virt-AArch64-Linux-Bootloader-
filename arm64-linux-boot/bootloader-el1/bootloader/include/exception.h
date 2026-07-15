#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdint.h>

typedef struct {
    uint64_t vector_id;
    uint64_t x0; //参数寄存器 x0~x7
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
    uint64_t x29; //帧指针 x29
    uint64_t x30; //返回地址 x30
    uint64_t sp; //栈指针 sp
    uint64_t elr_el1;
    uint64_t spsr_el1;
    uint64_t esr_el1;
    uint64_t far_el1;
    uint64_t current_el;
} exception_frame_t;

void exception_init(void);
void exception_handle(exception_frame_t *frame);

#endif

