vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : rotation matrix
; c12-c15 : position matrix
; _________________________________
; OBJECT PROPERTIES
; c35-c38 : first bone (B1)
; c39-c42 : second bone (B2)
; _________________________________
; LIGHT SOURCES
; > SCENE
;   c64 : ambient color (Ia)
;
; > POINT
;   c68 : position (P)
;   c69 : diffuse color (Id)
;   c70 : specular color (Is)
;   c71 : attenuation (a, b, c)

dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color
dcl_texcoord v3             ; v3.x and v3.y are weights

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1

; _________________________________
; CALCULATING POSITION
; recieves: -
; returns: r8 (norm), r9 (vertex)

; we need to do this:
; 2. w1*[B1 x v0] + w2*[B2 x v0]
; 1. rotate
; 3. move

m4x4    r1, v0, c35         ; r1 = [B1 x v0]
m4x4    r2, v0, c39         ; r2 = [B2 x v0]
mul     r0, r1, v3.x        ; r0 = w1*[B1 x v0]
mad     r0, r2, v3.y, r0    ; r0 = w1*[B1 x v0] + w2*[B2 x v0]
m4x4    r3, r0, c8          ; r3 = Rotate( r0 )

m4x4    r1, v1, c35         ; r1 = [B1 x norm]
m4x4    r2, v1, c39         ; r2 = [B2 x norm]
mul     r0, r1, v3.x        ; r0 = w1*[B1 x norm]
mad     r0, r2, v3.y, r0    ; r0 = w1*[B1 x norm] + w2*[B2 x norm]
dp3     r1, r0, r0          ; normalizing...
rsq     r1, r1              ; ...
mul     r0, r0, r1          ; norm is normalized
m4x4    r8, r0, c8          ; r8 = Rotate( norm )

m4x4    r9, r3, c12         ; r9 = cylinder_moving( r9 )
m4x4    oPos, r9, c4

; _________________________________
; C O L O R   C O L O R   C O L O R
; recieves: r8 (norm), r9 (vertex)
; returns: r10 (color)

; calculating (eye - v)/|eye-v|
    sub     r7, c3, r9          ; r7 = eye - v
    dp3     r0, r7, r7          ; normalizing...
    rsq     r0, r0              ; ...
    mul     r7, r7, r0          ; r7 = (eye - v)/|eye-v|

; ambient
    mov     r10, c64        ; r10 =  Ia

; _________________________________
; CALCULATING POINT COLOR
; recieves: r7 (eye - v), r8 (norm), r9 (vertex), r10 (color)
; returns: r10 (color)

    sub     r5, c68, r9         ; r5 = L = LightSource - Vertex
    dp3     r0, r5, r5          ; normalizing...
    rsq     r0, r0              ; ...
    mul     r5, r5, r0          ; r5 = L/|L|

    ; diffuse
        dp3     r1, r8, r5          ; r1 = (norm, L) << DON'T EDIT R1
        max     r2, r1, c0          ; r2 = max(r1, 0)
        
        mul     r6, c69, r2         ; r6 = Id * ( (norm, L) )
                                    ; r6 = diffuse

    ; specular
        add     r1, r1, r1          ; r1 = 2*(norm, L)
        mad     r1, r1, r8, -r5     ; r1 = 2*(norm, L)*norm - L
        dp3     r1, r7, r1          ; r1 = ( eye-v, 2*(norm, L)*norm - L )

        mov     r1.w, c33           ; powering and checking that it's > 0
        lit     r1, r1              ; r1.z = r1^f
        
        mul     r2, c70, r1.z       ; r2 = Is * ( eye-v, 2*(norm, L)*norm - L )^f
                                    ; r2 = specular

    add     r6, r6, r2          ; color = diffuse + specular
    
    ; attenuation
        dp3     r0, r5, r5          ; r0 = d^2
        rsq     r1, r0              ; r1 = 1/d
        dst     r0, r0, r1          ; r0 = ( 1, d, d^2, 1/d )
        dp3     r0, r0, c71         ; r0 = a + b*d + c*d^2
        rcp     r0, r0              ; r0 = 1 / (a + b*d + c*d^2)
                                    ; r0 = attenuation factor
    mul     r6, r6, r0          ; color /= (a + b*d + c*d^2)
    
add     r10, r10, r6        ; ambient + directional + point

; set color
mul     oD0, r10, v2        ; color *= C