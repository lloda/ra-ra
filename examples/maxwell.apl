⍝ -*- mode: apl; coding: utf-8 -*-
⍝ See Chaitin 1986, Physics in APL2, p. 20
⍝ Maxwell -- 4-vector potential vacuum field equations
⍝ Transcribed by Daniel Llorens - 2013
⍝ With GNU APL do apl -f maxwell.apl

∇MAXWELL
 ⎕IO←0
 DELTA←1
 O←N←20
 M←L←1
 A←O N M L 4⍴0
 A[0;;;;2]←-(÷○2÷N)×2○○2×(⍳N)÷N
 A[1;;;;2]←-(÷○2÷N)×2○○2×((-DELTA)+⍳N)÷N
 T←1
LOOP:
 X←(1⌽[0]A[T;;;;])+(1⌽[1]A[T;;;;])+(1⌽[2]A[T;;;;])
 Y←(¯1⌽[0]A[T;;;;])+(¯1⌽[1]A[T;;;;])+(¯1⌽[2]A[T;;;;])
 A[T+1;;;;]←X+Y-A[T-1;;;;]+4×A[T;;;;]
 →(O>1+T←T+1)/LOOP
 DA←O N M L 4 4⍴0
 DA[;;;;;0]←((1⌽[0]A)-(¯1⌽[0]A))÷2×DELTA
 DA[;;;;;1]←-((1⌽[1]A)-(¯1⌽[1]A))÷2×DELTA
 DA[;;;;;2]←-((1⌽[2]A)-(¯1⌽[2]A))÷2×DELTA
 DA[;;;;;3]←-((1⌽[3]A)-(¯1⌽[3]A))÷2×DELTA
 'LORENTZ CONDITION: MAX|DIV| = 0?'
 ⌈/,|+/0 1 2 3 4 4⍉DA
 F←(0 1 2 3 5 4⍉DA)-DA
 T←0
LOOP2:
 DRAW
 →(O>T←T+1)/LOOP2
∇

∇DRAW
 'Ex' SHOW F[T;;0;0;1;0]
 'Ey' SHOW F[T;;0;0;2;0]
 'Ez' SHOW F[T;;0;0;3;0]
 'Bx' SHOW F[T;;0;0;3;2]
 'By' SHOW F[T;;0;0;1;3]
 'Bz' SHOW F[T;;0;0;2;1]
∇

∇NAME SHOW F
 →(^/0=F)/0
 ' '
 NAME,' AT TIME = ',⍕T×DELTA
 FRAME(-⌊26+25×F)⌽((⍴F),52)⍴'⋆',51⍴' '
∇

∇FRAME PIC
 '-',[0]('|',' ',' ',(⍕N 1⍴⍳N),' ',' ','|',PIC,'|'),[0]'-'
∇

MAXWELL
