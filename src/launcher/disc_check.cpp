/* The launcher's disc check (disc_check.h). */
#include "disc_check.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstring>
#include <vector>

/* ================================================================ the known discs
 * As tools/extract_assets.py's DISCS, plus the European disc (checksums from the maintainer's copy,
 * 2026-11-03; docs/VERSION_DIFFERENCES.md). Only the US program builds today. */
struct KnownDisc {
    const char *serial, *title, *iso_sha1, *boot_sha1;
    Region region;
    bool buildable;
};
static const KnownDisc DISCS[] = {
    { "ULUS-10062", "Street Fighter Alpha 3 MAX", "321ff2143b4c3e8aa24efcf52ffe577c9739f57b",
      "3fd0abf7cc65742233e1b7787997fa3e20152d98", REGION_US, true },
    { "ULJM-05082", "Street Fighter Zero 3 Double Upper", "86b75c0e299400722d5e4e1f7077730da0fbac11",
      "d16de5a2bcd610a4841a15427b3716d33a4089bc", REGION_JP, false },
    { "ULES-00235", "Street Fighter Alpha 3 MAX", "6562c25bf07bc81ebcf7375105ec5bcce7daedc3",
      "dc789772c15b2c9e1daa41ef7cfab1b5a65f512e", REGION_EU, false },
};

const char *region_name(Region r) {
    switch (r) {
    case REGION_US: return "US";
    case REGION_JP: return "Japanese";
    case REGION_EU: return "European";
    default: return "unknown";
    }
}

/* ================================================================ SHA-1 (FIPS 180-1) */
struct Sha1 {
    uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    uint8_t block[64];
    size_t used = 0;
    uint64_t bytes = 0;

    static uint32_t rol(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }
    void compress(const uint8_t *p) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++) w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
        for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & d);           k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;                    k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d);  k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;                    k = 0xCA62C1D6; }
            uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    void update(const void *data, size_t n) {
        const uint8_t *p = (const uint8_t *)data;
        bytes += n;
        if (used) {
            size_t take = 64 - used < n ? 64 - used : n;
            memcpy(block + used, p, take); used += take; p += take; n -= take;
            if (used == 64) { compress(block); used = 0; }
        }
        while (n >= 64) { compress(p); p += 64; n -= 64; }
        if (n) { memcpy(block, p, n); used = n; }
    }
    std::string hex() {
        uint64_t bits = bytes * 8;
        uint8_t pad = 0x80, zero = 0;
        update(&pad, 1);
        while (used != 56) update(&zero, 1);
        uint8_t len[8];
        for (int i = 0; i < 8; i++) len[i] = (uint8_t)(bits >> (56 - 8 * i));
        update(len, 8);
        static const char *digits = "0123456789abcdef";
        std::string s;
        for (int i = 0; i < 5; i++)
            for (int j = 28; j >= 0; j -= 4) s += digits[(h[i] >> j) & 15];
        return s;
    }
};

/* ================================================================ ISO 9660 */
static const int SECTOR = 2048;

struct Iso {
    SDL_IOStream *io = NULL;
    Sint64 size = 0;
    ~Iso() { if (io) SDL_CloseIO(io); }

    bool open(const std::string &path) {
        io = SDL_IOFromFile(path.c_str(), "rb");
        if (!io) return false;
        size = SDL_GetIOSize(io);
        return size > 17 * SECTOR;
    }
    bool read(Sint64 at, void *out, size_t n) {
        if (SDL_SeekIO(io, at, SDL_IO_SEEK_SET) < 0) return false;
        return SDL_ReadIO(io, out, n) == n;
    }
    /* the directory record of PATH ("PSP_GAME/SYSDIR/BOOT.BIN"): its sector and size */
    bool find(const char *path, uint32_t &lba, uint32_t &len) {
        uint8_t pvd[SECTOR];
        if (!read(16 * SECTOR, pvd, SECTOR) || pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5)) return false;
        lba = le32(pvd + 156 + 2); len = le32(pvd + 156 + 10);
        const char *p = path;
        while (*p) {
            const char *slash = strchr(p, '/');
            size_t nl = slash ? (size_t)(slash - p) : strlen(p);
            if (len > 4u << 20) return false;
            std::vector<uint8_t> dir(len);
            if (!read((Sint64)lba * SECTOR, dir.data(), len)) return false;
            bool found = false;
            for (uint32_t pos = 0; pos < len;) {
                uint8_t n = dir[pos];
                if (!n) { pos = (pos / SECTOR + 1) * SECTOR; continue; }         /* records do not cross sectors */
                if (pos + n > len || n < 34) break;
                const uint8_t *r = &dir[pos];
                size_t namelen = r[32];
                const char *name = (const char *)r + 33;
                size_t cut = namelen;
                for (size_t i = 0; i < namelen; i++) if (name[i] == ';') { cut = i; break; }
                if (cut == nl && SDL_strncasecmp(name, p, nl) == 0) { lba = le32(r + 2); len = le32(r + 10); found = true; break; }
                pos += n;
            }
            if (!found) return false;
            p += nl + (slash ? 1 : 0);
        }
        return true;
    }
    bool file(const char *path, std::vector<uint8_t> &out, uint32_t max) {
        uint32_t lba, len;
        if (!find(path, lba, len) || len > max) return false;
        out.resize(len);
        return read((Sint64)lba * SECTOR, out.data(), len);
    }
    static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
};

/* ================================================================ the check */
DiscResult DiscCheck::check_now(const std::string &path) {
    DiscResult r;
    r.path = path;
    r.verdict = DISC_BAD;
    Iso iso;
    if (!iso.open(path)) { r.message = "Cannot read this file as a disc image."; return r; }
    std::vector<uint8_t> umd, boot;
    if (!iso.file("UMD_DATA.BIN", umd, 4096) || !iso.file("PSP_GAME/SYSDIR/BOOT.BIN", boot, 64u << 20)) {
        r.message = "Not a PSP disc image (ISO 9660 with PSP_GAME/SYSDIR/BOOT.BIN).";
        return r;
    }
    for (uint8_t c : umd) { if (c == '|' || c < 32 || c > 126) break; r.serial += (char)c; }
    Sha1 h;
    h.update(boot.data(), boot.size());
    std::string boot_sha = h.hex();

    const KnownDisc *k = NULL;
    for (const KnownDisc &d : DISCS) if (r.serial == d.serial) k = &d;
    if (!k) {
        r.message = "This is not A3MX (disc " + (r.serial.empty() ? std::string("?") : r.serial) + ").";
        return r;
    }
    r.region = k->region;
    r.title = k->title;
    if (k->boot_sha1 && boot_sha != k->boot_sha1) {
        r.message = std::string(k->title) + " (" + k->serial + "), but its program is not the known one: "
                    "another version, or a damaged dump.";
        return r;
    }
    if (!k->buildable) {
        r.verdict = DISC_UNSUPPORTED;
        r.message = std::string(k->title) + " (" + k->serial + ", " + region_name(k->region) + "): " +
                    "recognised, but only the US disc builds so far.";
        return r;
    }
    r.verdict = DISC_OK;
    r.message = std::string(k->title) + " (" + k->serial + ", " + region_name(k->region) + "): program checked.";
    return r;
}

DiscCheck::~DiscCheck() { stop(); }

void DiscCheck::stop() {
    cancel_ = true;
    if (thread_.joinable()) thread_.join();
    cancel_ = false;
}

void DiscCheck::start(const std::string &path) {
    stop();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result_ = DiscResult();
        result_.path = path;
        result_.verdict = path.empty() ? DISC_EMPTY : DISC_CHECKING;
    }
    if (path.empty()) return;
    running_ = true;
    progress_ = 0;
    thread_ = std::thread([this, path]() {
        DiscResult r = check_now(path);
        { std::lock_guard<std::mutex> lock(mutex_); result_ = r; }
        if (r.verdict == DISC_OK || r.verdict == DISC_UNSUPPORTED) {
            /* the whole image, to say whether it is the exact known dump */
            const char *want = NULL;
            for (const KnownDisc &d : DISCS) if (r.serial == d.serial) want = d.iso_sha1;
            SDL_IOStream *io = SDL_IOFromFile(path.c_str(), "rb");
            if (io && want) {
                Sint64 total = SDL_GetIOSize(io), done = 0;
                std::vector<uint8_t> buf(1 << 20);
                Sha1 h;
                size_t n;
                while (!cancel_ && (n = SDL_ReadIO(io, buf.data(), buf.size())) > 0) {
                    h.update(buf.data(), n);
                    done += (Sint64)n;
                    progress_ = total > 0 ? (float)done / (float)total : 0;
                }
                if (!cancel_) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    result_.exact = h.hex() == want;
                    result_.whole_done = true;
                }
            }
            if (io) SDL_CloseIO(io);
        }
        running_ = false;
    });
}

DiscResult DiscCheck::result() {
    std::lock_guard<std::mutex> lock(mutex_);
    return result_;
}
