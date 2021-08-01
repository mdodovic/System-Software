# file interrupts.s
.section ivt
    .word isr_reset
    int r0
    .skip 2  # isr_error
    .word isr_timer
    int psw
    .word isr_terminal
    not r7
    .skip 8

.extern myStart, myComputer
.section isr
.equ term_out, 0xFF00
.equ term_in, 0xFF02
.equ asciiCode, 84 # ascii('T')
# prekidna rutina za reset
isr_reset:
    .word 4 #jmp myStart
# prekidna rutina za tajmer
isr_timer:
    push r0
    .word 1 #ldr r0, $asciiCode
    .word 2 #str r0, term_out
    pop r0
    iret
#prekidna rutina za terminal
isr_terminal:
    push r0
    push r1
    .word 10 #ldr %r0, term_in
    .word 11 #str %r0, term_out
    .word 12 #ldr %r0, %myCounter # pc rel
    .word 13 #ldr %r1, $1
    .word 14 #add %r0, r1
    .word 15 #str %r0, myCounter # abs
    pop r1
    pop r0
    iret
