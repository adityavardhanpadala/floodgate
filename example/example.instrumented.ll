; ModuleID = 'example.ll'
source_filename = "example.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@.str = private unnamed_addr constant [15 x i8] c"Inside level3\0A\00", align 1
@.str.1 = private unnamed_addr constant [15 x i8] c"Inside level2\0A\00", align 1
@.str.2 = private unnamed_addr constant [15 x i8] c"Inside level1\0A\00", align 1
@.str.3 = private unnamed_addr constant [26 x i8] c"Starting example program\0A\00", align 1
@.str.4 = private unnamed_addr constant [26 x i8] c"Finished example program\0A\00", align 1
@0 = private unnamed_addr constant [7 x i8] c"level3\00", align 1
@1 = private unnamed_addr constant [7 x i8] c"level2\00", align 1
@2 = private unnamed_addr constant [7 x i8] c"level1\00", align 1
@3 = private unnamed_addr constant [5 x i8] c"main\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @level3() #0 {
  call void @print_backtrace_if_enabled(ptr @0)
  %1 = call i32 (ptr, ...) @printf(ptr noundef @.str)
  ret void
}

declare dso_local i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @level2() #0 {
  call void @print_backtrace_if_enabled(ptr @1)
  %1 = call i32 (ptr, ...) @printf(ptr noundef @.str.1)
  call void @level3()
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @level1() #0 {
  call void @print_backtrace_if_enabled(ptr @2)
  %1 = call i32 (ptr, ...) @printf(ptr noundef @.str.2)
  call void @level2()
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  call void @print_backtrace_if_enabled(ptr @3)
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = call i32 (ptr, ...) @printf(ptr noundef @.str.3)
  call void @level1()
  %3 = call i32 (ptr, ...) @printf(ptr noundef @.str.4)
  ret i32 0
}

declare void @print_backtrace_if_enabled(ptr)

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 2}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 19.1.7 (Fedora 19.1.7-2.fc41)"}
