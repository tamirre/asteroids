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


static inline void CollectCodepointsFromText(const char *text,
											 int *out,
											 int *count,
											 int max)
{
    int len = 0;
    int *cps = LoadCodepoints(text, &len);

    for (int i = 0; i < len; i++)
    {
        int cp = cps[i];

        // check if already added
        int exists = 0;
        for (int j = 0; j < *count; j++)
        {
            if (out[j] == cp)
            {
                exists = 1;
                break;
            }
        }

		if (*count >= max)
			TraceLog(LOG_ERROR, "CollectCodepointsFromText: Too many codepoints");
        if (!exists)
            out[(*count)++] = cp;
    }

    UnloadCodepoints(cps);
}

static inline Font LoadLanguageFont(const char *path,
									int size,
									int lang)
{
    int maxGlyphs = 2000;
    int *glyphs = malloc(sizeof(int) * maxGlyphs);
    int count = 0;

    // Always include basic ASCII so formatting works
    for (int cp = 32; cp <= 126; cp++)
        glyphs[count++] = cp;

    // Scan all texts of this language
    for (int i = 0; i < TXT_COUNT; i++)
    {
        const char *s = gText[lang][i];
        if (!s || !*s) continue;

        CollectCodepointsFromText(s, glyphs, &count, maxGlyphs);
    }

    TraceLog(LOG_INFO, "Font will contain %d glyphs", count);

    Font f = LoadFontEx(path, size, glyphs, count);

    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);

    free(glyphs);
    return f;
}


