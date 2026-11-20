/* The launcher's small pictures, drawn with ImGui's draw list (no image files): the three discs'
 * flags, the "not recognised" mark, the copy icon and the tabs' pixel pictures. Each is drawn into the box at POS of height H. */
#pragma once
#include "imgui.h"
#include "disc_check.h"

float flag_width(float h);                                  /* 3:2; every flag the same size */
void draw_flag(ImDrawList *dl, ImVec2 pos, float h, Region r);
void draw_bad_mark(ImDrawList *dl, ImVec2 pos, float h);    /* a red square with a white cross */
void draw_wait_mark(ImDrawList *dl, ImVec2 pos, float h, float t, ImU32 col);   /* three dots, moving */
void draw_copy(ImDrawList *dl, ImVec2 pos, float h, ImU32 col);   /* two sheets of paper, the front one with lines */
/* the tabs' pictures, in the tabs' order: folder (Paths), chain links (Dependencies), controller (Inputs),
 * tick (Extensions), disc (Build / Run), i (About); X pixels in body, o pixels in shade */
enum { TAB_ICON_COUNT = 6 };
float tab_icon_pixel(float h);                              /* the size of one of its pixels in a box of height H */
void draw_tab_icon(ImDrawList *dl, ImVec2 pos, float h, int which, ImU32 body, ImU32 shade);
