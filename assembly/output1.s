.data
print_fmt: .string "%ld\n"
println_fmt: .string "\n"

.text
.globl main

.globl triple
triple:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movl %eax, -8(%rbp)
  movq $3, %rax
  pushq %rax
  movslq -8(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  imulq %rcx, %rax
  jmp .end_triple
.end_triple:
  leave
  ret

.globl factorial
factorial:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movl %eax, -8(%rbp)
  movslq -8(%rbp), %rax
  pushq %rax
  movq $0, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  sete %al
  movzbq %al, %rax
  cmpq $0, %rax
  je else_0
  movq $1, %rax
  jmp .end_factorial
  jmp endif_0
else_0:
endif_0:
  movslq -8(%rbp), %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rcx
  popq %rax
  subq %rcx, %rax
  movq %rax, %rdi
  call factorial
  pushq %rax
  movslq -8(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  imulq %rcx, %rax
  jmp .end_factorial
.end_factorial:
  leave
  ret

.globl suma
suma:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movl %eax, -8(%rbp)
  movl %eax, -12(%rbp)
  movslq -8(%rbp), %rax
  pushq %rax
  movslq -12(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  jmp .end_suma
.end_suma:
  leave
  ret

.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movq $2, %rax
  pushq %rax
  movq $2, %rax
  movq %rax, %rcx
  popq %rax
  imulq %rcx, %rax
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
  movslq -8(%rbp), %rax
  movq %rax, %rdi
  call factorial
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movslq -8(%rbp), %rax
  movq %rax, %rdi
  call triple
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  leaq println_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  movslq -8(%rbp), %rax
  movq %rax, %rdi
  movslq -8(%rbp), %rax
  movq %rax, %rsi
  call suma
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
