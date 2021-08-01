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
  isr_timer:
	push r0
	push r3	
			ldr r0, myCounter
			ldr r3, $1
			add r0, r3
			str r0, %myCounter
	ldr r0, $asciiCode
	str r0, term_out
	pop r3
	pop r0
    iret
  isr_terminal:
  	push r0
  	ldr r0, term_in
  	str r0, term_out
  	pop r0
    iret
  main:
	ldr r1, $5
	tim_loop:
		ldr r0, $0x53 # S
		str r0, term_out	
		ldr r5, $0
		str r5, %myCounter
		str r1, tim_cfg
		wait:
                       # ldr r0, $77 # M
                       # str r0, term_out
			ldr r0, myCounter
			ldr r2, $2
			ldr r3, $1
			shl r3, r2
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
