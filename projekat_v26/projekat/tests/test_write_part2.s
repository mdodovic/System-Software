.extern PRINT_REG
.equ B_value, 5

.global print
.global A_location
.global B_text
.global B_value

.section text
  print:
    push r1
    push r3
    push r5
    loop_print:
    ldr r3, [r2]
    str r3, PRINT_REG
    str r3, %PRINT_REG # ovo bi bilo zgodno da moze jer ime smisla da neko rokne ovako
  
	ldr r1, $1
  ldr r5, $2
  mul r1, r5

  	add r2, r1
  	ldr r1, $0
    ldr r3, [r2]
    cmp r3, r1
    jne loop_print
    pop r5
    pop r3
    pop r1
    ret

.section data
  A_location: .word 4
  B_text: .word 66, 0x20, 86, 65, 76, 85, 69, 0x3A, 0x20, 0
.end