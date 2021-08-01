# test
.equ e1, 15
.equ e3, 0xF

.section s1

    .word 3
    push r2
    .skip 1
    
       .word -1
    and    r0  , r1
    pop r2
    not r3
    cmp  r3, r2
    int r5
    mul psw, psw
    push r2
    tst r3, r7
    pop r2
    halt
