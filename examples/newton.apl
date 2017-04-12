
⍝ See Chaitin 1986, Physics in APL2, p. 14
⍝ Newton -- Orbits
⍝ Transcribed by Daniel Llorens - 2013
⍝ With GNU APL do apl -f newton.apl

∇NEWTON
 ⎕IO←0
 BODIES←2
 ORBIT←50 50⍴' '
 G←.6674E¯10
 T←0
 DELT←50
 M←6E24 10
 V←X←BODIES 3⍴0
 X[1;0]←1E7
 V[1;1]←6E3
 STEP←1
LOOP:
 F←(⍳BODIES)∘.FORCE⍳BODIES
 A←⊃(+/F)÷M
 V←V+A×DELT
 X←X+V×DELT
 T←T+DELT
 DRAW
 →((12×15)≥STEP←STEP+1)/LOOP
∇

∇F←I FORCE J
 F←3⍴0
 →(I=J)/0
 DELX←X[J;]-X[I;]
 R←(+/DELX⋆2)⋆.5
 F←(G×M[I]×M[J]÷R⋆2)×(DELX÷R)
∇

∇DRAW
 ORBIT[25+⌊X[0;0]÷5E5;25+⌊X[0;1]÷5E5]←'E'
 ORBIT[25+⌊X[1;0]÷5E5;25+⌊X[1;1]÷5E5]←'⋆'
 →(0≠15|STEP)/0
 ' '
 'TIME IN HOURS = ',⍕T÷60×60
 FRAME ORBIT
∇

∇FRAME PIC
 '|',('-',[0]PIC,[0]'-'),'|'
∇

NEWTON
