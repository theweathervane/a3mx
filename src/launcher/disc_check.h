/* The launcher's disc check: which disc an ISO is, from its program's checksum.
 *
 * It reads the ISO 9660 image the way tools/extract_assets.py does: the disc's serial from
 * UMD_DATA.BIN, then the SHA-1 of PSP_GAME/SYSDIR/BOOT.BIN (the program, a few MB, so the answer comes
 * in well under a second), then the SHA-1 of the whole image in the background (only to say whether
 * it is the exact known dump; any dump with the right program is fine). Known discs: the table in
 * disc_check.cpp, the same checksums as extract_assets.py's DISCS. */
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

enum Region { REGION_NONE, REGION_US, REGION_JP, REGION_EU };

enum DiscVerdict {
    DISC_EMPTY,          /* nothing to check */
    DISC_CHECKING,
    DISC_OK,             /* the US program: builds */
    DISC_UNSUPPORTED,    /* recognised (JP, EU), not buildable yet */
    DISC_BAD,            /* not an ISO, not this game, or a different / damaged program */
};

struct DiscResult {
    DiscVerdict verdict = DISC_EMPTY;
    Region region = REGION_NONE;
    std::string path;            /* the ISO checked */
    std::string serial;          /* from UMD_DATA.BIN, e.g. ULUS-10062 */
    std::string title;           /* from the table */
    std::string message;         /* one line for the window */
    bool exact = false;          /* the whole image's SHA-1 is the known dump's */
    bool whole_done = false;     /* the whole-image hash has finished */
};

/* Checks one ISO on a thread of its own; the window polls result(). */
class DiscCheck {
public:
    ~DiscCheck();
    void start(const std::string &path);           /* cancels a check still running */
    DiscResult result();
    float whole_progress() const { return progress_.load(); }   /* 0..1 while hashing the whole image */
    bool busy() const { return running_.load(); }

    /* the same check, here and now, without the whole-image hash (for scanning imports/psp/) */
    static DiscResult check_now(const std::string &path);

private:
    void stop();
    std::thread thread_;
    std::mutex mutex_;
    DiscResult result_;
    std::atomic<bool> cancel_{false}, running_{false};
    std::atomic<float> progress_{0};
};

const char *region_name(Region r);
