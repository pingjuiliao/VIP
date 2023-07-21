; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -basicaa -loop-vectorize -force-vector-interleave=1 -force-vector-width=4 -loop-vectorize-with-block-frequency -dce -instcombine -S | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.8.0"

@b = common global [2048 x i32] zeroinitializer, align 16
@c = common global [2048 x i32] zeroinitializer, align 16
@a = common global [2048 x i32] zeroinitializer, align 16
@G = common global [32 x [1024 x i32]] zeroinitializer, align 16
@ub = common global [1024 x i32] zeroinitializer, align 16
@uc = common global [1024 x i32] zeroinitializer, align 16
@d = common global [2048 x i32] zeroinitializer, align 16
@fa = common global [1024 x float] zeroinitializer, align 16
@fb = common global [1024 x float] zeroinitializer, align 16
@ic = common global [1024 x i32] zeroinitializer, align 16
@da = common global [1024 x float] zeroinitializer, align 16
@db = common global [1024 x float] zeroinitializer, align 16
@dc = common global [1024 x float] zeroinitializer, align 16
@dd = common global [1024 x float] zeroinitializer, align 16
@dj = common global [1024 x i32] zeroinitializer, align 16

; We can optimize this test without a tail.
define void @example1() optsize {
; CHECK-LABEL: @example1(
; CHECK-NEXT:    br i1 false, label [[SCALAR_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[VECTOR_BODY]] ]
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 [[INDEX]]
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i32* [[TMP1]] to <4 x i32>*
; CHECK-NEXT:    [[WIDE_LOAD:%.*]] = load <4 x i32>, <4 x i32>* [[TMP2]], align 16
; CHECK-NEXT:    [[TMP3:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @c, i64 0, i64 [[INDEX]]
; CHECK-NEXT:    [[TMP4:%.*]] = bitcast i32* [[TMP3]] to <4 x i32>*
; CHECK-NEXT:    [[WIDE_LOAD1:%.*]] = load <4 x i32>, <4 x i32>* [[TMP4]], align 16
; CHECK-NEXT:    [[TMP5:%.*]] = add nsw <4 x i32> [[WIDE_LOAD1]], [[WIDE_LOAD]]
; CHECK-NEXT:    [[TMP6:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @a, i64 0, i64 [[INDEX]]
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast i32* [[TMP6]] to <4 x i32>*
; CHECK-NEXT:    store <4 x i32> [[TMP5]], <4 x i32>* [[TMP7]], align 16
; CHECK-NEXT:    [[INDEX_NEXT]] = add i64 [[INDEX]], 4
; CHECK-NEXT:    [[TMP8:%.*]] = icmp eq i64 [[INDEX_NEXT]], 256
; CHECK-NEXT:    br i1 [[TMP8]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop !0
; CHECK:       middle.block:
; CHECK-NEXT:    br i1 true, label [[TMP10:%.*]], label [[SCALAR_PH]]
; CHECK:       scalar.ph:
; CHECK-NEXT:    br label [[TMP9:%.*]]
; CHECK:         br i1 undef, label [[TMP10]], label [[TMP9]], !llvm.loop !2
; CHECK:         ret void
;
  br label %1

; <label>:1                                       ; preds = %1, %0
  %indvars.iv = phi i64 [ 0, %0 ], [ %indvars.iv.next, %1 ]
  %2 = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 %indvars.iv
  %3 = load i32, i32* %2, align 4
  %4 = getelementptr inbounds [2048 x i32], [2048 x i32]* @c, i64 0, i64 %indvars.iv
  %5 = load i32, i32* %4, align 4
  %6 = add nsw i32 %5, %3
  %7 = getelementptr inbounds [2048 x i32], [2048 x i32]* @a, i64 0, i64 %indvars.iv
  store i32 %6, i32* %7, align 4
  %indvars.iv.next = add i64 %indvars.iv, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, 256
  br i1 %exitcond, label %8, label %1

; <label>:8                                       ; preds = %1
  ret void
}

; Can vectorize in 'optsize' mode by masking the needed tail.
define void @example2(i32 %n, i32 %x) optsize {
; CHECK-LABEL: @example2(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sgt i32 [[N:%.*]], 0
; CHECK-NEXT:    br i1 [[TMP1]], label [[DOTLR_PH5_PREHEADER:%.*]], label [[DOTPREHEADER:%.*]]
; CHECK:       .lr.ph5.preheader:
; CHECK-NEXT:    [[TMP2:%.*]] = add i32 [[N]], -1
; CHECK-NEXT:    [[TMP3:%.*]] = zext i32 [[TMP2]] to i64
; CHECK-NEXT:    br i1 false, label [[SCALAR_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    [[N_RND_UP:%.*]] = add nuw nsw i64 [[TMP3]], 4
; CHECK-NEXT:    [[N_VEC:%.*]] = and i64 [[N_RND_UP]], 8589934588
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT1:%.*]] = insertelement <4 x i64> undef, i64 [[TMP3]], i32 0
; CHECK-NEXT:    [[BROADCAST_SPLAT2:%.*]] = shufflevector <4 x i64> [[BROADCAST_SPLATINSERT1]], <4 x i64> undef, <4 x i32> zeroinitializer
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[PRED_STORE_CONTINUE8:%.*]] ]
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT:%.*]] = insertelement <4 x i64> undef, i64 [[INDEX]], i32 0
; CHECK-NEXT:    [[BROADCAST_SPLAT:%.*]] = shufflevector <4 x i64> [[BROADCAST_SPLATINSERT]], <4 x i64> undef, <4 x i32> zeroinitializer
; CHECK-NEXT:    [[INDUCTION:%.*]] = add <4 x i64> [[BROADCAST_SPLAT]], <i64 0, i64 1, i64 2, i64 3>
; CHECK-NEXT:    [[TMP5:%.*]] = or i64 [[INDEX]], 1
; CHECK-NEXT:    [[TMP6:%.*]] = or i64 [[INDEX]], 2
; CHECK-NEXT:    [[TMP7:%.*]] = or i64 [[INDEX]], 3
; CHECK-NEXT:    [[TMP8:%.*]] = icmp ule <4 x i64> [[INDUCTION]], [[BROADCAST_SPLAT2]]
; CHECK-NEXT:    [[TMP9:%.*]] = extractelement <4 x i1> [[TMP8]], i32 0
; CHECK-NEXT:    br i1 [[TMP9]], label [[PRED_STORE_IF:%.*]], label [[PRED_STORE_CONTINUE:%.*]]
; CHECK:       pred.store.if:
; CHECK-NEXT:    [[TMP10:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 [[INDEX]]
; CHECK-NEXT:    store i32 [[X:%.*]], i32* [[TMP10]], align 16
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE]]
; CHECK:       pred.store.continue:
; CHECK-NEXT:    [[TMP11:%.*]] = extractelement <4 x i1> [[TMP8]], i32 1
; CHECK-NEXT:    br i1 [[TMP11]], label [[PRED_STORE_IF3:%.*]], label [[PRED_STORE_CONTINUE4:%.*]]
; CHECK:       pred.store.if3:
; CHECK-NEXT:    [[TMP12:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 [[TMP5]]
; CHECK-NEXT:    store i32 [[X]], i32* [[TMP12]], align 4
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE4]]
; CHECK:       pred.store.continue4:
; CHECK-NEXT:    [[TMP13:%.*]] = extractelement <4 x i1> [[TMP8]], i32 2
; CHECK-NEXT:    br i1 [[TMP13]], label [[PRED_STORE_IF5:%.*]], label [[PRED_STORE_CONTINUE6:%.*]]
; CHECK:       pred.store.if5:
; CHECK-NEXT:    [[TMP14:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 [[TMP6]]
; CHECK-NEXT:    store i32 [[X]], i32* [[TMP14]], align 8
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE6]]
; CHECK:       pred.store.continue6:
; CHECK-NEXT:    [[TMP15:%.*]] = extractelement <4 x i1> [[TMP8]], i32 3
; CHECK-NEXT:    br i1 [[TMP15]], label [[PRED_STORE_IF7:%.*]], label [[PRED_STORE_CONTINUE8]]
; CHECK:       pred.store.if7:
; CHECK-NEXT:    [[TMP16:%.*]] = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 [[TMP7]]
; CHECK-NEXT:    store i32 [[X]], i32* [[TMP16]], align 4
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE8]]
; CHECK:       pred.store.continue8:
; CHECK-NEXT:    [[INDEX_NEXT]] = add i64 [[INDEX]], 4
; CHECK-NEXT:    [[TMP17:%.*]] = icmp eq i64 [[INDEX_NEXT]], [[N_VEC]]
; CHECK-NEXT:    br i1 [[TMP17]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop !4
; CHECK:       middle.block:
; CHECK-NEXT:    br i1 true, label [[DOT_PREHEADER_CRIT_EDGE:%.*]], label [[SCALAR_PH]]
; CHECK:       ._crit_edge:
; CHECK-NEXT:    ret void
;
  %1 = icmp sgt i32 %n, 0
  br i1 %1, label %.lr.ph5, label %.preheader

..preheader_crit_edge:                            ; preds = %.lr.ph5
  %phitmp = sext i32 %n to i64
  br label %.preheader

.preheader:                                       ; preds = %..preheader_crit_edge, %0
  %i.0.lcssa = phi i64 [ %phitmp, %..preheader_crit_edge ], [ 0, %0 ]
  %2 = icmp eq i32 %n, 0
  br i1 %2, label %._crit_edge, label %.lr.ph

.lr.ph5:                                          ; preds = %0, %.lr.ph5
  %indvars.iv6 = phi i64 [ %indvars.iv.next7, %.lr.ph5 ], [ 0, %0 ]
  %3 = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 %indvars.iv6
  store i32 %x, i32* %3, align 4
  %indvars.iv.next7 = add i64 %indvars.iv6, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next7 to i32
  %exitcond = icmp eq i32 %lftr.wideiv, %n
  br i1 %exitcond, label %..preheader_crit_edge, label %.lr.ph5

.lr.ph:                                           ; preds = %.preheader, %.lr.ph
  %indvars.iv = phi i64 [ %indvars.iv.next, %.lr.ph ], [ %i.0.lcssa, %.preheader ]
  %.02 = phi i32 [ %4, %.lr.ph ], [ %n, %.preheader ]
  %4 = add nsw i32 %.02, -1
  %5 = getelementptr inbounds [2048 x i32], [2048 x i32]* @b, i64 0, i64 %indvars.iv
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds [2048 x i32], [2048 x i32]* @c, i64 0, i64 %indvars.iv
  %8 = load i32, i32* %7, align 4
  %9 = and i32 %8, %6
  %10 = getelementptr inbounds [2048 x i32], [2048 x i32]* @a, i64 0, i64 %indvars.iv
  store i32 %9, i32* %10, align 4
  %indvars.iv.next = add i64 %indvars.iv, 1
  %11 = icmp eq i32 %4, 0
  br i1 %11, label %._crit_edge, label %.lr.ph

._crit_edge:                                      ; preds = %.lr.ph, %.preheader
  ret void
}

; N is unknown, we need a tail. Can't vectorize because loop has no primary
; induction.
;CHECK-LABEL: @example3(
;CHECK-NOT: <4 x i32>
;CHECK: ret void
define void @example3(i32 %n, i32* noalias nocapture %p, i32* noalias nocapture %q) optsize {
  %1 = icmp eq i32 %n, 0
  br i1 %1, label %._crit_edge, label %.lr.ph

.lr.ph:                                           ; preds = %0, %.lr.ph
  %.05 = phi i32 [ %2, %.lr.ph ], [ %n, %0 ]
  %.014 = phi i32* [ %5, %.lr.ph ], [ %p, %0 ]
  %.023 = phi i32* [ %3, %.lr.ph ], [ %q, %0 ]
  %2 = add nsw i32 %.05, -1
  %3 = getelementptr inbounds i32, i32* %.023, i64 1
  %4 = load i32, i32* %.023, align 16
  %5 = getelementptr inbounds i32, i32* %.014, i64 1
  store i32 %4, i32* %.014, align 16
  %6 = icmp eq i32 %2, 0
  br i1 %6, label %._crit_edge, label %.lr.ph

._crit_edge:                                      ; preds = %.lr.ph, %0
  ret void
}

; We can't vectorize this one because we need a runtime ptr check.
;CHECK-LABEL: @example23(
;CHECK-NOT: <4 x i32>
;CHECK: ret void
define void @example23(i16* nocapture %src, i32* nocapture %dst) optsize {
  br label %1

; <label>:1                                       ; preds = %1, %0
  %.04 = phi i16* [ %src, %0 ], [ %2, %1 ]
  %.013 = phi i32* [ %dst, %0 ], [ %6, %1 ]
  %i.02 = phi i32 [ 0, %0 ], [ %7, %1 ]
  %2 = getelementptr inbounds i16, i16* %.04, i64 1
  %3 = load i16, i16* %.04, align 2
  %4 = zext i16 %3 to i32
  %5 = shl nuw nsw i32 %4, 7
  %6 = getelementptr inbounds i32, i32* %.013, i64 1
  store i32 %5, i32* %.013, align 4
  %7 = add nsw i32 %i.02, 1
  %exitcond = icmp eq i32 %7, 256
  br i1 %exitcond, label %8, label %1

; <label>:8                                       ; preds = %1
  ret void
}


; We CAN vectorize this example because the pointers are marked as noalias.
define void @example23b(i16* noalias nocapture %src, i32* noalias nocapture %dst) optsize {
; CHECK-LABEL: @example23b(
; CHECK-NEXT:    br i1 false, label [[SCALAR_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[VECTOR_BODY]] ]
; CHECK-NEXT:    [[NEXT_GEP:%.*]] = getelementptr i16, i16* [[SRC:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    [[NEXT_GEP4:%.*]] = getelementptr i32, i32* [[DST:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i16* [[NEXT_GEP]] to <4 x i16>*
; CHECK-NEXT:    [[WIDE_LOAD:%.*]] = load <4 x i16>, <4 x i16>* [[TMP1]], align 2
; CHECK-NEXT:    [[TMP2:%.*]] = zext <4 x i16> [[WIDE_LOAD]] to <4 x i32>
; CHECK-NEXT:    [[TMP3:%.*]] = shl nuw nsw <4 x i32> [[TMP2]], <i32 7, i32 7, i32 7, i32 7>
; CHECK-NEXT:    [[TMP4:%.*]] = bitcast i32* [[NEXT_GEP4]] to <4 x i32>*
; CHECK-NEXT:    store <4 x i32> [[TMP3]], <4 x i32>* [[TMP4]], align 4
; CHECK-NEXT:    [[INDEX_NEXT]] = add i64 [[INDEX]], 4
; CHECK-NEXT:    [[TMP5:%.*]] = icmp eq i64 [[INDEX_NEXT]], 256
; CHECK-NEXT:    br i1 [[TMP5]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop !6
; CHECK:       middle.block:
; CHECK-NEXT:    br i1 true, label [[TMP7:%.*]], label [[SCALAR_PH]]
; CHECK:       scalar.ph:
; CHECK-NEXT:    br label [[TMP6:%.*]]
; CHECK:         br i1 undef, label [[TMP7]], label [[TMP6]], !llvm.loop !7
; CHECK:         ret void
;
  br label %1

; <label>:1                                       ; preds = %1, %0
  %.04 = phi i16* [ %src, %0 ], [ %2, %1 ]
  %.013 = phi i32* [ %dst, %0 ], [ %6, %1 ]
  %i.02 = phi i32 [ 0, %0 ], [ %7, %1 ]
  %2 = getelementptr inbounds i16, i16* %.04, i64 1
  %3 = load i16, i16* %.04, align 2
  %4 = zext i16 %3 to i32
  %5 = shl nuw nsw i32 %4, 7
  %6 = getelementptr inbounds i32, i32* %.013, i64 1
  store i32 %5, i32* %.013, align 4
  %7 = add nsw i32 %i.02, 1
  %exitcond = icmp eq i32 %7, 256
  br i1 %exitcond, label %8, label %1

; <label>:8                                       ; preds = %1
  ret void
}

; We CAN vectorize this example by folding the tail it entails.
define void @example23c(i16* noalias nocapture %src, i32* noalias nocapture %dst) optsize {
; CHECK-LABEL: @example23c(
; CHECK-NEXT:    br i1 false, label [[SCALAR_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[PRED_STORE_CONTINUE22:%.*]] ]
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT:%.*]] = insertelement <4 x i64> undef, i64 [[INDEX]], i32 0
; CHECK-NEXT:    [[BROADCAST_SPLAT:%.*]] = shufflevector <4 x i64> [[BROADCAST_SPLATINSERT]], <4 x i64> undef, <4 x i32> zeroinitializer
; CHECK-NEXT:    [[INDUCTION:%.*]] = add <4 x i64> [[BROADCAST_SPLAT]], <i64 0, i64 1, i64 2, i64 3>
; CHECK-NEXT:    [[TMP1:%.*]] = icmp ult <4 x i64> [[INDUCTION]], <i64 257, i64 257, i64 257, i64 257>
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i1> [[TMP1]], i32 0
; CHECK-NEXT:    br i1 [[TMP2]], label [[PRED_LOAD_IF:%.*]], label [[PRED_LOAD_CONTINUE:%.*]]
; CHECK:       pred.load.if:
; CHECK-NEXT:    [[NEXT_GEP:%.*]] = getelementptr i16, i16* [[SRC:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    [[TMP3:%.*]] = load i16, i16* [[NEXT_GEP]], align 2
; CHECK-NEXT:    br label [[PRED_LOAD_CONTINUE]]
; CHECK:       pred.load.continue:
; CHECK-NEXT:    [[TMP4:%.*]] = phi i16 [ undef, [[VECTOR_BODY]] ], [ [[TMP3]], [[PRED_LOAD_IF]] ]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x i1> [[TMP1]], i32 1
; CHECK-NEXT:    br i1 [[TMP5]], label [[PRED_LOAD_IF11:%.*]], label [[PRED_LOAD_CONTINUE12:%.*]]
; CHECK:       pred.load.if11:
; CHECK-NEXT:    [[TMP6:%.*]] = or i64 [[INDEX]], 1
; CHECK-NEXT:    [[NEXT_GEP4:%.*]] = getelementptr i16, i16* [[SRC]], i64 [[TMP6]]
; CHECK-NEXT:    [[TMP7:%.*]] = load i16, i16* [[NEXT_GEP4]], align 2
; CHECK-NEXT:    br label [[PRED_LOAD_CONTINUE12]]
; CHECK:       pred.load.continue12:
; CHECK-NEXT:    [[TMP8:%.*]] = phi i16 [ undef, [[PRED_LOAD_CONTINUE]] ], [ [[TMP7]], [[PRED_LOAD_IF11]] ]
; CHECK-NEXT:    [[TMP9:%.*]] = extractelement <4 x i1> [[TMP1]], i32 2
; CHECK-NEXT:    br i1 [[TMP9]], label [[PRED_LOAD_IF13:%.*]], label [[PRED_LOAD_CONTINUE14:%.*]]
; CHECK:       pred.load.if13:
; CHECK-NEXT:    [[TMP10:%.*]] = or i64 [[INDEX]], 2
; CHECK-NEXT:    [[NEXT_GEP5:%.*]] = getelementptr i16, i16* [[SRC]], i64 [[TMP10]]
; CHECK-NEXT:    [[TMP11:%.*]] = load i16, i16* [[NEXT_GEP5]], align 2
; CHECK-NEXT:    br label [[PRED_LOAD_CONTINUE14]]
; CHECK:       pred.load.continue14:
; CHECK-NEXT:    [[TMP12:%.*]] = phi i16 [ undef, [[PRED_LOAD_CONTINUE12]] ], [ [[TMP11]], [[PRED_LOAD_IF13]] ]
; CHECK-NEXT:    [[TMP13:%.*]] = extractelement <4 x i1> [[TMP1]], i32 3
; CHECK-NEXT:    br i1 [[TMP13]], label [[PRED_LOAD_IF15:%.*]], label [[PRED_LOAD_CONTINUE16:%.*]]
; CHECK:       pred.load.if15:
; CHECK-NEXT:    [[TMP14:%.*]] = or i64 [[INDEX]], 3
; CHECK-NEXT:    [[NEXT_GEP6:%.*]] = getelementptr i16, i16* [[SRC]], i64 [[TMP14]]
; CHECK-NEXT:    [[TMP15:%.*]] = load i16, i16* [[NEXT_GEP6]], align 2
; CHECK-NEXT:    br label [[PRED_LOAD_CONTINUE16]]
; CHECK:       pred.load.continue16:
; CHECK-NEXT:    [[TMP16:%.*]] = phi i16 [ undef, [[PRED_LOAD_CONTINUE14]] ], [ [[TMP15]], [[PRED_LOAD_IF15]] ]
; CHECK-NEXT:    [[TMP17:%.*]] = extractelement <4 x i1> [[TMP1]], i32 0
; CHECK-NEXT:    br i1 [[TMP17]], label [[PRED_STORE_IF:%.*]], label [[PRED_STORE_CONTINUE:%.*]]
; CHECK:       pred.store.if:
; CHECK-NEXT:    [[TMP18:%.*]] = zext i16 [[TMP4]] to i32
; CHECK-NEXT:    [[TMP19:%.*]] = shl nuw nsw i32 [[TMP18]], 7
; CHECK-NEXT:    [[NEXT_GEP7:%.*]] = getelementptr i32, i32* [[DST:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    store i32 [[TMP19]], i32* [[NEXT_GEP7]], align 4
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE]]
; CHECK:       pred.store.continue:
; CHECK-NEXT:    [[TMP20:%.*]] = extractelement <4 x i1> [[TMP1]], i32 1
; CHECK-NEXT:    br i1 [[TMP20]], label [[PRED_STORE_IF17:%.*]], label [[PRED_STORE_CONTINUE18:%.*]]
; CHECK:       pred.store.if17:
; CHECK-NEXT:    [[TMP21:%.*]] = zext i16 [[TMP8]] to i32
; CHECK-NEXT:    [[TMP22:%.*]] = shl nuw nsw i32 [[TMP21]], 7
; CHECK-NEXT:    [[TMP23:%.*]] = or i64 [[INDEX]], 1
; CHECK-NEXT:    [[NEXT_GEP8:%.*]] = getelementptr i32, i32* [[DST]], i64 [[TMP23]]
; CHECK-NEXT:    store i32 [[TMP22]], i32* [[NEXT_GEP8]], align 4
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE18]]
; CHECK:       pred.store.continue18:
; CHECK-NEXT:    [[TMP24:%.*]] = extractelement <4 x i1> [[TMP1]], i32 2
; CHECK-NEXT:    br i1 [[TMP24]], label [[PRED_STORE_IF19:%.*]], label [[PRED_STORE_CONTINUE20:%.*]]
; CHECK:       pred.store.if19:
; CHECK-NEXT:    [[TMP25:%.*]] = zext i16 [[TMP12]] to i32
; CHECK-NEXT:    [[TMP26:%.*]] = shl nuw nsw i32 [[TMP25]], 7
; CHECK-NEXT:    [[TMP27:%.*]] = or i64 [[INDEX]], 2
; CHECK-NEXT:    [[NEXT_GEP9:%.*]] = getelementptr i32, i32* [[DST]], i64 [[TMP27]]
; CHECK-NEXT:    store i32 [[TMP26]], i32* [[NEXT_GEP9]], align 4
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE20]]
; CHECK:       pred.store.continue20:
; CHECK-NEXT:    [[TMP28:%.*]] = extractelement <4 x i1> [[TMP1]], i32 3
; CHECK-NEXT:    br i1 [[TMP28]], label [[PRED_STORE_IF21:%.*]], label [[PRED_STORE_CONTINUE22]]
; CHECK:       pred.store.if21:
; CHECK-NEXT:    [[TMP29:%.*]] = zext i16 [[TMP16]] to i32
; CHECK-NEXT:    [[TMP30:%.*]] = shl nuw nsw i32 [[TMP29]], 7
; CHECK-NEXT:    [[TMP31:%.*]] = or i64 [[INDEX]], 3
; CHECK-NEXT:    [[NEXT_GEP10:%.*]] = getelementptr i32, i32* [[DST]], i64 [[TMP31]]
; CHECK-NEXT:    store i32 [[TMP30]], i32* [[NEXT_GEP10]], align 4
; CHECK-NEXT:    br label [[PRED_STORE_CONTINUE22]]
; CHECK:       pred.store.continue22:
; CHECK-NEXT:    [[INDEX_NEXT]] = add i64 [[INDEX]], 4
; CHECK-NEXT:    [[TMP32:%.*]] = icmp eq i64 [[INDEX_NEXT]], 260
; CHECK-NEXT:    br i1 [[TMP32]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop !8
; CHECK:       middle.block:
; CHECK-NEXT:    br i1 true, label [[TMP34:%.*]], label [[SCALAR_PH]]
; CHECK:       scalar.ph:
; CHECK-NEXT:    br label [[TMP33:%.*]]
; CHECK:         br i1 undef, label [[TMP34]], label [[TMP33]], !llvm.loop !9
; CHECK:         ret void
;
  br label %1

; <label>:1                                       ; preds = %1, %0
  %.04 = phi i16* [ %src, %0 ], [ %2, %1 ]
  %.013 = phi i32* [ %dst, %0 ], [ %6, %1 ]
  %i.02 = phi i64 [ 0, %0 ], [ %7, %1 ]
  %2 = getelementptr inbounds i16, i16* %.04, i64 1
  %3 = load i16, i16* %.04, align 2
  %4 = zext i16 %3 to i32
  %5 = shl nuw nsw i32 %4, 7
  %6 = getelementptr inbounds i32, i32* %.013, i64 1
  store i32 %5, i32* %.013, align 4
  %7 = add nsw i64 %i.02, 1
  %exitcond = icmp eq i64 %7, 257
  br i1 %exitcond, label %8, label %1

; <label>:8                                       ; preds = %1
  ret void
}

; We CAN'T vectorize this example because it would entail a tail and an
; induction is used outside the loop.
define i64 @example23d(i16* noalias nocapture %src, i32* noalias nocapture %dst) optsize {
;CHECK-LABEL: @example23d(
; CHECK-NOT: <4 x
; CHECK: ret i64
  br label %1

; <label>:1                                       ; preds = %1, %0
  %.04 = phi i16* [ %src, %0 ], [ %2, %1 ]
  %.013 = phi i32* [ %dst, %0 ], [ %6, %1 ]
  %i.02 = phi i64 [ 0, %0 ], [ %7, %1 ]
  %2 = getelementptr inbounds i16, i16* %.04, i64 1
  %3 = load i16, i16* %.04, align 2
  %4 = zext i16 %3 to i32
  %5 = shl nuw nsw i32 %4, 7
  %6 = getelementptr inbounds i32, i32* %.013, i64 1
  store i32 %5, i32* %.013, align 4
  %7 = add nsw i64 %i.02, 1
  %exitcond = icmp eq i64 %7, 257
  br i1 %exitcond, label %8, label %1

; <label>:8                                       ; preds = %1
  ret i64 %7
}
