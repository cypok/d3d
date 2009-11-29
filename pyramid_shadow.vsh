vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : pyramid rotation matrix
; c12-c15 : pyramid position matrix
; _________________________________
; OBJECT PROPERTIES
; c32 : 0 <= time <= 1 (t)
; c33 : 1 / specular degradation (f)
; c34 : sphere radius (R)
; _________________________________
; LIGHT SOURCES
; > POINT
;   c68 : position (P)
;   c71 : attenuation (a, b, c)
; _________________________________
; STUFF
; c48 : shadow matrix
; c52 : shadow color

dcl_position v0             ; vertex

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1

; _________________________________
; CALCULATING POSITION
; recieves: -
; returns: r8 (norm), r9 (vertex)

; we need to do this:
; 1. rotate
; 2. v0 * (1 + t*(R/|v0| - 1))
; 3. move

; v1 - initial normal (inorm)
; v0/|v0| - final normal (fnorm)

m4x4    r1, v0, c8          ; r1 = Rotate( v0 )
dp3     r0, r1, r1          ; r0 = Rotate(v0)^2
sge     r2, r0, r0          ; r2 = 1
rsq     r0, r0              ; r0.x = 1/|v0|
mad     r0, r0, c34, -r2    ; r0.x = R/|v0| - 1
mad     r0, r0, c32, r2     ; r0.x = 1 + t*(R/|v0| - 1)
mul     r1.xyz, r1.xyz, r0  ; r1 = v0 * (1 + t*(R/|v0| - 1))
m4x4    r8, r1, c12         ; r0 = Pyramid_moving( r9 )
m4x4    r9, r8, c48         ; making shadow ...

rcp     r1, r9.w            ; converting...
mul     r9, r9, r1          ; from (x,y,z,w) to (x/w, y/w, z/w, 1)

m4x4    oPos, r9, c4

; set color
        
        ; got them:
        ; r2 = distance from shadow vertex to light
        ; r3 = distance from shadow vertex to real vertex
        sub     r0, c68, r9         ; r2 = (LightSource - ShadowVertex)^2
        dp3     r2, r0, r0          ; 
        sub     r0, r8, r9          ; r3 = (Vertex - ShadowVertex)^2
        dp3     r3, r0, r0          ; 
        
        ; attenuation
        rsq     r1, r2              ; r1 = 1/d
        dst     r0, r2, r1          ; r0 = ( 1, d, d^2, 1/d )
        dp3     r0, r0, c71         ; r0 = a + b*d + c*d^2
        rcp     r0, r0              ; r0 = 1 / (a + b*d + c*d^2)
                                    ; r0 = attenuation factor
                                    
        ; cut "inversed" shadows
        sge     r1, r2, r3          ; r2 should be >= r3
        
mov     r10, c52
mul     r10.a, r10.a, r0
mul     r10.a, r10.a, r1
mov     oD0, r10