    .globl _start
_start:
    call main
    li a7, 93 # placeholder exit
    li a0, 0
    ecall
