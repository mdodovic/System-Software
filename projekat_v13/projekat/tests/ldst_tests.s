.equ term_out, 0xFF00
.equ term_in, 0xFF03
.section text
ldr r3,$13
ldr r4,$0xF
# abs 
ldr r5,$lloc     # symbol is local, so s1 and lloc_value is the combination
ldr r6,$lglob    # symbol is global, so lglob and 0 is the combination
ldr r7,$lext     # symbol is extern, so lext and 0 is the combination
ldr psw,$lloc_dif # symbol is local, so s2 and 4 is the combination
ldr r0,$term_in  # written as it is
# memdir
str r1,13
str r2,0xF
# abs 
str r3, lloc     # symbol is local, so s1 and lloc_value is the combination
str r3, lglob    # symbol is global, so lglob and 0 is the combination
str r3, lext     # symbol is extern, so lext and 0 is the combination
str r3, lloc_dif # symbol is local, so s2 and 4 is the combination
str r3, term_in  # written as it is
# pcre
ldr r4, %lloc     # 
ldr r4, %lglob    # 
ldr r4, %lext     # 
ldr r4, %lloc_dif # 
ldr r4, %term_in  # 

# registers:
str r5, r3  # reg dir
str r5, [r4] # reg ind
str r5, [r3 + 7] # displ = 7
str r5, [r4 + 0xB] # displ = B
str r5, [r5 + lloc]      # displ = local symbol: value goes into memory, reloc table is filled by section reloc
str r5, [r6 + lglob]     # displ = global symbol: 0 goes into memory, reloc table is filled by this symbol
str r5, [r7 + lext]      # displ = extern symbol: 0 goes into memory, reloc table is filled by this symbol
str r5, [psw + lloc_dif] #displ = local symbol from different section : value goes into memory, reloc table is filled by section reloc
str r5, [psw + term_in]  # displ = equ sybol: value goes into memory as a constant

.skip 10
lloc:
    .word 0x11
.skip 0x20
lglob: .word 0x22
.skip 30

.global lglob
.extern lext

.section s2
.skip 4
lloc_dif:
    .word lloc

