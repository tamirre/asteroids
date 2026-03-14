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
//   typedef enum { LANG_EN, LANG_DE, LANG_ZH, ...,  LANG_COUNT } Language;
//   static const char *gText[LANG_COUNT][TXT_COUNT];
//
// -----------------------------------------------------------------------------
#include "txt.h"

typedef enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER
} TextAlign;

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

static inline const char* T(TextID id)
{
    if (id < 0 || id >= TXT_COUNT)
        return "INVALID_TEXT_ID";

    return gText[s_locLang][id];
}

// ======================= FORMATTING (%d %f %s) ===============================

static inline const char* TF(TextID id, ...)
{
    static char buffer[2048];

    va_list args;
    va_start(args, id);
    vsnprintf(buffer, sizeof(buffer), T(id), args);
    va_end(args);

    return buffer;
}

// ======================= UTF-8 WORD WRAP =====================================

static void flushWord(char *out, int capacity,
                      int *outLen,
                      char *word, int *wordLen,
                      float *lineW,
                      float maxWidth,
                      Font font,
                      float fontSize,
                      float spacing)
{
    if (*wordLen == 0)
        return;

    float w = MeasureTextEx(font, word, fontSize, spacing).x;

    // Need line break?
    if (*lineW + w > maxWidth && *lineW > 0)
    {
        if (*outLen + 1 < capacity)
        {
            out[*outLen] = '\n';
            (*outLen)++;
            out[*outLen] = 0;
        }
        *lineW = 0;
    }

    // Append word to output
    if (*outLen + *wordLen < capacity)
    {
        memcpy(out + *outLen, word, *wordLen);
        *outLen += *wordLen;
        out[*outLen] = 0;
    }

    *lineW += w;

    // reset word buffer
    *wordLen = 0;
    word[0] = 0;
}

// ------------------------------------------------------------

int TWrap(char *out, int capacity,
          Font font,
          const char *text,
          float maxWidth,
          float fontSize,
          float spacing)
{
    if (!out || capacity <= 0)
        return 0;

    out[0] = 0;
    int outLen = 0;

    float lineW = 0.0f;

    char word[256];
    int wordLen = 0;

    int cpSize = 0;
    char utf8[5] = {0};

    for (int i = 0; text[i]; i += cpSize)
    {
        GetCodepoint(&text[i], &cpSize);

        memcpy(utf8, &text[i], cpSize);
        utf8[cpSize] = 0;

        int isSpace   = (cpSize == 1 && utf8[0] == ' ');
        int isNewline = (cpSize == 1 && utf8[0] == '\n');

        if (isSpace || isNewline)
        {
            flushWord(out, capacity, &outLen,
                      word, &wordLen,
                      &lineW, maxWidth,
                      font, fontSize, spacing);

            // copy the space itself
            if (!isNewline)
            {
                if (outLen + 1 < capacity)
                {
                    out[outLen++] = ' ';
                    out[outLen] = 0;

                    lineW += MeasureTextEx(font, " ", fontSize, spacing).x;
				}
            }

            if (isNewline)
            {
                if (outLen + 1 < capacity)
                {
                    out[outLen++] = '\n';
                    out[outLen] = 0;
                }
                lineW = 0;
            }

            continue;
        }

        // accumulate into current word
        if (wordLen + cpSize < (int)sizeof(word))
        {
            memcpy(word + wordLen, utf8, cpSize);
            wordLen += cpSize;
            word[wordLen] = 0;
        }
        else
        {
            // extremely long word → flush defensively
            flushWord(out, capacity, &outLen,
                      word, &wordLen,
                      &lineW, maxWidth,
                      font, fontSize, spacing);
        }
    }

    // flush last word
    flushWord(out, capacity, &outLen,
              word, &wordLen,
              &lineW, maxWidth,
              font, fontSize, spacing);

    return outLen;
}

// ======================= FORMAT + WRAP =======================================

static inline const char* TFWrap(char *out, int capacity,
								 Font font,
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

	TWrap(out, capacity, font, fmtBuf, maxWidth, fontSize, spacing);
    return out;
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

void DrawTextWrapped(Font font,
                     const char *text,
                     char *wrapped,
                     int capacity,
                     Vector2 pos,
                     float maxWidth,
                     float fontSize,
                     TextAlign align,
                     float rotation,
					 float scaling,
                     Vector2 pivot,
                     Color color)
{
	fontSize *= (1.0f + scaling);
    float spacing = GetDefaultSpacing(fontSize);
    TWrap(wrapped, capacity, font, text, maxWidth, fontSize, spacing);

    float yOffset = 0.0f;

    const char *lineStart = wrapped;
    const char *p = wrapped;

    while (1)
    {
        if (*p == '\n' || *p == '\0')
        {
            int len = p - lineStart;
            if (len >= 512) len = 511;

            char line[512];
            memcpy(line, lineStart, len);
            line[len] = 0;

            float w = MeasureTextEx(font, line, fontSize, spacing).x;

            float x = pos.x;
            if (align == ALIGN_RIGHT) x = pos.x + (maxWidth - w);
            else if (align == ALIGN_CENTER) x = pos.x + (maxWidth - w) / 2.0f;

            float y = pos.y;

            // derive pivot for this line without mutating original pivot
            Vector2 linePivot = {
                pivot.x,
                pivot.y - yOffset
            };

            DrawTextPro(font, line,
                        (Vector2){ x, y },
                        linePivot,
                        rotation,
                        fontSize,
                        spacing,
                        color);

            if (*p == '\0')
                break;

            yOffset += fontSize;
            lineStart = p + 1;
        }
        p++;
    }
}

