/* The launcher's child process: build.cmd / build.sh, or the game, with its output read through a
 * pipe without blocking. Windows: CreateProcess with CREATE_NO_WINDOW (one hidden console shared by
 * everything it starts, so no console window opens) inside a job object, so stop() ends the whole
 * tree. POSIX: fork into a process group of its own, SIGTERM then SIGKILL to the group. */
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* The child's output, read by a thread of its own as fast as it comes, so the child never waits
 * on the launcher (a full pipe would stop the game at its next log line until the window drew). */
struct OutputReader;

struct Child {
#ifdef _WIN32
    void *process = nullptr, *job = nullptr, *out = nullptr;     /* HANDLEs */
#else
    int pid = -1;
    int out = -1;
    uint64_t term_sent = 0;         /* when SIGTERM went to the group (SIGKILL follows if it lingers) */
#endif
    bool running = false;           /* the process has not exited yet */
    bool reading = false;           /* its output pipe is still open */
    std::shared_ptr<OutputReader> reader;
    int exit_code = 0;

    bool busy() const { return running || reading; }
    /* Windows: cmdline is passed to CreateProcess as it is. POSIX: argv. */
    bool start(const std::vector<std::string> &argv, const std::string &cmdline, const std::string &cwd, std::string &err);
    void poll(std::string &got);    /* whatever output is waiting, without blocking; also notices the exit */
    void stop();
    /* Closing the launcher while the game runs. close_game asks politely, so the game saves as it
     * would from its own close button: Windows, WM_CLOSE to every window of a process running EXE;
     * POSIX, SIGTERM to the group (SDL turns it into a quit). end_game is the fallback: stop(),
     * then, on Windows, every process still running EXE is ended. The job usually holds the game
     * already; this also catches one that is outside it (a Python started through an app alias,
     * as the Microsoft Store and Python's install manager put on the PATH, starts its children
     * outside the job). Each returns how many processes it found. */
    int close_game(const std::string &exe);
    int end_game(const std::string &exe);
};

#ifdef _WIN32
std::string win_quote(const std::string &a);     /* quoted as CommandLineToArgvW and the C runtime read it */
#endif
std::vector<std::string> split_args(const std::string &s);   /* "a b" c -> [a b] [c] */
