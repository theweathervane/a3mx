/* The launcher's small pictures (icons.h). */
#include "icons.h"
#include <cmath>

static const float PI = 3.14159265f;

float flag_width(float h) { return std::floor(h * 1.5f); }

static void star(ImDrawList *dl, ImVec2 c, float r_out, ImU32 col) {
    ImVec2 p[10];
    for (int i = 0; i < 10; i++) {
        float a = -PI / 2 + i * PI / 5, r = (i & 1) ? r_out * 0.42f : r_out;
        p[i] = ImVec2(c.x + std::cos(a) * r, c.y + std::sin(a) * r);
    }
    for (int i = 0; i < 10; i++) dl->AddTriangleFilled(c, p[i], p[(i + 1) % 10], col);   /* a fan: the star is not convex */
}

/* Every flag fills the same box, FLAG_INSET inside the field-high square, with a white outline. */
static const float FLAG_INSET = 2.0f;

void draw_flag(ImDrawList *dl, ImVec2 at, float h, Region r) {
    float w = flag_width(h);
    ImVec2 pos(std::floor(at.x + FLAG_INSET), std::floor(at.y + FLAG_INSET));
    ImVec2 end(std::floor(at.x + w - FLAG_INSET), std::floor(at.y + h - FLAG_INSET));
    float fw = end.x - pos.x, fh = end.y - pos.y;
    dl->PushClipRect(pos, end, true);
    switch (r) {
    case REGION_US: {                                   /* 7 red and 6 white stripes, a canton with a few stars */
        for (int i = 0; i < 13; i++)
            dl->AddRectFilled(ImVec2(pos.x, pos.y + fh * i / 13.0f), ImVec2(end.x, pos.y + fh * (i + 1) / 13.0f),
                              (i & 1) ? IM_COL32(255, 255, 255, 255) : IM_COL32(178, 34, 52, 255));
        ImVec2 cend(pos.x + std::floor(fw * 0.42f), pos.y + std::floor(fh * 7 / 13.0f));
        dl->AddRectFilled(pos, cend, IM_COL32(60, 59, 110, 255));
        float cw = cend.x - pos.x, ch = cend.y - pos.y;
        for (int row = 0; row < 3; row++)               /* at this size, a hint of stars reads better than fifty */
            for (int col = 0; col < 4; col++)
                dl->AddCircleFilled(ImVec2(pos.x + cw * (col + 0.5f) / 4.0f, pos.y + ch * (row + 0.5f) / 3.0f),
                                    fh > 20 ? 1.1f : 0.8f, IM_COL32(255, 255, 255, 255), 6);
        break;
    }
    case REGION_JP:                                     /* the red disc on white */
        dl->AddRectFilled(pos, end, IM_COL32(255, 255, 255, 255));
        dl->AddCircleFilled(ImVec2(pos.x + fw / 2, pos.y + fh / 2), fh * 0.3f, IM_COL32(188, 0, 45, 255), 32);
        break;
    case REGION_EU: {                                   /* twelve gold stars in a circle on blue */
        dl->AddRectFilled(pos, end, IM_COL32(0, 51, 153, 255));
        ImVec2 c(pos.x + fw / 2, pos.y + fh / 2);
        for (int i = 0; i < 12; i++) {
            float a = i * PI / 6;
            star(dl, ImVec2(c.x + std::cos(a) * fh / 3, c.y + std::sin(a) * fh / 3), fh / 18 + 0.6f, IM_COL32(255, 204, 0, 255));
        }
        break;
    }
    default:
        dl->AddRectFilled(pos, end, IM_COL32(120, 120, 120, 255));
        break;
    }
    dl->PopClipRect();
    /* the outline: white, so every flag (the Japanese one included) has the same visible edge */
    dl->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(end.x + 1, end.y + 1), IM_COL32(255, 255, 255, 255), 0, 1.5f);
}

void draw_bad_mark(ImDrawList *dl, ImVec2 pos, float h) {
    float w = flag_width(h), s = h * 0.8f;
    ImVec2 a(pos.x + (w - s) / 2, pos.y + (h - s) / 2), b(a.x + s, a.y + s);
    dl->AddRectFilled(a, b, IM_COL32(200, 40, 40, 255), s * 0.2f);
    float m = s * 0.28f, t = s > 16 ? 2.5f : 1.8f;
    dl->AddLine(ImVec2(a.x + m, a.y + m), ImVec2(b.x - m, b.y - m), IM_COL32(255, 255, 255, 255), t);
    dl->AddLine(ImVec2(b.x - m, a.y + m), ImVec2(a.x + m, b.y - m), IM_COL32(255, 255, 255, 255), t);
}

void draw_wait_mark(ImDrawList *dl, ImVec2 pos, float h, float t, ImU32 col) {
    float w = flag_width(h);
    for (int i = 0; i < 3; i++) {
        float phase = std::fmod(t * 2.0f - i * 0.25f, 1.0f);
        float r = h * (0.07f + 0.05f * (phase < 0.5f ? phase * 2 : (1 - phase) * 2));
        dl->AddCircleFilled(ImVec2(pos.x + w * (0.25f + 0.25f * i), pos.y + h / 2), r, col, 12);
    }
}

void draw_bug(ImDrawList *dl, ImVec2 pos, float h, ImU32 body, ImU32 shade) {
    static const char *const BUG[] = {
        "..X.....X..",
        "...X...X...",
        "....XXX....",
        "...XXXXX...",
        "X.XXXoXXX.X",
        ".XXXXoXXXX.",
        "...XXoXX...",
        "XXXXXoXXXXX",
        "...XXoXX...",
        ".XXXXoXXXX.",
        "X..XXXXX..X",
    };
    float px = std::floor(h / 11.0f + 0.25f);
    if (px < 1) px = 1;
    ImVec2 o(std::floor(pos.x + (h - px * 11) / 2), std::floor(pos.y + (h - px * 11) / 2));
    for (int y = 0; y < 11; y++)
        for (int x = 0; x < 11; x++) {
            char c = BUG[y][x];
            if (c == '.') continue;
            ImU32 col = c == 'o' ? shade : body;
            dl->AddRectFilled(ImVec2(o.x + x * px, o.y + y * px), ImVec2(o.x + (x + 1) * px, o.y + (y + 1) * px), col);
        }
}

void draw_copy(ImDrawList *dl, ImVec2 pos, float h, ImU32 col) {
    /* the back sheet up and left, the front one down and right, as copy icons are */
    float u = std::floor(h / 8.0f) > 1 ? std::floor(h / 8.0f) : 1;
    float sw = u * 4, sh = u * 5;
    ImVec2 o(std::floor(pos.x + (h - sw - u * 1.5f) / 2), std::floor(pos.y + (h - sh - u * 1.5f) / 2));
    ImVec2 b0(o.x, o.y), b1(o.x + sw, o.y + sh);
    ImVec2 f0(o.x + u * 1.5f, o.y + u * 1.5f), f1(f0.x + sw, f0.y + sh);
    float t = h > 18 ? 1.5f : 1.2f;
    dl->AddRect(b0, b1, col, u * 0.4f, t);
    dl->AddRectFilled(ImVec2(f0.x - t, f0.y - t), ImVec2(f1.x + t, f1.y + t), ImGui::GetColorU32(ImGuiCol_WindowBg), u * 0.4f);
    dl->AddRect(f0, f1, col, u * 0.4f, t);
    for (int i = 1; i <= 3; i++) {                      /* lines of text on the front sheet */
        float y = f0.y + sh * i / 4.5f + 0.5f;
        dl->AddLine(ImVec2(f0.x + u, y), ImVec2(f1.x - u, y), col, 1.0f);
    }
}
