.section section1 # ovo je prvi komentar
label1:  .skip 100 #drugi 
.global label2	  
label2:         

.word -1,   2, -3, 4, 5
#.global labEX3
#treci

  .word label1, label2, label3


.section section2
#.
label3  :   
#.extern labEX1, labEX2, labEX3
# .skip 4
#labEX3:.word labEX2
.end
label4: .section sec4
