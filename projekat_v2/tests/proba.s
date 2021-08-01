.section section1 # ovo je prvi komentar
label1:  .skip 0x10 #drugi 
.global label2	  
label2:         

.word -1,   2, -3, 4, 5
#.global labEX3
#treci

label4:  .word label1
.word label2, label3  , label4 #, label5

#.global label4
#.extern label4
.section section2
#.
.word 1
label3  :   
.word 16
.extern labEX1, labEX2, labEX3
# .skip 4
#labEX3:.word labEX2
.end
label4: .section sec4
