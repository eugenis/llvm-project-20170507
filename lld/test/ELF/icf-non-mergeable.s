// REQUIRES: x86

// This file contains two functions. They are themselves identical,
// but because they have relocations against different data sections,
// they are not mergeable.

// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t1
// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux \
// RUN:    %p/Inputs/icf-non-mergeable.s -o %t2

// RUN: ld.lld %t1 %t2 -o %t3 --icf=all --verbose 2>&1 | FileCheck %s

// CHECK-NOT: selected section '.text.f1'
// CHECK-NOT:   removing identical section '.text.f2'

.globl _start, f1, f2, d1, d2
_start:
  ret

.section .text.f1, "ax"
f1:
  movl $5, d1
  ret

.section .text.f2, "ax"
f2:
  movl $5, d2
  ret
