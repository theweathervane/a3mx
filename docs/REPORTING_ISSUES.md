# Reporting a problem

Found a bug, or have an idea? Please open an issue. This page says where, and what to include, so the
problem can be found without a lot of back and forth.

## Where

The project's home is GitLab, and it is mirrored to GitHub and Gitea. **Issues opened on any of the
three are read and handled**, so use whichever you already have an account on (each needs one):

| | Issues | New issue |
|---|---|---|
| **GitLab** (the project's home) | [gitlab.com/theweathervane-group/a3mx](https://gitlab.com/theweathervane-group/a3mx/-/issues) | [Bug](https://gitlab.com/theweathervane-group/a3mx/-/issues/new?issuable_template=Bug) · [Feature request](https://gitlab.com/theweathervane-group/a3mx/-/issues/new?issuable_template=Feature%20request) |
| **GitHub** (mirror) | [github.com/theweathervane/a3mx](https://github.com/theweathervane/a3mx/issues) | [Bug](https://github.com/theweathervane/a3mx/issues/new?template=bug_report.yml) · [Feature request](https://github.com/theweathervane/a3mx/issues/new?template=feature_request.yml) |
| **Gitea** (mirror) | [gitea.com/theweathervane/a3mx](https://gitea.com/theweathervane/a3mx/issues) | [Choose a form](https://gitea.com/theweathervane/a3mx/issues/new/choose) |

Please open each problem in one place only: the same report on two sites is read twice and answered
in two places. The code, the releases and the merge requests are on GitLab; the mirrors are copies of
it, updated from it.

## The quickest way: the launcher

1. Set **Logging** (under Game) to **INFO**, or **DEBUG** if the problem is hard to pin down. The
   default, OFF, writes nothing, so the log would be empty. (The Report window has a button for this.)
2. Make the problem happen again: press **Run** and do what you did before, then quit the game.
3. Click the **bug** icon in the top left corner (or **About > Report a bug**).
4. Choose **GitLab**, **GitHub** or **Gitea**. The launcher remembers your choice.
5. Click **Open the form on ...**. The launcher:
   - opens a new issue on that site in your browser, filled in with the version, your system, the
     disc's serial, the launcher's settings, the last run's renderer and frame times, and the end of
     the launcher's console (on GitHub, in the bug form's fields);
   - makes **`a3mx-report.zip`** in your logs folder, and opens the folder. The zip holds
     `a3mx.log`, `last_run.txt`, `launcher.log` (this session and the one before),
     `launcher.ini` and `controllers.cfg`, each cut to its end if very long;
   - puts the report on the clipboard too, should the page open empty.
6. **Drag `a3mx-report.zip` into the form.** This is the one step the launcher can't do for you: no
   site lets a web address carry a file, and only you can upload to your account.
7. Fill in the steps, what happened and what you expected, then submit.

Your home folder's path (it has your user name in it) is written as `~` in the form and in the zip.
Nothing is sent anywhere until you submit the form, and you can look inside the zip first.

## Without the launcher

Open a new issue on any of the three sites (the links above give you the form). Attach the logs
from `logs/` in your a3mx folder (`%APPDATA%\a3mx` on Windows; [SAVES.md](SAVES.md)): `a3mx.log`
and, for a slow or stuttering game, `last_run.txt`. To get more in the log, run the game with
`--log-level info` (the default when run by hand) or `--log-level debug`, for example:

```
./build.sh --run -- --log-level debug              # build.cmd --run -- --log-level debug on Windows
build/a3mx --work work --log-level debug        # an existing build
```

## Which version

Releases (`v0.55.0`) are made by hand and are the supported ones. There is no nightly build for now.
If you built from a newer commit than the last release, say so; the launcher's bug button fills in
the full version for you, which tells us exactly which code it was. A nightly's version looks like
`v0.55.0-nightly.20261112+c67e648`, should the nightly be switched back on.

## What to include

| | Why |
|---|---|
| **The version** | Bottom right of the launcher, or OPTION MODE > EX. SETTING in the game (e.g. `v0.34.2`; EX. SETTING writes it in capitals) |
| **Your system** | Windows 11, Ubuntu 24.04, macOS 15 ... |
| **The disc** | The serial the launcher shows under Image (`ULUS-10062` for the US disc, the only one that builds so far) |
| **How you ran it** | The launcher's settings, or the command you typed. With or without the extensions makes a big difference. |
| **The mode and controllers** | Arcade, Dramatic Battle, NETWORK ...; keyboard, which pads |
| **Steps** | What to do, from the MODE MENU, to see it happen. "Every time" or "once in a while" helps too. |
| **What happened, and what you expected** | A screenshot or a short video, if it is something you can see |
| **The log** | `a3mx-report.zip` from the launcher, or `logs/a3mx.log`, attached as a file |
| **Renderer and frame times** (slow or stuttering games) | Filled in by the launcher's bug button from your last run (`logs/last_run.txt`: the renderer, your GPU, the scale, vsync, FRAME TIMING and which timing was used, the monitor's refresh rate, how long frames took, and how often a frame was shown twice: `hitches`). Run the slow part first, quit the game, then press the bug button. Without the launcher, paste that file. With Logging at INFO, the log also has a frame time line every 10 seconds. |

**A build that fails:** first try starting from nothing: **Clean and try again** in
`--clean` with any of the scripts (`build.cmd --clean`, `./launcher.sh
--clean`, ...). Your saves and settings are kept. If it still fails, copy the launcher's whole console (the copy icon above it) into the issue, or the
terminal's output if you ran `build.cmd` / `build.sh`. The first red line is usually the one that
matters.

**Slow fights or stutter:** first try a lower **Resolution** in the launcher (2x or 1x, or `--scale 2`).
On a laptop, check the launcher's **GPU** row: its tooltip names the GPU the last run drew on, which
should be the fast one. Say in the report whether either helped.

**The game stops with an error:** the log's last lines start with `FATAL:`. They are written whatever
the log level.

**The log levels:** each adds to the one before it.
- **OFF**: nothing but a fatal error.
- **ERROR** and **WARN**: things that went wrong, or that look wrong.
- **INFO**: what the game is doing.
- **DEBUG**: more detail.
- **VERBOSE**: every call the game makes to the PSP's system. It is large and slows the game down,
  so use it only when asked.

## Please don't

- Attach your ISO, or anything extracted from it (`work/`, `assets/`). It's the game's data. The log,
  screenshots and your saves (`saves/a3mx.dat`) are fine.
- Report problems in other projects here (emulators, other ports).

## Ideas and questions

Each of the three sites has the same forms:

- **Report a problem**: the form the launcher fills in.
- **Request a feature**: an idea, and why it would help (**About > Request a feature** in the launcher).
- **A blank issue**: anything else, such as a question or a problem with the docs.

Merge requests are welcome too, on GitLab; see [CONTRIBUTING.md](../CONTRIBUTING.md).
