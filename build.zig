const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "hacknes",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{
        .files = &.{
            "hacknes.cpp",
            "platformPosix.cpp",
            "cartridge.cpp",
            "cpu.cpp",
            "ppu.cpp",
            "graphics/dummy.cpp",
            "audio/dummy.cpp",
        },
    });
    exe.linkLibCpp();

    {
        const install_exe = b.addInstallArtifact(exe, .{});
        b.getInstallStep().dependOn(&install_exe.step);
        const run = b.addRunArtifact(exe);
        run.step.dependOn(&install_exe.step);
        if (b.args) |args| run.addArgs(args);
        b.step("run", "").dependOn(&run.step);
    }

    const nes_test_roms_dep = b.dependency("nes_test_roms", .{});
    _ = nes_test_roms_dep;
    // TODO: add some tests?
}
