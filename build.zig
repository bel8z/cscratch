const std = @import("std");

const Builder = std.build.Builder;
const LibExeObjStep = std.build.LibExeObjStep;
const CrossTarget = std.zig.CrossTarget;
const Mode = std.builtin.Mode;

const warnings = [_][]const u8{
    // DO
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Wunused",
    "-Wnull-dereference",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wdouble-promotion",
    // DON'T
    "-Wno-implicit-int-float-conversion", // int 2 float seems ok to me
    "-Wno-nested-anon-types", // anonymous struct members are useful
    "-Wno-gnu-zero-variadic-macro-arguments", // make variadic macros a bit more ergonomic
};

const c_flags = warnings ++ [_][]const u8{
    // Reasonable standard
    "-std=c11", "-pedantic",
};

const cpp_flags = warnings ++ [_][]const u8{
    // Reasonable standard
    "-std=c++14",      "-pedantic",
    // No exceptions and RTTI in my C++
    "-fno-exceptions", "-fno-rtti",
};

fn buildFoundation(
    b: *Builder,
    target: CrossTarget,
    mode: Mode,
) !*LibExeObjStep {
    const lib = b.addStaticLibrary("foundation", null);
    const dir = "code/libs/foundation/";

    lib.setTarget(target);
    lib.setBuildMode(mode);
    lib.install();
    lib.linkLibC();
    lib.addCSourceFiles(&[_][]const u8{
        dir ++ "colors.c",
        dir ++ "error.c",
        dir ++ "io.c",
        dir ++ "list.c",
        dir ++ "log.c",
        dir ++ "memory.c",
        dir ++ "paths.c",
        dir ++ "strings.c",
        dir ++ "task.c",
        dir ++ "threading.c",
        dir ++ "time.c",
    }, &c_flags);

    return lib;
}

pub fn build(b: *Builder) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    _ = buildFoundation(b, target, mode) catch unreachable;

    // const exe = b.addExecutable("cscratch", "src/main.zig");
    // exe.setTarget(target);
    // exe.setBuildMode(mode);
    // exe.install();

    // const run_cmd = exe.run();
    // run_cmd.step.dependOn(b.getInstallStep());
    // if (b.args) |args| {
    //     run_cmd.addArgs(args);
    // }

    // const run_step = b.step("run", "Run the app");
    // run_step.dependOn(&run_cmd.step);

    // const exe_tests = b.addTest("src/main.zig");
    // exe_tests.setTarget(target);
    // exe_tests.setBuildMode(mode);

    // const test_step = b.step("test", "Run unit tests");
    // test_step.dependOn(&exe_tests.step);
}
