/* The launcher's child process (process.h). */
#include "process.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
static std::wstring widen(const std::string &s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
#endif

/* One run's output: the thread reads the pipe until it closes, into `pending`; poll() takes it.
 * Shared with the thread, so a run given up on (its pipe held open by something it started) can
 * be left to finish on its own. */
struct OutputReader {
    std::mutex lock;
    std::string pending;
    std::atomic<bool> open{true};
#ifdef _WIN32
    void *pipe = nullptr;
#else
    int pipe = -1;
#endif
    void run() {
        char buf[65536];
        for (;;) {
#ifdef _WIN32
            DWORD n = 0;
            if (!ReadFile(pipe, buf, sizeof buf, &n, NULL) || n == 0) break;
#else
            ssize_t n = read(pipe, buf, sizeof buf);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) break;
#endif
            std::lock_guard<std::mutex> g(lock);
            pending.append(buf, (size_t)n);
        }
#ifdef _WIN32
        CloseHandle(pipe);
#else
        close(pipe);
#endif
        open = false;
    }
};

static std::shared_ptr<OutputReader> start_reader(
#ifdef _WIN32
    void *pipe
#else
    int pipe
#endif
) {
    auto r = std::make_shared<OutputReader>();
    r->pipe = pipe;
    std::thread([r] { r->run(); }).detach();
    return r;
}

/* Windows: cmdline is passed to CreateProcess as it is. POSIX: argv. */
bool Child::start(const std::vector<std::string> &argv, const std::string &cmdline, const std::string &cwd, std::string &err) {
#ifdef _WIN32
    (void)argv;
    SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
    HANDLE rd = NULL, wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 1 << 20)) { err = "CreatePipe failed"; return false; }   /* 1 MB: room for bursts */
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    STARTUPINFOW si; memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nul; si.hStdOutput = wr; si.hStdError = wr;
    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof pi);
    std::wstring cl = widen(cmdline), wd = widen(cwd);
    /* CREATE_NO_WINDOW: a hidden console that cmd, Python, CMake and the game all share, so none
     * of them opens a console window; the game's own window still shows. */
    BOOL ok = CreateProcessW(NULL, &cl[0], NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL,
                             wd.empty() ? NULL : wd.c_str(), &si, &pi);
    CloseHandle(wr);
    if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
    if (!ok) { CloseHandle(rd); err = "could not start: error " + std::to_string((unsigned)GetLastError()); return false; }
    /* a job holds the whole tree: Stop ends cmd, Python, CMake, Ninja, the compilers and the game */
    job = CreateJobObjectW(NULL, NULL);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION li; memset(&li, 0, sizeof li);
        li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof li);
        AssignProcessToJobObject(job, pi.hProcess);
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    process = pi.hProcess; out = NULL;
    reader = start_reader(rd);
#else
    (void)cmdline;
    int p[2];
    if (pipe(p) != 0) { err = std::string("pipe: ") + strerror(errno); return false; }
    std::vector<char *> av;
    for (const std::string &a : argv) av.push_back(const_cast<char *>(a.c_str()));
    av.push_back(NULL);
    pid_t k = fork();
    if (k < 0) { close(p[0]); close(p[1]); err = std::string("fork: ") + strerror(errno); return false; }
    if (k == 0) {                                   /* the child: its own process group, so Stop reaches it all */
        setpgid(0, 0);
        int nul = open("/dev/null", O_RDONLY);
        if (nul >= 0) { dup2(nul, 0); close(nul); }
        dup2(p[1], 1); dup2(p[1], 2);
        close(p[0]); close(p[1]);
        signal(SIGPIPE, SIG_DFL);
        if (!cwd.empty() && chdir(cwd.c_str()) != 0) { fprintf(stderr, "cannot enter %s\n", cwd.c_str()); _exit(127); }
        execvp(av[0], av.data());
        fprintf(stderr, "cannot start %s: %s\n", av[0], strerror(errno));
        _exit(127);
    }
    setpgid(k, k);
    close(p[1]);
    fcntl(p[0], F_SETFD, FD_CLOEXEC);
    pid = k; out = -1; term_sent = 0;
    reader = start_reader(p[0]);
#endif
    running = reading = true; exit_code = 0;
    return true;
}


/* whatever output is waiting, without blocking; also notices the exit */
void Child::poll(std::string &got) {
    if (reader) {                                   /* what the reader thread has taken in */
        std::lock_guard<std::mutex> g(reader->lock);
        if (!reader->pending.empty()) { got.append(reader->pending); reader->pending.clear(); }
    }
    bool pipe_open = reader && reader->open;
#ifdef _WIN32
    if (running && WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
        DWORD code = 0; GetExitCodeProcess(process, &code);
        exit_code = (int)code; running = false;
        CloseHandle(process); process = NULL;
    }
    reading = pipe_open;
    if (!running && !reading && job) { CloseHandle(job); job = NULL; }
#else
    if (running) {
        int st = 0;
        pid_t r = waitpid(pid, &st, WNOHANG);
        if (r == pid) {
            exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : 128 + (WIFSIGNALED(st) ? WTERMSIG(st) : 0);
            running = false;
        }
    }
    reading = pipe_open;
    if (term_sent && SDL_GetTicks() - term_sent > 3000) { kill(-pid, SIGKILL); term_sent = 0; }
    /* the main process is gone but something it started still holds the pipe: give up on it (the
     * reader thread finishes by itself when that lets go) */
    if (!running && reading && term_sent == 0 && pid > 0 && kill(-pid, 0) != 0) reading = false;
#endif
    if (!reading && !running) reader.reset();
}


void Child::stop() {
#ifdef _WIN32
    if (job) TerminateJobObject(job, 1);
    else if (process) TerminateProcess(process, 1);
#else
    if (pid > 0 && busy()) { kill(-pid, SIGTERM); term_sent = SDL_GetTicks(); if (!term_sent) term_sent = 1; }
#endif
}


#ifdef _WIN32
#include <tlhelp32.h>

static bool runs_exe(DWORD pid, const std::wstring &exe) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    wchar_t path[MAX_PATH * 2];
    DWORD n = MAX_PATH * 2;
    bool same = QueryFullProcessImageNameW(h, 0, path, &n) && CompareStringOrdinal(path, (int)n, exe.c_str(), (int)exe.size(), TRUE) == CSTR_EQUAL;
    CloseHandle(h);
    return same;
}

static std::wstring win_path(const std::string &exe) {
    std::string p = exe;
    for (char &c : p) if (c == '/') c = '\\';
    std::wstring w = widen(p), full(MAX_PATH * 2, L'\0');
    DWORD n = GetFullPathNameW(w.c_str(), (DWORD)full.size(), &full[0], NULL);
    if (n == 0 || n >= full.size()) return w;
    full.resize(n);
    return full;
}

struct CloseScan { std::wstring exe; int found; };
static BOOL CALLBACK close_window(HWND w, LPARAM lp) {
    CloseScan *s = (CloseScan *)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid && IsWindowVisible(w) && runs_exe(pid, s->exe)) { PostMessageW(w, WM_CLOSE, 0, 0); s->found++; }
    return TRUE;
}

int Child::close_game(const std::string &exe) {
    CloseScan s = { win_path(exe), 0 };
    EnumWindows(close_window, (LPARAM)&s);
    return s.found;
}

int Child::end_game(const std::string &exe) {
    stop();
    std::wstring w = win_path(exe);
    int found = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe; pe.dwSize = sizeof pe;
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (!runs_exe(pe.th32ProcessID, w)) continue;
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
        if (h) { if (TerminateProcess(h, 1)) found++; CloseHandle(h); }
    }
    CloseHandle(snap);
    return found;
}
#else
int Child::close_game(const std::string &) {
    if (pid > 0 && busy()) { kill(-pid, SIGTERM); return 1; }    /* SDL turns SIGTERM into a quit: the game saves */
    return 0;
}
int Child::end_game(const std::string &) {
    if (pid > 0 && busy()) { kill(-pid, SIGKILL); return 1; }
    return 0;
}
#endif

#ifdef _WIN32
std::string win_quote(const std::string &a) {
    if (!a.empty() && a.find_first_of(" \t\"") == std::string::npos) return a;
    std::string q = "\"";
    size_t slashes = 0;
    for (char ch : a) {
        if (ch == '\\') { slashes++; continue; }
        if (ch == '"') q.append(slashes * 2 + 1, '\\');
        else q.append(slashes, '\\');
        slashes = 0;
        q += ch;
    }
    q.append(slashes * 2, '\\');
    return q + "\"";
}
#endif

std::vector<std::string> split_args(const std::string &s) {
    std::vector<std::string> out;
    std::string cur; bool in = false, any = false;
    for (char ch : s) {
        if (ch == '"') { in = !in; any = true; }
        else if ((ch == ' ' || ch == '\t') && !in) { if (any) out.push_back(cur); cur.clear(); any = false; }
        else { cur += ch; any = true; }
    }
    if (any) out.push_back(cur);
    return out;
}

