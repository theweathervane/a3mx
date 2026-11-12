#include "shortcut.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>

static std::wstring widen(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    std::wstring w(n > 0 ? (size_t)n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    for (wchar_t &c : w) if (c == L'/') c = L'\\';
    return w;
}
static std::string narrow(const std::wstring &w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s(n > 0 ? (size_t)n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, NULL, NULL);
    return s;
}
bool shortcut_menu_available() { return true; }
bool shortcut_create(int where, const std::string &exe, const std::string &workdir, const std::string &, std::string &made, std::string &err) {
    PWSTR dir = NULL;
    if (FAILED(SHGetKnownFolderPath(where == SHORTCUT_MENU ? FOLDERID_Programs : FOLDERID_Desktop, 0, NULL, &dir)) || !dir) {
        err = where == SHORTCUT_MENU ? "Windows didn't say where the Start menu's folder is" : "Windows didn't say where the Desktop is";
        return false;
    }
    std::wstring link = std::wstring(dir) + L"\\A3MX Launcher.lnk";
    CoTaskMemFree(dir);
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IShellLinkW *sl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void **)&sl);
    if (SUCCEEDED(hr)) {
        std::wstring e = widen(exe), w = widen(workdir);
        while (!w.empty() && w.back() == L'\\') w.pop_back();
        sl->SetPath(e.c_str());
        sl->SetWorkingDirectory(w.c_str());
        sl->SetIconLocation(e.c_str(), 0);
        sl->SetDescription(L"A3MX: build and play from your own disc");
        IPersistFile *pf = NULL;
        hr = sl->QueryInterface(IID_IPersistFile, (void **)&pf);
        if (SUCCEEDED(hr)) { hr = pf->Save(link.c_str(), TRUE); pf->Release(); }
        sl->Release();
    }
    if (SUCCEEDED(init)) CoUninitialize();
    if (FAILED(hr)) { char b[80]; snprintf(b, sizeof b, "Windows refused (0x%08lX)", (unsigned long)hr); err = b; return false; }
    made = narrow(link);
    return true;
}

#elif defined(__APPLE__)
#include <unistd.h>
bool shortcut_menu_available() { return false; }
bool shortcut_create(int, const std::string &exe, const std::string &, const std::string &, std::string &made, std::string &err) {
    const char *home = getenv("HOME");
    if (!home) { err = "HOME is not set"; return false; }
    /* the app, if the launcher runs from one: .../A3MX Launcher.app/Contents/MacOS/a3mx-launcher */
    std::string target = exe;
    size_t app = target.find(".app/");
    if (app != std::string::npos) target.erase(app + 4);
    std::string link = std::string(home) + "/Desktop/A3MX Launcher" + (app != std::string::npos ? ".app" : "");
    unlink(link.c_str());
    if (symlink(target.c_str(), link.c_str()) != 0) { err = "could not make a link on the Desktop"; return false; }
    made = link;
    return true;
}

#else
#include <sys/stat.h>
bool shortcut_menu_available() { return true; }
bool shortcut_create(int where, const std::string &exe, const std::string &workdir, const std::string &icon, std::string &made, std::string &err) {
    const char *home = getenv("HOME");
    if (!home) { err = "HOME is not set"; return false; }
    std::string dir;
    if (where == SHORTCUT_MENU) {
        const char *x = getenv("XDG_DATA_HOME");
        dir = (x && *x == '/' ? std::string(x) : std::string(home) + "/.local/share") + "/applications";
    } else {
        char *d = SDL_GetUserFolder(SDL_FOLDER_DESKTOP) ? SDL_strdup(SDL_GetUserFolder(SDL_FOLDER_DESKTOP)) : NULL;
        dir = d ? d : std::string(home) + "/Desktop";
        SDL_free(d);
        while (dir.size() > 1 && dir.back() == '/') dir.pop_back();
    }
    SDL_CreateDirectory(dir.c_str());
    std::string path = dir + "/a3mx-launcher.desktop";
    std::string text = "[Desktop Entry]\nType=Application\nName=A3MX Launcher\nComment=Build and play from your own disc\n"
                       "Exec=\"" + exe + "\"\nPath=" + workdir + "\n" + (icon.empty() ? std::string() : "Icon=" + icon + "\n") +
                       "Terminal=false\nCategories=Game;\n";
    if (!SDL_SaveFile(path.c_str(), text.data(), text.size())) { err = std::string("could not write ") + path + ": " + SDL_GetError(); return false; }
    chmod(path.c_str(), 0755);        /* desktops only start a .desktop file that is executable */
    made = path;
    return true;
}
#endif
