# test
.equ e1, 15
.equ e3, 0xF
.section ivt
   push r0
   pop r0
   iret
.section s1

    .word 3
    push r2
    .skip 1
    
       .word -1
       int r5 

    xchg    r4  , r5
    add    r4  , r5
    sub    r4  , r5
    mul    r4  , r5
    div    r4  , r5
    cmp    r4  , r5
    and    r4  , r5
    or    r4  , r5
    xor    r4  , r5
    test    r4  , r5
    shl    r4  , r5
    shr    r4  , r5
    pop r2
    not r3
    cmp  r3, r2
    int r5
    mul psw, psw
    push r2
    test r3, r7
    pop r2
    halt
