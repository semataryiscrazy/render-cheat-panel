; ─── Reflective Loader — x64 assembly ──────────────────────────
; Versão simplificada: só aplica relocações + chama entry point.
; Imports são resolvidos do lado do host (ghost.exe) e escritos
; na IAT antes do loader executar.
;
; Compilar: ml64 /c /Fo reflective_loader.obj reflective_loader.asm
;
; Registradores durante execução:
;   r15 = LOADER_DATA
;   r12 = pImageBase

.CODE

; ─── Entry point ──────────────────────────────────────────────
; RCX = ponteiro para LOADER_DATA
ReflectiveLoader PROC
    push    rbx
    push    rbp
    push    rsi
    push    rdi
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 28h

    mov     r15, rcx                    ; r15 = LOADER_DATA
    mov     r12, [r15 + 00h]            ; r12 = pImageBase
    mov     r13, [r15 + 20h]            ; r13 = qwDelta

    ; ─── Aplicar relocações ─────────────────────────────────
    mov     r14d, [r15 + 18h]           ; r14d = pRelocDirRVA
    test    r14d, r14d
    jz      call_entry

    add     r14, r12                    ; r14 = IMAGE_BASE_RELOCATION
    mov     r10d, [r15 + 1Ch]           ; r10d = dwRelocSize
    test    r10d, r10d
    jz      call_entry

    mov     r9d, r10d                   ; r9d = size remaining

reloc_loop:
    cmp     r9d, 8
    jb      call_entry                  ; dados insuficientes

    mov     ebx, [r14]                  ; VirtualAddress (page RVA)
    mov     ebp, [r14 + 4]              ; SizeOfBlock
    test    ebx, ebx
    jz      call_entry
    test    ebp, ebp
    jz      call_entry

    sub     r9d, ebp
    sub     ebp, 8                      ; subtrair header
    jz      next_block

    ; processar entries
    lea     rdi, [r14 + 8]              ; primeiro TypeOffset
    xor     ecx, ecx                    ; index

reloc_entry:
    cmp     ecx, ebp
    jae     next_block

    movzx   eax, word ptr [rdi + rcx]   ; TypeOffset
    mov     edx, eax
    and     edx, 0FFFh                  ; offset in page
    and     eax, 0F000h
    shr     eax, 12                     ; type

    cmp     eax, 0Ah                    ; IMAGE_REL_BASED_DIR64
    jne     skip_entry

    ; DIR64: [page_base + offset] += delta
    lea     rsi, [r12 + rbx]
    add     rsi, rdx
    mov     rax, [rsi]
    add     rax, r13
    mov     [rsi], rax

skip_entry:
    add     ecx, 2
    jmp     reloc_entry

next_block:
    add     r14, [r14 + 4]              ; avançar para próximo bloco
    test    r9d, r9d
    jnz     reloc_loop

    ; ─── Chamar entry point ─────────────────────────────────
call_entry:
    ; sem entry point por enquanto — só relocações
success:
    xor     eax, eax
    inc     eax                         ; return 1

    add     rsp, 28h
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbp
    pop     rbx
    ret
ReflectiveLoader ENDP

; ─── End marker ──────────────────────────────────────────────
ReflectiveLoaderEnd PROC
    ret
ReflectiveLoaderEnd ENDP

END
