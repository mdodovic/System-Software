#.section text
#.skip 1
#label1:.skip 4
#.global label2	  
#label2:

#.word -1, 2, -3, 4, 5


#.word label1, label2, label3


#.section section2
#.skip 2
#label3:
#.end
.equ term_out, 0xFF00
.equ term_in, 0xFF03
.section text
jmp 13
jmp 0xF
# abs 
jeq lloc # symbol is local, so s1 and lloc_value is the combination
jeq lglob # symbol is global, so lglob and 0 is the combination
jeq lext # symbol is extern, so lext and 0 is the combination
jeq lloc_dif # symbol is local, so s2 and 4 is the combination
jeq term_in # written as it is
# pc rel
jeq %lloc  # symbol is local and in the same section! -> no relocation data
jeq %lglob # symbol is global -> -2 in mem, rel table filled
jeq %lext # symbol is extern -> -2 in mem, rel table filled
jeq %lloc_dif # symbol is local, but in another section, value - 2 is written, section is in rel table
jeq %term_in # ? 
jmp *10
jmp *0xA
call *lloc  # mem
call *lglob
call *lext
call *lloc_dif
call *term_in
jgt *r3  # reg dir
jeq *[r4] # reg ind
jne *[r3 + 7] # displ = 7
jne *[r4 + 0xB] # displ = B
jne *[r5 + lloc] # displ = local symbol: value goes into memory, reloc table is filled by section reloc
jne *[r6 + lglob]# displ = global symbol: 0 goes into memory, reloc table is filled by this symbol
jne *[r7 + lext]# displ = extern symbol: 0 goes into memory, reloc table is filled by this symbol
jne *[psw + lloc_dif] #displ = local symbol from different section : value goes into memory, reloc table is filled by section reloc
jne *[psw + term_in] # displ = equ sybol: value goes into memory as a constant
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

