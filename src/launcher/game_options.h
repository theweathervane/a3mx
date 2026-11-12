/* The game's command-line options, for the launcher's "See option details" window. The same list as
 * docs/BUILDING.md's "Command line (the game)" and src/platform/main.c: keep the three in step. */
#pragma once

struct GameOption {
    const char *option;      /* what is copied, e.g. "--scale 2" */
    const char *syntax;      /* how it is written, shown */
    const char *what;
    bool launcher_sets;      /* the launcher's own controls already pass it */
};

static const GameOption PLAY_OPTIONS[] = {
    { "--fullscreen", "--fullscreen / --windowed", "Open full screen (the default) or in a window. F11 or Alt+Enter switch while playing.", true },
    { "--log-level info", "--log-level LEVEL", "How much goes to the console and work/a3mx.log: off, error, warn, info, debug or verbose. Each adds to the one before.", true },
    { "--renderer soft", "--renderer gl|soft", "The GPU renderer (OpenGL 3.3, the default) or the software reference renderer.", true },
    { "--scale 2", "--scale N", "The GPU's internal resolution, N x 480x272 (1-8). Default: enough to cover the monitor, at most 4 (1 on a software OpenGL). Lower it if fights run slowly.", true },
    { "--no-extensions", "--no-extensions", "The PSP program alone: its own menus and saves, one pad, no F2 / L2 / R2.", false },
    { "--p2 human", "--p2 human|cpu|auto", "Who controls fighter 2: auto is human when gamepad 2 is connected.", false },
    { "--p3 human", "--p3 human|cpu|auto", "The same for fighter 3 (the team modes).", false },
    { "--save-dir saves", "--save-dir DIR", "Keep saves and settings (a3mx.dat / .cfg) in DIR instead of work/.", false },
    { "--assets assets/us", "--assets DIR", "Load each archive that DIR/index.json gives an unpacked folder from it (default: assets/<region>).", false },
    { "--custom-assets", "--custom-assets", "Use the project's own new art in assets/extensions/ (the WIDE FORCED stage edge strips). Off without it.", true },
    { "--no-assets", "--no-assets", "Load everything from the disc, ignoring edited assets.", false },
    { "--log run.log", "--log FILE", "Write the log to FILE instead of work/a3mx.log.", false },
    { "--trace", "--trace", "Log every OS call (slow; the same as --log-level verbose).", false },
};

static const GameOption DEV_OPTIONS[] = {
    { "--frames 600", "--frames N", "Quit after N frames.", false },
    { "--fast", "--fast", "Do not wait for the clock: run as fast as the PC can.", false },
    { "--perf-log perf.csv", "--perf-log FILE", "One CSV row per frame: its time split into game, render, textures, present, idle and GPU.", false },
    { "--input \"200=cross;206=\"", "--input \"FRAME=BUTTONS;...\"", "Scripted buttons by frame (cross circle square triangle start select up down left right l r, joined with +; p2: / p3: for other pads).", false },
    { "--stick \"200=255,128\"", "--stick \"FRAME=LX,LY;...\"", "Scripted analogue stick by frame.", false },
    { "--dump shots", "--dump DIR", "Save the screen as DIR/frame_NNNNN.png (with --dump-every N, --dump-from F).", false },
    { "--hash-log hashes.txt", "--hash-log FILE", "One line per frame: hashes of the program's memory and of the picture (parity tests).", false },
    { "--reference", "--reference", "Every decompiled function runs as its translated original.", false },
    { "--reference-only 0880A000", "--reference-only A,B", "Only these functions (hex addresses) run as the original.", false },
    { "--decomp-report report.txt", "--decomp-report FILE", "How many times each decompiled function ran.", false },
    { "--difftest", "--difftest [N]", "Test decompiled functions against their originals on N random inputs each, then quit.", false },
    { "--snap 600:ram.bin", "--snap FRAME:FILE", "Save the game's memory at a frame.", false },
    { "--poke 600:092E3F10=1/2", "--poke FRAME[-END]:ADDR=VALUE[/SIZE]", "Set memory every frame from FRAME (to END).", false },
    { "--save-fails", "--save-fails", "Every save fails, as on a full memory stick (tests).", false },
    { "--save-questions", "--save-questions", "A failed auto-save asks the game's own questions (tests).", false },
    { "--io-faults busy=7", "--io-faults busy=N,error=N,poll=N,read=N", "A misbehaving disc: every Nth read busy or failing (tests).", false },
    { "--palette-census census.txt", "--palette-census FILE", "Record the colours each indexed texture is drawn with (tools/pair_palettes.py).", false },
    { "--record-audio audio", "--record-audio DIR", "Write each PSP audio channel to DIR/chN.wav.", false },
    { "--net-hub 0", "--net-hub PORT / --net-join PORT N", "Link separate instances over TCP for the network modes (tests).", false },
    { "--pads 12", "--pads LIST", "The gamepads (1-4) shown as plugged in on the MODE MENU's pad icons (tests).", false },
};
