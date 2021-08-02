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
	ldr r3, %A_location # dodata uduplana naredba pc relativnim da bi probali
	ldr r1, $48
	add r3, r1
	str r3, PRINT_REG  
	str r3, %PRINT_REG # pc relativni equ - ostavlja se za kasnije (kad se i emulator zavrsi)
	# gotov A
	# lupi enter:
	ldr r5, $46
	str r5, PRINT_REG 
	ldr r2, $B_text
    call %print   # stavljeno pc relativno radi probe
	ldr r3, $B_value # ispravljeno u B_value iz A_location
	ldr r1, $48
	add r3, r1
	str r3, PRINT_REG
	str r3, %PRINT_REG  # pc relativni equ - ostavlja se za kasnije (kad se i emulator zavrsi)
	# gotov B
    halt

.section data
  A_text: .word 65, 0x20, 86, 65, 76, 85, 69, 0x3A, 0x20, 0
.end