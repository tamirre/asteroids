#pragma once
#define SHADOW_COLOR ((Color){255.0f, 255.0f, 255.0f, 50.0f})

#include "txt.h"
#include "raylib.h"
#include "raymath.h"
#include "localization.h"


static inline Color GetRainbowColor(float time)
{
    float speed = 2.0f; // animation speed

    float r = sinf(time * speed + 0.0f) * 0.5f + 0.5f;
    float g = sinf(time * speed + 2.094f) * 0.5f + 0.5f; // +120°
    float b = sinf(time * speed + 4.188f) * 0.5f + 0.5f; // +240°

    return (Color){
        (unsigned char)(r * 255),
        (unsigned char)(g * 255),
        (unsigned char)(b * 255),
        255
    };
}

static inline void DrawTextWave(Font font, const char* text, Vector2 center, float fontSize, Color color, bool rainbow, float time, float amplitude, float frequency, float phase, bool drawShadow)
{
    float spacing = GetDefaultSpacing(fontSize);

    // --- First pass: compute total width ---
    float totalWidth = 0.0f;

    int byteOffset = 0;
    int codepoint = 0;

    while (text[byteOffset] != '\0')
    {
        int next = 0;
        codepoint = GetCodepointNext(&text[byteOffset], &next);

        int glyphIndex = GetGlyphIndex(font, codepoint);
        float advance = (font.glyphs[glyphIndex].advanceX > 0)
            ? font.glyphs[glyphIndex].advanceX
            : font.recs[glyphIndex].width;

        totalWidth += advance * (fontSize / font.baseSize) + spacing;

        byteOffset += next;
    }

    float x = center.x - totalWidth / 2.0f;

    // --- Second pass: draw ---
    byteOffset = 0;
    int i = 0;

    while (text[byteOffset] != '\0')
    {
        int next = 0;
        codepoint = GetCodepointNext(&text[byteOffset], &next);

        int glyphIndex = GetGlyphIndex(font, codepoint);

        float advance = (font.glyphs[glyphIndex].advanceX > 0)
            ? font.glyphs[glyphIndex].advanceX
            : font.recs[glyphIndex].width;

        float charWidth = advance * (fontSize / font.baseSize);

		// float amplitude = 7.0f;
		// float frequency = 3.0f;
		float phaseOut = i * phase;
        float yOffset = amplitude * sinf(time * frequency + phaseOut);
		Color fontColor = color;
		if (rainbow) 
		{
			fontColor = GetRainbowColor(time + i * 0.3f);
		} 
		
		if (drawShadow)
		{
			// Draw shadow
			Vector2 shadowPos = (Vector2){x, center.y + yOffset};
			shadowPos.x += (int)(fontSize / 10);
			shadowPos.y += (int)(fontSize / 10);
			DrawTextCodepoint(font,
							  codepoint,
							  shadowPos,
							  fontSize,
							  SHADOW_COLOR);
		}

		// Draw codepoint
        DrawTextCodepoint(font,
                          codepoint,
                          (Vector2){x, center.y + yOffset},
                          fontSize,
                          fontColor);

        x += charWidth + spacing;

        byteOffset += next;
        i++;
    }
}

static inline void DrawTextCentered(Font font, const char* text, Vector2 pos, int fontSize, Color color)
{
	float fontSpacing = GetDefaultSpacing(fontSize);
	const Vector2 textSize = MeasureTextEx(font, text, fontSize, fontSpacing);
    pos.x -= textSize.x / 2.0f;
    pos.y -= textSize.y / 2.0f;

	Vector2 shadowPos = pos;
	shadowPos.x += (int)(fontSize / 10);
	shadowPos.y += (int)(fontSize / 10);
	DrawTextEx(font, text, shadowPos, fontSize, fontSpacing, SHADOW_COLOR);
	DrawTextEx(font, text, (Vector2){pos.x, pos.y}, fontSize, fontSpacing, color);
}

static inline void DrawTextWrapped(Font font,
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

			// Vector2 shadowPos = (Vector2){x, y};
			// shadowPos.x += (int)(fontSize / 10);
			// shadowPos.y += (int)(fontSize / 10);
			// Color shadowColor = (Color){130.0f, 130.0f, 130.0f, 50.0f};
			//          DrawTextPro(font, line,
			//                      shadowPos,
			//                      linePivot,
			//                      rotation,
			//                      fontSize,
			//                      spacing,
			//                      shadowColor);

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

