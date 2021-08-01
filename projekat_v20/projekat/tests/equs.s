.section section1
label1:
    .skip 10
label2:
    .word label2
#.global eLabel1
    .equ eLabel1, 15

   .word eLabel1

.section myData
myCounter: 
    .word 0

.end
