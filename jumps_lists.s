.section text
jmp 13
jeq lloc # abs
jeq lglob
jeq lext
jeq lloc_dif
jeq %lloc # pc rel
jeq %lglob
jeq %lext
jeq %lloc_dif
jmp *10
call *lloc  # mem
call *lglob
call *lext
call *lloc_dif
jgt *r3  # reg
jeq *[r4]
jne *[r3 + 7]
jne *[r3 + lloc]
jne *[r3 + lglob]
jne *[r3 + lext]
jne *[r3 + lloc_dif]
.skip 10
lloc:
    .word 0x11

lglob:
    .word 0x22

.global lglob
.extern lext

.section s2

lloc_dif:
    .word 0x55

