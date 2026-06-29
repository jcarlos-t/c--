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
  movq $10, %rax
  movl %eax, -8(%rbp)
  movq $3, %rax
  movl %eax, -12(%rbp)
  movslq -8(%rbp), %rax
  pushq %rax
  movslq -12(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  setg %al
  movzbq %al, %rax
  cmpq $0, %rax
  je else_0
  movslq -8(%rbp), %rax
  pushq %rax
  movslq -12(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movl %eax, -16(%rbp)
  jmp endif_0
else_0:
  movslq -8(%rbp), %rax
  pushq %rax
  movslq -12(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  subq %rcx, %rax
  movl %eax, -16(%rbp)
endif_0:
  movslq -16(%rbp), %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movslq -8(%rbp), %rax
  pushq %rax
  movq $5, %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movl %eax, -8(%rbp)
  movslq -8(%rbp), %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movslq -12(%rbp), %rax
  pushq %rax
  movq $2, %rax
  movq %rax, %rcx
  popq %rax
  imulq %rcx, %rax
  movl %eax, -12(%rbp)
  movslq -12(%rbp), %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movslq -8(%rbp), %rax
  pushq %rax
  movq $3, %rax
  movq %rax, %rcx
  popq %rax
  cqto
  idivq %rcx
  movl %eax, -8(%rbp)
  movslq -8(%rbp), %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movslq -12(%rbp), %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rcx
  popq %rax
  subq %rcx, %rax
  movl %eax, -12(%rbp)
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
