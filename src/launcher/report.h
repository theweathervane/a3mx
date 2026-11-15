/* report.h: the parts of a bug report that need no window (report.cpp).
 *
 * The project lives on GitLab and is mirrored to GitHub and Gitea; an issue opened on any of the three
 * is read and handled. The launcher's Report window lets the player choose one, then opens that site's
 * new-issue form filled in from the address (GitHub: its issue form's fields, by id, from
 * .github/ISSUE_TEMPLATE/bug_report.yml; GitLab and Gitea: a title and a Markdown description).
 *
 * No site lets an address carry a file, so the logs go into one .zip (stored, no compression: no zlib
 * here) beside them for the player to drag into the form, with their home folder's path written as ~. */
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum IssueHostId { HOST_GITLAB, HOST_GITHUB, HOST_GITEA, HOST_COUNT };
struct IssueHost {
    const char *id;      /* saved in launcher.ini (report_host=) */
    const char *name;    /* GitLab, GitHub, Gitea */
    const char *role;    /* "the project's home", "mirror" */
    const char *repo;    /* the project's page */
};
extern const IssueHost ISSUE_HOSTS[HOST_COUNT];
int issue_host_find(const std::string &id);                /* HOST_GITLAB if unknown */

/* what the launcher knows, for the form */
struct IssueFields {
    bool feature = false;     /* a feature request, not a bug */
    std::string version;      /* v0.83.0 */
    std::string details;      /* "- Version: ...\n- System: ...\n..." (Markdown list) */
    std::string last_run;     /* logs/last_run.txt as a few lines, or "" */
    std::string console;      /* the end of the launcher's console, or "" */
};
/* the report as Markdown: the description on GitLab and Gitea, and what goes on the clipboard */
std::string issue_body(const IssueFields &f);
/* the new-issue form on that site, filled in. Addresses over max_len are refused by some sites
   (GitHub's limit is about 8 KB), so the console is cut from the top, then dropped, to fit */
std::string issue_url(int host, IssueFields f, size_t max_len = 7000);
std::string issues_page(int host);                         /* the list of issues */
std::string repo_file_page(int host, const char *path);    /* a file on main, e.g. docs/REPORTING_ISSUES.md */

std::string url_encode(const std::string &s);

/* every spelling of the home folder in text (as given, with / or \, without its trailing separator)
   written as ~, so a report doesn't carry the player's user name */
std::string redact_home(const std::string &text, const std::string &home);
/* at most max bytes from the end of text, starting at a line, with a note of what was cut */
std::string keep_tail(const std::string &text, size_t max);

struct ZipEntry { std::string name, data; };
uint32_t crc32_of(const void *data, size_t n);
/* the bytes of a .zip of the entries, stored (method 0), dated now ("" if over 4 GB). The launcher
   saves it with SDL_SaveFile, which takes UTF-8 paths on every system */
std::string zip_build(const std::vector<ZipEntry> &entries);
