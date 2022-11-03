const std = @import("std");
const GitRepoStep = @import("GitRepoStep.zig");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    {
        const exe = b.addExecutable("hacknes", null);
        exe.setTarget(target);
        exe.setBuildMode(mode);
        exe.addCSourceFiles(&[_][]const u8 {
            "hacknes.cpp", "platformPosix.cpp", "cartridge.cpp", "cpu.cpp", "ppu.cpp", "graphics/dummy.cpp", "audio/dummy.cpp",
        }, &[_][]const u8 {});
        exe.linkLibCpp();
        exe.install();
        
        const run_cmd = exe.run();
        run_cmd.step.dependOn(b.getInstallStep());
        if (b.args) |args| {
            run_cmd.addArgs(args);
        }

        const run_step = b.step("run", "Run the app");
        run_step.dependOn(&run_cmd.step);
    }
    {
        const test_rom_repo = GitRepoStep.create(b, .{
            .url = "https://github.com/christopherpow/nes-test-roms",
            .branch = null,
            .sha = "95d8f621ae55cee0d09b91519a8989ae0e64753b",
            .fetch_enabled = true,
        });
        b.step("test-roms", "fetch test rom repo").dependOn(&test_rom_repo.step);
    }
}
