; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s | FileCheck %s

; The input value is already sign extended, don't re-extend it.
; This testcase corresponds to:
;   int test(short X) { return (int)X; }

target datalayout = "e-p:64:64"
target triple = "x86_64-apple-darwin8"


define i32 @test(i16 signext  %X) {
; CHECK-LABEL: test:
; CHECK:       ## %bb.0: ## %entry
; CHECK-NEXT:    movl %edi, %eax
; CHECK-NEXT:    retq
entry:
        %tmp12 = sext i16 %X to i32             ; <i32> [#uses=1]
        ret i32 %tmp12
}

