/* A shortcut to the launcher, made only when the player asks (the Paths tab's button): nothing makes
 * one by itself since 0.60.0. Windows: a .lnk on the Desktop or in the Start menu. Linux: a .desktop
 * file on the Desktop or in the applications menu. macOS: a link on the Desktop (the Dock and
 * Launchpad are the player's to arrange). */
#pragma once
#include <string>
enum { SHORTCUT_DESKTOP, SHORTCUT_MENU };
/* EXE: the launcher (on macOS, the .app if it is one). ICON: an image for it, or "" (Linux).
 * made: the file written. false with err on failure. */
bool shortcut_create(int where, const std::string &exe, const std::string &workdir, const std::string &icon, std::string &made, std::string &err);
bool shortcut_menu_available();     /* macOS has no menu to put one in */
