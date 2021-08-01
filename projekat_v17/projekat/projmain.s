.section ivt
    .word isr_reset
    .skip 2  # isr_error
    .word isr_timer
    .word isr_terminal
    .skip 8
.equ asciiCode, 84 # ascii('T')
.equ term_out, 0xFF00
.equ term_in, 0xFF02
.equ tim_cfg, 0xFF10
.global myCounter
.section text
  isr_reset:
    jmp main
  isr_terminal:
	push r0	
	ldr r0, $1
	str r0, myCounter
	ldr r0, $asciiCode
	str r0, term_out
	pop r0
    iret
  isr_timer:
    iret
  main:
	ldr r1, $7
	tim_loop:
		str r1, tim_cfg
		wait:
			ldr r0, myCounter

			ldr r3, $5
			str r3, %myCounter
			cmp r0, r3
			jne wait

			xchg r3, r1

			ldr r2, $1
			sub r3, r2
			xchg r3, r1
			ldr r4, $0
			cmp r1, r4
	#halt
			jgt %tim_loop #;


    halt

.section data
  myCounter: .word 0
.end
