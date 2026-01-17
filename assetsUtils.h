#pragma once
#include <stdlib.h>
#include "raylib.h"

#define internal static

typedef struct SpriteAnimation
{
    Texture2D atlas;
    int framesPerSecond;
    float timeStarted;
    Rectangle* rectangles;
    int rectanglesLentgh;
} SpriteAnimation;

typedef struct TextureAtlas 
{
    Texture2D textureAtlas;
    Image imageAtlas;
    Color* pixelsAtlas;
    SpriteAnimation playerAnimation;
} TextureAtlas;

typedef struct Sprite {
    Texture2D texture;
    Rectangle coords;
	Vector2 pivotOffset;
	int numFrames;
} Sprite;

typedef struct SpriteMask {
    Color* pixels;
    int width;
    int height;
} SpriteMask;

typedef struct SpriteMaskCache {
    SpriteMask player;
    SpriteMask asteroid1;
    SpriteMask asteroid2;
    SpriteMask asteroid3;
	SpriteMask bullet;
} SpriteMaskCache;


#define ASSETS_IMPLEMENTATION
#ifdef ASSETS_IMPLEMENTATION

internal Color* getPixelsFromAtlas(Image atlasImage, Sprite sprite, int numberOfFrames)
{
	// Extract the sprite rectangle as an Image from the atlas image
	Rectangle src = sprite.coords;
	src.width = src.width / numberOfFrames;
	Image img = ImageFromImage(atlasImage, src);
	// Ensure uncompressed RGBA8 format
	ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
	// Allocate pixel data (must be freed with UnloadImageColors)
	Color *pixels = LoadImageColors(img);
	// Free temporary Image (LoadImageColors copies pixel data)
	UnloadImage(img);
	return pixels;
}

Rectangle GetCurrentAnimationFrame(SpriteAnimation animation) {
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLentgh;
    return animation.rectangles[index];
}

int GetCurrentAnimationFrameIndex(SpriteAnimation animation) {
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLentgh;
    return index;
}

SpriteAnimation createSpriteAnimation(Texture2D atlas, int framesPerSecond, Rectangle rectangles[], int length)
{
    SpriteAnimation spriteAnimation = 
    {
        .atlas = atlas,
        .framesPerSecond = framesPerSecond,
        .rectanglesLentgh = length,
        .rectangles = NULL,
        .timeStarted = GetTime(),
    };

    Rectangle* mem = (Rectangle*)malloc(sizeof(Rectangle) * length);
    if (mem == NULL)
    {
        TraceLog(LOG_FATAL, "No memory for CreateSpriteAnimation");
        spriteAnimation.rectanglesLentgh = 0;
        return spriteAnimation;
    }

    spriteAnimation.rectangles = mem;
    for(int i = 0; i < length; i++)
    {
        spriteAnimation.rectangles[i] = rectangles[i];
    }

    return spriteAnimation;
}

void DrawSpriteAnimationPro(SpriteAnimation animation, Rectangle destination, Vector2 origin, float rotation, Color tint, Shader shader)
{
	int texSizeLoc = GetShaderLocation(shader, "textureSize");
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLentgh;
    Rectangle source = animation.rectangles[index];
	Vector2 texSize = { animation.rectangles->width, animation.rectangles->height };
	SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
    DrawTexturePro(animation.atlas, source, destination, origin, rotation, tint);
}

void FreeSpriteAnimation(SpriteAnimation animation)
{
    free(animation.rectangles);
}


#endif 
