; RUN: opt %loadPolly -polly-analyze-read-only-scalars=false -polly-scops \
; RUN:                -polly-detect-unprofitable -analyze < %s | FileCheck %s
; RUN: opt %loadPolly -polly-analyze-read-only-scalars=true -polly-scops \
; RUN:                -polly-detect-unprofitable -analyze < %s | FileCheck %s \
; RUN:                -check-prefix=SCALARS

; CHECK-NOT: Memref_scalar

; SCALARS: float MemRef_scalar[*] // Element size 4

; SCALARS: ReadAccess :=  [Reduction Type: NONE] [Scalar: 1]
; SCALARS:     { Stmt_stmt1[i0] -> MemRef_scalar[] };

define void @foo(float* noalias %A, float %scalar) {
entry:
  br label %loop

loop:
  %indvar = phi i64 [0, %entry], [%indvar.next, %loop.backedge]
  br label %stmt1

stmt1:
  %val = load float, float* %A
  %sum = fadd float %val, %scalar
  store float %sum, float* %A
  br label %loop.backedge

loop.backedge:
  %indvar.next = add i64 %indvar, 1
  %cond = icmp sle i64 %indvar, 100
  br i1 %cond, label %loop, label %exit

exit:
  ret void
}
