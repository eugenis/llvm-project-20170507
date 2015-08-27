; RUN: opt %loadPolly -polly-analyze-read-only-scalars=false -polly-codegen \
; RUN:     -polly-no-early-exit -polly-detect-unprofitable \
; RUN:     -S < %s | FileCheck %s
; RUN: opt %loadPolly -polly-analyze-read-only-scalars=true -polly-codegen \
; RUN:     -polly-no-early-exit -polly-detect-unprofitable \
; RUN:     -S < %s | FileCheck %s -check-prefix=SCALAR

; CHECK-NOT: alloca

; SCALAR-LABEL: entry:
; SCALAR-NEXT: %scalar.s2a = alloca float

; SCALAR:  %scalar.s2a.reload = load float, float* %scalar.s2a
; SCALAR-NEXT:  %p_sum = fadd float %val_p_scalar_, %scalar.s2a.reload

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
