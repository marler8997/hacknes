const c = @cImport({
    @cInclude("common.h");
});

extern fn hacknes(
    romFile: [*:0]const u8,
    audioEnabled: bool,
    startAddress: c_int,
) c_int;

pub fn main() !u8 {
    var arena_instance: std.heap.ArenaAllocator = .init(std.heap.page_allocator);
    // no need to free
    const arena = arena_instance.allocator();

    const opt: struct {
        rom_file: [*:0]const u8,
        audio_enabled: bool,
        start_addr: c_int,
    } = blk: {
        var rom_file: ?[*:0]const u8 = null;
        var audio_enabled: bool = true;
        var start_addr: c_int = -1;
        const args_array = try std.process.argsAlloc(arena);
        defer std.process.argsFree(arena, args_array);
        const args = if (args_array.len == 0) args_array else args_array[1..];
        if (args.len == 0) {
            try std.io.getStdErr().writer().writeAll(
                \\Usage: hacknes [options] <rom-file>
                \\  --no-audio               Disable audio
                \\  --no-graphics            Disable graphics
                \\  --start-address <hex>    Address to start execution
                \\
            );
            return 0xff;
        }
        for (args) |arg| {
            if (!std.mem.startsWith(u8, arg, "-")) {
                if (rom_file != null) errExit("too many cmdline args", .{});
                rom_file = arena.dupeZ(u8, arg) catch |e| oom(e);
            } else errExit("unknown cmdline option '{s}'", .{arg});
            _ = &audio_enabled;
            _ = &start_addr;
        }
        break :blk .{
            .audio_enabled = audio_enabled,
            .start_addr = start_addr,
            .rom_file = rom_file orelse errExit(
                "missing ROM_FILE from cmdline",
                .{},
            ),
        };
    };

    return std.math.cast(u8, hacknes(opt.rom_file, opt.audio_enabled, opt.start_addr)) orelse 0xff;
}

fn errExit(comptime format: []const u8, args: anytype) noreturn {
    std.log.err(format, args);
    std.process.exit(1);
}
pub fn oom(e: error{OutOfMemory}) noreturn {
    @panic(@errorName(e));
}

const std = @import("std");
