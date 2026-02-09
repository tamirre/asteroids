#pragma once
#include "raylib.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// -----------------------------------------------------------------------------
// This file expects a generated "txt.h" with:
//
//   typedef enum { TXT_..., TXT_COUNT } TextID;
//   typedef enum { LANG_EN, LANG_DE, LANG_ZH, LANG_COUNT } Language;
//   static const char *gText[LANG_COUNT][TXT_COUNT];
//
// -----------------------------------------------------------------------------
#include "txt.h"

static Language s_locLang = LANG_EN;

static inline void LocSetLanguage(Language lang)
{
    if (lang >= 0 && lang < LANG_COUNT)
        s_locLang = lang;
}

static inline Language LocGetLanguage(void)
{
    return s_locLang;
}

static inline float GetDefaultSpacing(float size)
{
    switch(s_locLang)
    {
        case LANG_ZH: return size * 0.05f;
        default:      return size * 0.025f;
    }
}

static inline const char *T(TextID id)
{
    if (id < 0 || id >= TXT_COUNT)
        return "INVALID_TEXT_ID";

    return gText[s_locLang][id];
}

// ======================= FORMATTING (%d %f %s) ===============================

static inline const char *TF(TextID id, ...)
{
    static char buffer[2048];

    va_list args;
    va_start(args, id);
    vsnprintf(buffer, sizeof(buffer), T(id), args);
    va_end(args);

    return buffer;
}

// ======================= UTF-8 WORD WRAP =====================================

static inline const char *TWrap(Font font,
                                const char *text,
                                float maxWidth,
                                float fontSize,
                                float spacing)
{
    static char out[4096];
    out[0] = 0;

    float lineW = 0.0f;

    int cpSize = 0;
    char utf8[5] = {0};

    for (int i = 0; text[i]; i += cpSize)
    {
        GetCodepoint(&text[i], &cpSize);
        memcpy(utf8, &text[i], cpSize);
        utf8[cpSize] = 0;

        float w = MeasureTextEx(font, utf8, fontSize, spacing).x;

        if (lineW + w > maxWidth && lineW > 0)
        {
            strcat(out, "\n");
            lineW = 0;
        }

        strcat(out, utf8);
        lineW += w;
    }

    return out;
}

// ======================= FORMAT + WRAP =======================================

static inline const char *TFWrap(Font font,
                                 float maxWidth,
                                 float fontSize,
                                 float spacing,
                                 TextID id, ...)
{
    static char fmtBuf[2048];

    va_list args;
    va_start(args, id);
    vsnprintf(fmtBuf, sizeof(fmtBuf), T(id), args);
    va_end(args);

    return TWrap(font, fmtBuf, maxWidth, fontSize, spacing);
}

static Font LoadChineseFont(const char *path, int size)
{
    // // allocate dynamically
    // int *glyphs = malloc(sizeof(int) * 22000);
    // int count = 0;
    //
    // // ASCII
    // for (int cp = 32; cp <= 126; cp++)
    //     glyphs[count++] = cp;
    //
    // // CJK
    // for (int cp = 0x4E00; cp <= 0x9FFF; cp++)
    //     glyphs[count++] = cp;
    //
    // Font f = LoadFontEx(path, size, glyphs, count);
    //
    // SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
    //
    // free(glyphs);   // safe after LoadFontEx
    // return f;
	int glyphs[] = {
        0x4F60, // 你
        0x597D, // 好
        0x4E16, // 世
        0x754C, // 界
        32,33,44,46 // space ! , .
    };

    Font f = LoadFontEx(path, size, glyphs,
                        sizeof(glyphs)/sizeof(int));

    SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
    return f;
}

// ======================= GERMAN FONT LOADER ==================================
// Correct glyph loading for äöüß
// -----------------------------------------------------------------------------
static Font LoadGermanFont(const char *path, int size)
{
    int glyphs[512];
    int count = 0;

    // --- 1) FULL ASCII BLOCK (guarantees .,!?: etc) ---
    for (int cp = 32; cp <= 126; cp++)
        glyphs[count++] = cp;

    // --- 2) Latin-1 supplement for umlauts ---
    for (int cp = 160; cp <= 255; cp++)
        glyphs[count++] = cp;

    // --- 3) German typography extras ---
    int extras[] = {
        0x201E,0x201C,0x201D, // „ “ ”
        0x2013,0x2014         // – —
    };

    for (int i=0;i<5;i++)
        glyphs[count++] = extras[i];

    Font f = LoadFontEx(path, size, glyphs, count);

    if (f.texture.id == 0)
        TraceLog(LOG_ERROR, "LoadGermanFont FAILED: %s", path);

    // SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);   

    return f;
}

// ======================= GENERIC DRAW HELPERS ================================

static inline void DrawTextT(Font font, TextID id,
                             Vector2 pos, float size,
                             float spacing, Color col)
{
    DrawTextEx(font, T(id), pos, size, spacing, col);
}

static inline void DrawTextTF(Font font, TextID id,
                              Vector2 pos, float size,
                              float spacing, Color col, ...)
{
    va_list args;
    va_start(args, col);

    char buf[1024];
    vsnprintf(buf, sizeof(buf), T(id), args);

    va_end(args);

    DrawTextEx(font, buf, pos, size, spacing, col);
}

