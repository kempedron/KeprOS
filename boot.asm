bits 32

; Мультизагрузочный заголовок
section .multiboot
align 4
    dd 0x1BADB002          ; Магическое число
    dd 0x00000003          ; Флаги (выровнять + информация о памяти)
    dd -(0x1BADB002 + 0x03) ; Контрольная сумма

section .text
global start
extern os_main  ; точка входа C-кода

start:
    cli                     ; Отключить прерывания
    
    ; Установка стека
    mov esp, stack_top
    
    ; Сброс EFLAGS
    push 0
    popf
    
    ; Вызов основной C-функции
    call os_main
    
    ; Если os_main вернется (не должно быть)
    cli
.hang:
    hlt
    jmp .hang

; Стек
section .bootstrap_stack
stack_bottom:
    times 16384 db 0       ; 16KB стек
stack_top:
