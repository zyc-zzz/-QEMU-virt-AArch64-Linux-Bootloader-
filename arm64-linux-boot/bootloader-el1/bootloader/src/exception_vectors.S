.section .text
.align 11 
.global exception_vectors
.global exception_vector_common

/* exception_frame_t offsets */
.equ FRAME_VECTOR_ID,   0
.equ FRAME_X0,          8
.equ FRAME_X1,          16
.equ FRAME_X2,          24
.equ FRAME_X3,          32
.equ FRAME_X4,          40
.equ FRAME_X5,          48
.equ FRAME_X6,          56
.equ FRAME_X7,          64
.equ FRAME_X29,         72
.equ FRAME_X30,         80
.equ FRAME_SP,          88
.equ FRAME_ELR_EL1,     96
.equ FRAME_SPSR_EL1,    104
.equ FRAME_ESR_EL1,     112
.equ FRAME_FAR_EL1,     120
.equ FRAME_CURRENT_EL,  128
.equ FRAME_SIZE,        136

.macro VECTOR_ENTRY id
    mov     x18, #\id
    b       exception_vector_common
    .space  128 - (. - .Lvec_\id)
.endm

exception_vectors:
.Lvec_0:
    mov     x18, #0
    b       exception_vector_common
    .space  128 - (. - .Lvec_0)

.Lvec_1:
    mov     x18, #1
    b       exception_vector_common
    .space  128 - (. - .Lvec_1)

.Lvec_2:
    mov     x18, #2
    b       exception_vector_common
    .space  128 - (. - .Lvec_2)

.Lvec_3:
    mov     x18, #3
    b       exception_vector_common
    .space  128 - (. - .Lvec_3)

.Lvec_4:
    mov     x18, #4
    b       exception_vector_common
    .space  128 - (. - .Lvec_4)

.Lvec_5:
    mov     x18, #5
    b       exception_vector_common
    .space  128 - (. - .Lvec_5)

.Lvec_6:
    mov     x18, #6
    b       exception_vector_common
    .space  128 - (. - .Lvec_6)

.Lvec_7:
    mov     x18, #7
    b       exception_vector_common
    .space  128 - (. - .Lvec_7)

.Lvec_8:
    mov     x18, #8
    b       exception_vector_common
    .space  128 - (. - .Lvec_8)

.Lvec_9:
    mov     x18, #9
    b       exception_vector_common
    .space  128 - (. - .Lvec_9)

.Lvec_10:
    mov     x18, #10
    b       exception_vector_common
    .space  128 - (. - .Lvec_10)

.Lvec_11:
    mov     x18, #11
    b       exception_vector_common
    .space  128 - (. - .Lvec_11)

.Lvec_12:
    mov     x18, #12
    b       exception_vector_common
    .space  128 - (. - .Lvec_12)

.Lvec_13:
    mov     x18, #13
    b       exception_vector_common
    .space  128 - (. - .Lvec_13)

.Lvec_14:
    mov     x18, #14
    b       exception_vector_common
    .space  128 - (. - .Lvec_14)

.Lvec_15:
    mov     x18, #15
    b       exception_vector_common
    .space  128 - (. - .Lvec_15)

exception_vector_common:
    sub     sp, sp, #FRAME_SIZE

    str     x18, [sp, #FRAME_VECTOR_ID]
    str     x0,  [sp, #FRAME_X0]
    str     x1,  [sp, #FRAME_X1]
    str     x2,  [sp, #FRAME_X2]
    str     x3,  [sp, #FRAME_X3]
    str     x4,  [sp, #FRAME_X4]
    str     x5,  [sp, #FRAME_X5]
    str     x6,  [sp, #FRAME_X6]
    str     x7,  [sp, #FRAME_X7]
    str     x29, [sp, #FRAME_X29]
    str     x30, [sp, #FRAME_X30]

    add     x0, sp, #FRAME_SIZE
    str     x0, [sp, #FRAME_SP]

    mrs     x0, elr_el1
    str     x0, [sp, #FRAME_ELR_EL1]

    mrs     x0, spsr_el1
    str     x0, [sp, #FRAME_SPSR_EL1]

    mrs     x0, esr_el1
    str     x0, [sp, #FRAME_ESR_EL1]

    mrs     x0, far_el1
    str     x0, [sp, #FRAME_FAR_EL1]

    mrs     x0, CurrentEL
    str     x0, [sp, #FRAME_CURRENT_EL]

    mov     x0, sp
    bl      exception_handle

1:
    b       1b

