/* The build's dependencies, for the launcher's Dependencies tab: what this system's build downloads
 * (from tools/data/pins.json: the same list tools/build.py and CMake check every download against,
 * each with its SHA-256), what must already be on the system, and which of them are here now.
 * Reads files only: nothing is downloaded from here (tools/build.py does that, and checks each hash). */
#pragma once
#include <string>
#include <vector>

struct Dep {
    std::string id, name, version, url, sha256, sha_of;   /* sha_of: the file the hash is of, when it isn't the download itself */
    std::string dest;            /* where it goes, under the checkout (or the tools folder) */
    std::string when;            /* "" a build tool, downloaded when asked; else when it is downloaded by itself */
    bool here = false;
    bool system = false;         /* not downloaded: has to be on the system (Linux, macOS): cmake, ninja, clang, python3 */
    bool optional = false;       /* the build and the game work without it (the FILTER setting's shaders and library) */
    std::string detail;          /* system: where it was found, or how to get it */
};

/* ROOT: the checkout. TOOLS: the tools folder ("" = ROOT/.tools). err: why the list is empty. */
std::vector<Dep> deps_list(const std::string &root, const std::string &tools, std::string &err);
bool deps_any_missing(const std::vector<Dep> &d);                 /* a build tool that would be downloaded, or a system one not found */
bool deps_any_to_download(const std::vector<Dep> &d);            /* something tools/build.py --tools-only would fetch now */
std::string deps_signature(const std::vector<Dep> &d);           /* changes when the pinned list does: consent is asked again */
