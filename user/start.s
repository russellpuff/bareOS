    .globl _start
_start:
    call main
    li a7, 93 # ECALL_EXIT
    li a0, 0
    ecall
