#file main.s
.global myStart
.global myCounter
.section myCode
.equ tim_cfg, 0xFF10
myStart:
    .skip 5
    .skip 5
    iret
wait:
    .word -1
    .word -2
    .word -3
#    halt
.section myData
myCounter:
    .word 0
    ret
.end
    