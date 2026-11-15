/* report.cpp: the bug report's addresses, its Markdown, and the .zip of the logs (report.h) */
#include "report.h"
#include <ctime>

const IssueHost ISSUE_HOSTS[HOST_COUNT] = {
    { "gitlab", "GitLab", "the project's home", "https://gitlab.com/theweathervane-group/a3mx" },
    { "github", "GitHub", "mirror",             "https://github.com/theweathervane/a3mx" },
    { "gitea",  "Gitea",  "mirror",             "https://gitea.com/theweathervane/a3mx" },
};

int issue_host_find(const std::string &id) {
    for (int i = 0; i < HOST_COUNT; i++) if (id == ISSUE_HOSTS[i].id) return i;
    return HOST_GITLAB;
}

std::string url_encode(const std::string &s) {
    static const char *hex = "0123456789ABCDEF";
    std::string o;
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c;
        else { o += '%'; o += hex[c >> 4]; o += hex[c & 15]; }
    }
    return o;
}

static const char *const ZIP_NAME = "a3mx-report.zip";

static std::string fenced(const std::string &s) { return "```\n" + s + (s.empty() || s.back() == '\n' ? "" : "\n") + "```\n"; }

std::string issue_body(const IssueFields &f) {
    if (f.feature)
        return "## The idea\n\n(what you would like)\n\n## Why\n\n(what it would help with)\n\n## Details\n\n- Version: " + f.version + "\n";
    std::string b = "## What happened\n\n(what you did, what you expected, what you saw instead)\n\n"
                    "## Steps\n\n1. \n\n## Details\n\n" + f.details;
    if (!f.last_run.empty()) b += "\n## The last run\n\n" + fenced(f.last_run);
    b += std::string("\n## Logs\n\nPlease attach `") + ZIP_NAME + "` from your logs folder (the launcher made it and opened the folder): "
         "it holds a3mx.log, last_run.txt and launcher.log.\n";
    if (!f.console.empty()) b += "\nThe end of the launcher's console:\n\n" + fenced(f.console);
    return b;
}

/* one address, before trimming */
static std::string url_once(int host, const IssueFields &f) {
    const IssueHost &h = ISSUE_HOSTS[host];
    std::string title = f.feature ? "Feature request: " : "Bug: ";
    switch (host) {
    case HOST_GITHUB: {
        /* the issue forms in .github/ISSUE_TEMPLATE: each field filled by its id */
        if (f.feature) return std::string(h.repo) + "/issues/new?template=feature_request.yml&title=" + url_encode(title) + "&version=" + url_encode(f.version);
        std::string u = std::string(h.repo) + "/issues/new?template=bug_report.yml&title=" + url_encode(title) +
                        "&details=" + url_encode(f.details);
        if (!f.last_run.empty()) u += "&last_run=" + url_encode(f.last_run);
        if (!f.console.empty()) u += "&console=" + url_encode(f.console);
        return u;
    }
    case HOST_GITEA:
        return std::string(h.repo) + "/issues/new?title=" + url_encode(title) + "&body=" + url_encode(issue_body(f));
    default:   /* GitLab: as .recomp.json's "Report a problem" link */
        return std::string(h.repo) + "/-/work_items/new?type=Issue&initialCreationContext=list-route&issue[title]=" + url_encode(title) +
               "&issue[description]=" + url_encode(issue_body(f));
    }
}

std::string issue_url(int host, IssueFields f, size_t max_len) {
    if (host < 0 || host >= HOST_COUNT) host = HOST_GITLAB;
    std::string u = url_once(host, f);
    while (u.size() > max_len && !f.console.empty()) {       /* the oldest console lines first */
        size_t nl = f.console.find('\n');
        f.console = nl == std::string::npos ? std::string() : f.console.substr(nl + 1);
        u = url_once(host, f);
    }
    if (u.size() > max_len) { f.last_run.clear(); u = url_once(host, f); }
    return u;
}

std::string issues_page(int host) {
    if (host < 0 || host >= HOST_COUNT) host = HOST_GITLAB;
    return std::string(ISSUE_HOSTS[host].repo) + (host == HOST_GITLAB ? "/-/issues" : "/issues");
}

std::string repo_file_page(int host, const char *path) {
    if (host < 0 || host >= HOST_COUNT) host = HOST_GITLAB;
    const char *mid = host == HOST_GITLAB ? "/-/blob/main/" : host == HOST_GITHUB ? "/blob/main/" : "/src/branch/main/";
    return std::string(ISSUE_HOSTS[host].repo) + mid + path;
}

static void replace_all(std::string &s, const std::string &from, const std::string &to) {
    if (from.empty()) return;
    for (size_t at = s.find(from); at != std::string::npos; at = s.find(from, at + to.size())) s.replace(at, from.size(), to);
}

std::string redact_home(const std::string &text, const std::string &home) {
    std::string h = home;
    while (h.size() > 1 && (h.back() == '/' || h.back() == '\\')) h.pop_back();
    if (h.size() < 3) return text;                           /* "/" or "C:": nothing personal to hide */
    std::string fwd = h, back = h;
    for (char &c : fwd) if (c == '\\') c = '/';
    for (char &c : back) if (c == '/') c = '\\';
    std::string out = text;
    replace_all(out, h, "~");
    replace_all(out, fwd, "~");
    replace_all(out, back, "~");
    return out;
}

std::string keep_tail(const std::string &text, size_t max) {
    if (text.size() <= max) return text;
    size_t from = text.size() - max;
    size_t nl = text.find('\n', from);
    if (nl != std::string::npos && nl + 1 < text.size()) from = nl + 1;
    return "(the first " + std::to_string(from) + " bytes were left out: the report keeps the end)\n" + text.substr(from);
}

uint32_t crc32_of(const void *data, size_t n) {
    static uint32_t table[256];
    static bool made = false;
    if (!made) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        made = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < n; i++) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static void put16(std::string &o, uint32_t v) { o += (char)(v & 0xFF); o += (char)((v >> 8) & 0xFF); }
static void put32(std::string &o, uint32_t v) { put16(o, v & 0xFFFF); put16(o, v >> 16); }

std::string zip_build(const std::vector<ZipEntry> &entries) {
    /* the MS-DOS date and time the format keeps, from the local clock */
    std::time_t now = std::time(nullptr);
    std::tm t = {};
#ifdef _WIN32
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    uint32_t dos_time = (uint32_t)(t.tm_hour << 11 | t.tm_min << 5 | t.tm_sec / 2);
    uint32_t dos_date = (uint32_t)((t.tm_year > 80 ? t.tm_year - 80 : 0) << 9 | (t.tm_mon + 1) << 5 | t.tm_mday);

    std::string out, central;
    for (const ZipEntry &e : entries) {
        if (out.size() + e.data.size() > 0xFFFFFF00u) return std::string();   /* would need ZIP64 (keep_tail keeps it far smaller) */
        uint32_t crc = crc32_of(e.data.data(), e.data.size()), size = (uint32_t)e.data.size(), offset = (uint32_t)out.size();
        auto header = [&](std::string &o, bool is_central) {
            put32(o, is_central ? 0x02014B50u : 0x04034B50u);
            if (is_central) put16(o, 20);                   /* made by: 2.0 */
            put16(o, 20);                                   /* needed: 2.0 */
            put16(o, 0x0800);                               /* names are UTF-8 */
            put16(o, 0);                                    /* stored */
            put16(o, dos_time); put16(o, dos_date);
            put32(o, crc); put32(o, size); put32(o, size);
            put16(o, (uint32_t)e.name.size());
            put16(o, 0);                                    /* extra */
            if (is_central) { put16(o, 0); put16(o, 0); put16(o, 0); put32(o, 0); put32(o, offset); }
            o += e.name;
        };
        header(out, false);
        out += e.data;
        header(central, true);
    }
    uint32_t cd_at = (uint32_t)out.size();
    out += central;
    put32(out, 0x06054B50u);
    put16(out, 0); put16(out, 0);
    put16(out, (uint32_t)entries.size()); put16(out, (uint32_t)entries.size());
    put32(out, (uint32_t)central.size()); put32(out, cd_at);
    put16(out, 0);

    return out;
}
