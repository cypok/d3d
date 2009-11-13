vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : rotation matrix
; c12-c15 : position matrix
; _________________________________
; OBJECT PROPERTIES
; c32 : 0 <= time <= 1 (t)
; c33 : 1 / specular degradation (f)
; _________________________________
; LIGHT SOURCES
; > SCENE
;   c64 : ambient color (Ia)
;
; > DIRECTIONAL
;   c65 : vector (-L)
;   c66 : diffuse color (Id)
;   c67 : specular color (Is)
; 
; > POINT
;   c68 : position (P)
;   c69 : diffuse color (Id)
;   c70 : specular color (Is)
;   c71 : attenuation (a, b, c)
; 
; > SPOT
;   c72 : position (P)
;   c73 : vector (Vspot)
;   c74 : diffuse color (Id)
;   c75 : specular color (Is)
;   c76 : attenuation (a, b, c)
;   c77 : ranges ( 1/(max-min) and min/(max-min) )
; _________________________________
; ANISOTROPIC COLORS
;   c96 : diffuse colors count
;   c97...c111 : diffuse colors
;   c112 : specular colors count
;   c113...c127 : specular colors

dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1

; _________________________________
; CALCULATING POSITION
; recieves: -
; returns: r8 (norm), r9 (vertex)

mov     r8, v1              ; r8 = normal
m4x4    r9, v0, c12         ; r9 = moving( r9 )
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
    mov     r10, c64            ; ambient
    

; _________________________________
; CALCULATING DIRECTIONAL COLOR
; recieves: r7 (eye - v), r8 (norm), r9 (vertex), r10 (color)
; returns: r10 (color)

    ; diffuse
        dp3     r1, r8, -c65        ; r1 = (norm, L) << DON'T EDIT R1
        max     r2, r1, c0          ; r2 = max(r1, 0)
        mul     r6, c66, r2         ; r6 = Id * (norm, L)
                                    ; r6 = diffuse

    ; specular
        add     r1, r1, r1          ; r1 = 2*(norm, L)
        mad     r1, r1, r8, c65     ; r1 = 2*(norm, L)*norm - L
        dp3     r1, r7, r1          ; r1 = ( eye-v, 2*(norm, L)*norm - L )

        mov     r1.w, c33           ; powering and checking that it's > 0
        lit     r1, r1              ; r1.z = r1^f

        mul     r2, c67, r1.z       ; r2 = Is * ( eye-v, 2*(norm, L)*norm - L )
        
        ; extra specular anisotropic
        ;dp3     r0, r7, r8          ; r0 = (eye-v, norm)
        ;mul     r0, r0, c96         ; r0 = float index of anisotropic color
        ;mov     a0.x, r0.x          ; a0 = index of anisotropic color
        ;frc     r1.y, r0            ; r1 = r0 - [r0] = Weight
        ;mul     r3, c[a0.x+98], r1.y; r3 = NextColor * w
        ;sub     r1.y, c1, r1.y      ; r1 = 1 - w
        ;mad     r3, c[a0.x+97], r1.y, r3 ; r3 = NextColor * w + PrevColor * (1-w)
        
        ;mul     r2, r2, r3

    add     r6, r6, r2          ; color = diffuse + specular

add     r10, r10, r6        ; ambient + directional

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
        
        mul     r0, r2, c96         ; r0 = float index of anisotropic color
        mov     a0.x, r0.x          ; a0 = index of anisotropic color
        frc     r2.y, r0            ; r2 = r0 - [r0] = Weight
        mul     r3, c[a0.x+98], r2.y; r3 = NextColor * w
        sub     r2.y, c1, r2.y      ; r2 = 1 - w
        mad     r3, c[a0.x+97], r2.y, r3 ; r3 = NextColor * w + PrevColor * (1-w)
        
        mul     r6, c69, r3         ; r6 = Id * Anisotropic( (norm, L) )
                                    ; r6 = diffuse

    ; specular
        add     r1, r1, r1          ; r1 = 2*(norm, L)
        mad     r1, r1, r8, -r5     ; r1 = 2*(norm, L)*norm - L
        dp3     r1, r7, r1          ; r1 = ( eye-v, 2*(norm, L)*norm - L )

        mov     r1.w, c33           ; powering and checking that it's > 0
        lit     r1, r1              ; r1.z = r1^f
        
        mul     r0, r1.z, c112      ; r0 = index of anisotropic color
        mov     a0.x, r0            ; a0 = index of anisotropic color
        frc     r2.y, r0            ; r2 = r0 - [r0] = Weight
        mul     r3, c[a0.x+114], r2.y; r3 = NextColor * w
        sub     r2.y, c1, r2.y      ; r2 = 1 - w
        mad     r3, c[a0.x+113], r2.y, r3 ; r3 = NextColor * w + PrevColor * (1-w)

        mul     r2, c70, r3         ; r2 = Is * Anisotropic( eye-v, 2*(norm, L)*norm - L )
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

; _________________________________
; CALCULATING SPOT COLOR
; recieves: r7 (eye - v), r8 (norm), r9 (vertex), r10 (color)
; returns: r10 (color)

    sub     r5, c72, r9         ; r5 = L = LightSource - Vertex
    dp3     r0, r5, r5          ; normalizing...
    rsq     r0, r0              ; ...
    mul     r5, r5, r0          ; r5 = L/|L|
    
    ; diffuse
        dp3     r1, r8, r5          ; r1 = (norm, L) << DON'T EDIT R1
        max     r2, r1, c0          ; r2 = max(r1, 0)
        mul     r6, c74, r2         ; r6 = Id * (norm, L)
                                    ; r6 = diffuse

    ; specular
        add     r1, r1, r1          ; r1 = 2*(norm, L)
        mad     r1, r1, r8, -r5     ; r1 = 2*(norm, L)*norm - L
        dp3     r1, r7, r1          ; r1 = ( eye-v, 2*(norm, L)*norm - L )

        mov     r1.w, c33           ; powering and checking that it's > 0
        lit     r1, r1              ; r1.z = r1^f

        mul     r2, c75, r1.z       ; r2 = Is * ( eye-v, 2*(norm, L)*norm - L )
                                    ; r2 = specular

    add     r6, r6, r2          ; color = diffuse + specular
    
    ; attenuation
        dp3     r0, r5, r5          ; r0 = d^2
        rsq     r1, r0              ; r1 = 1/d
        dst     r0, r0, r1          ; r0 = ( 1, d, d^2, 1/d )
        dp3     r0, r0, c76         ; r0 = a + b*d + c*d^2
        rcp     r0, r0              ; r0 = 1 / (a + b*d + c*d^2)
                                    ; r0 = attenuation factor
    mul     r6, r6, r0          ; color /= (a + b*d + c*d^2)
    
    ; spotting: we would calc this: (r0-min)/(max-min)
        dp3     r1, r5, -c73          ; r1 = (Vspot, L)
        mad     r0, r1, c77.x, -c77.y ; r0 = r1/(max-min) - min/(max-min)
        max     r0, r0, c0            ; => r0 >= 0
        min     r0, r0, c1            ; => r0 <= 1
        
    mul     r6, r6, r0          ; color *= spotting_factor
    
add     r10, r10, r6        ; ambient + directional + point + spot

;dp3     r0, r7, r8          ; r0 = (eye-v, norm)
;sge     r1, r0, c0          ; r1 = (r0 >= 0) ? 1 : 0
;mul     r10, r10, r1        ; r10 = 0 if back side of vertex is visible

; set color
mul     oD0, r10, v2        ; color *= C