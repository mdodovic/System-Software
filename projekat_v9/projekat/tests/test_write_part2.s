.section ivtp
  .word main
  .skip 2
  .word timer
  .word keyboard
  .skip 8

.extern print
.extern A_location
.extern B_value
.extern B_text

.global PRINT_REG

.equ PRINT_REG, 0xFF00

.section text
  keyboard:
    iret
  timer:
    iret
  main:
	ldr r2, $A_text
    call print
	ldr r3, A_location
	ldr r1, $48
	add r3, r1
	str r3, PRINT_REG
	str r3, PRINT_REG
	# gotov A
	ldr r2, $B_text
    call print
	ldr r3, A_location
	ldr r1, $48
	add r3, r1
	str r3, PRINT_REG
	str r3, PRINT_REG
	# gotov B
    halt

.section data
  A_text: .word 65, 0x20, 86, 65, 76, 85, 69, 0x3A, 0x20, 0
.end