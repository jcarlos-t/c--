.data
print_fmt: .string "%ld\n"
println_fmt: .string "\n"

.text
.globl main

.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movq $0, %rax
  movl %eax, -8(%rbp)
  movq $0, %rax
  movl %eax, -12(%rbp)
dowhile_0:
  movslq -12(%rbp), %rax
  pushq %rax
  movslq -8(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movl %eax, -12(%rbp)
  movslq -8(%rbp), %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movl %eax, -8(%rbp)
docond_0:
  movslq -8(%rbp), %rax
  pushq %rax
  movq $5, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  setle %al
  movzbq %al, %rax
  cmpq $0, %rax
  jne dowhile_0
endwhile_0:
  movq $6, %rax
  movl %eax, -8(%rbp)
for_1:
  movslq -8(%rbp), %rax
  pushq %rax
  movq $10, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  setle %al
  movzbq %al, %rax
  cmpq $0, %rax
  je endfor_1
  movslq -8(%rbp), %rax
  pushq %rax
  movq $8, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  setg %al
  movzbq %al, %rax
  cmpq $0, %rax
  je else_2
  jmp endfor_1
  jmp endif_2
else_2:
endif_2:
  movslq -8(%rbp), %rax
  pushq %rax
  movq $7, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  sete %al
  movzbq %al, %rax
  cmpq $0, %rax
  je else_3
  jmp forinc_1
  jmp endif_3
else_3:
endif_3:
  movslq -12(%rbp), %rax
  pushq %rax
  movslq -8(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movl %eax, -12(%rbp)
forinc_1:
  movslq -8(%rbp), %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movl %eax, -8(%rbp)
  jmp for_1
endfor_1:
  movslq -12(%rbp), %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movq $0, %rax
  jmp .end_main
.end_main:
  leave
  ret

potencia:
  pushq %rbp
  movq %rsp, %rbp
  cmpq $0, %rsi
  jne .pot_nz
  movq $1, %rax
  jmp .pot_end
.pot_nz:
  cmpq $1, %rsi
  jne .pot_rec
  movq %rdi, %rax
  jmp .pot_end
.pot_rec:
  pushq %rbx
  movq %rdi, %rbx
  testq $1, %rsi
  jz .pot_even
  imulq %rdi, %rdi
  sarq $1, %rsi
  call potencia
  imulq %rbx, %rax
  popq %rbx
  jmp .pot_end
.pot_even:
  imulq %rdi, %rdi
  sarq $1, %rsi
  call potencia
  popq %rbx
.pot_end:
  leave
  ret

.section .note.GNU-stack,"",@progbits
