.extern PRINT_REG
.equ B_value, 5

.global print
.global A_location
.global B_text
.global B_value

.section text
  print:
    loop_print:
    ldr r3, [r2]
    str r3, PRINT_REG
    #str r3, %PRINT_REG # ovo bi bilo zgodno da moze jer ime smisla da neko rokne ovako

	ldr r1, $5
  	add r2, r1
  	ldr r1, $0
    cmp r2, r1
    jne loop_print
    ret

.section data
  A_location: .word 4
  B_text: .word 66, 0x20, 86, 65, 76, 85, 69, 0x3A, 0x20, 0
.end