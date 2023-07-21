; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --function-signature --scrub-attributes
; RUN: opt -S -passes=attributor -aa-pipeline='basic-aa' -attributor-disable=false -attributor-max-iterations-verify -attributor-max-iterations=2 < %s | FileCheck %s

define internal i32 @testf(i1 %c) {
; CHECK-LABEL: define {{[^@]+}}@testf
; CHECK-SAME: (i1 [[C:%.*]])
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 [[C]], label [[IF_COND:%.*]], label [[IF_END:%.*]]
; CHECK:       if.cond:
; CHECK-NEXT:    unreachable
; CHECK:       if.then:
; CHECK-NEXT:    unreachable
; CHECK:       if.end:
; CHECK-NEXT:    ret i32 10
;
entry:
  br i1 %c, label %if.cond, label %if.end

if.cond:
  br i1 undef, label %if.then, label %if.end

if.then:                                          ; preds = %entry, %if.then
  ret i32 11

if.end:                                          ; preds = %if.then1, %entry
  ret i32 10
}

define internal i32 @test1(i1 %c) {
; CHECK-LABEL: define {{[^@]+}}@test1
; CHECK-SAME: (i1 [[C:%.*]])
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label [[IF_THEN:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[CALL:%.*]] = call i32 @testf(i1 [[C]])
; CHECK-NEXT:    br label [[RET1:%.*]]
; CHECK:       ret1:
; CHECK-NEXT:    ret i32 99
; CHECK:       ret2:
; CHECK-NEXT:    unreachable
;
entry:
  br label %if.then

if.then:                                          ; preds = %entry, %if.then
  %call = call i32 @testf(i1 %c)
  %res = icmp eq i32 %call, 10
  br i1 %res, label %ret1, label %ret2

ret1:                                           ; preds = %if.then, %entry
  ret i32 99

ret2:                                           ; preds = %if.then, %entry
  ret i32 0
}

define i32 @main(i1 %c) {
; CHECK-LABEL: define {{[^@]+}}@main
; CHECK-SAME: (i1 [[C:%.*]])
; CHECK-NEXT:    [[RES:%.*]] = call i32 @test1(i1 [[C]])
; CHECK-NEXT:    ret i32 99
;
  %res = call i32 @test1(i1 %c)
  ret i32 %res
}
