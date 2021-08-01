.section section1 # ovo je prvi komentar
label1:  .skip 100 #drugi 
.global label2	  
label2:         

.word -1,   2
label3:
.word -3
label4:
.word 4, 5
.global label3,label4
#treci

  .word label1, label2, label3


.section section2
#.
label3  :   
.end
label4: .section sec4