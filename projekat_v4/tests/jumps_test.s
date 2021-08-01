.section text
int r3
l1:		jmp *10

	jeq zika
	
pera: call *pera

	.equ zika,  9
	.equ ana, -2

.extern mika
.global zika
jgt *r3
jeq *[r4]

call *[r7 -   5]
zika: 
jmp l1
jmp *[r3 + zika]
jmp 13
jmp %l1
jmp %zika

call pera

.end