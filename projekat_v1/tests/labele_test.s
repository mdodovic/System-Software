.section section1
label1:.skip 100
.global label2	  
label2:

.byte -1, 2, -3, 4, 5


.word label1, label2, label3


.section section2
.
label3:
.end