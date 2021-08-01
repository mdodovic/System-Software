 	.intel_syntax noprefix
        .section .rodata
msg:
        .string "Hi\n"
        .text
        .globl  main
main:
        push    rbp
        mov     rbp, rsp
        call    getchar
        cmp     eax, 'A'
        jne     skip
        mov     edi, OFFSET msg
        call    printf
skip:
        mov     eax, 0
        mov     rsp, rbp
        pop     rbp
        ret
