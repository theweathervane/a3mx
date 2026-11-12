/* The project's version: shown in the bottom corner of OPTION MODE > EX. SETTING (exsetting.c).
 * It comes from the VERSION file at the top of the repository, which CMake passes in; a tag
 * v<VERSION> on main makes the release (.gitlab-ci.yml, docs/BUILDING.md). */
#pragma once
#ifndef A3MX_VERSION
#define A3MX_VERSION "v?"
#endif
/* as EX. SETTING draws it: capitals, digits and . / - only (V0.34.2; a nightly's +commit left off).
 * Everywhere else the version is lower case, as the tags are: v0.34.2. */
#ifndef A3MX_VERSION_SCREEN
#define A3MX_VERSION_SCREEN A3MX_VERSION
#endif
