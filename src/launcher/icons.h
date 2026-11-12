/* The launcher's small pictures, drawn with ImGui's draw list (no image files): the three discs'
 * flags, the "not recognised" mark, the grey pixel bug on the report button, and the copy icon. Each is drawn into the box at POS of height H. */
#pragma once
#include "imgui.h"
#include "disc_check.h"

float flag_width(float h);                                  /* 3:2; every flag the same size */
void draw_flag(ImDrawList *dl, ImVec2 pos, float h, Region r);
void draw_bad_mark(ImDrawList *dl, ImVec2 pos, float h);    /* a red square with a white cross */
void draw_wait_mark(ImDrawList *dl, ImVec2 pos, float h, float t, ImU32 col);   /* three dots, moving */
void draw_bug(ImDrawList *dl, ImVec2 pos, float h, ImU32 body, ImU32 shade);   /* 11 x 11 pixels: body, and its middle line */
void draw_copy(ImDrawList *dl, ImVec2 pos, float h, ImU32 col);   /* two sheets of paper, the front one with lines */
