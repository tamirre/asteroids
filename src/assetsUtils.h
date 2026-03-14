#pragma once
#include <stdlib.h>
#include "raylib.h"
#include "assetsData.h"

#define internal static
#define SHAPE_PADDING 1

typedef struct SpriteAnimation
{
    int framesPerSecond;
    float timeStarted;
    Rectangle* rectangles;
    int rectanglesLength;
} SpriteAnimation;

typedef struct TextureAtlas 
{
    Texture2D textureAtlas;
    Image imageAtlas;
    Color* pixelsAtlas;
	SpriteAnimation animations[ANIMATION_COUNT];
} TextureAtlas;

typedef struct SpriteMask {
    Color* pixels;
    int width;
    int height;
} SpriteMask;

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
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLength;
    return animation.rectangles[index];
}

int GetCurrentAnimationFrameIndex(SpriteAnimation animation) {
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLength;
    return index;
}

SpriteAnimation createSpriteAnimation(Texture2D atlas, SpriteID spriteID, int framesPerSecond, int numFrames)
{
	Sprite sprite = getSprite(spriteID);
	int x = sprite.coords.x;
	int y = sprite.coords.y;
	int width = sprite.coords.width;
	int height = sprite.coords.height;

    SpriteAnimation spriteAnimation = 
    {
        .framesPerSecond = framesPerSecond,
        .rectanglesLength = numFrames,
        .rectangles = NULL,
        .timeStarted = GetTime(),
    };

    Rectangle* mem = (Rectangle*)malloc(sizeof(Rectangle) * numFrames);
    if (mem == NULL)
    {
        TraceLog(LOG_FATAL, "No memory for CreateSpriteAnimation");
        spriteAnimation.rectanglesLength = 0;
        return spriteAnimation;
    }

    spriteAnimation.rectangles = mem;
    for(int i = 0; i < numFrames; i++)
    {

        spriteAnimation.rectangles[i] = (Rectangle){x+i*((width/numFrames)+SHAPE_PADDING)-SHAPE_PADDING*numFrames,y,width/numFrames,height};
		// printf("Rectangle %i: %f %f %f %f\n", i, spriteAnimation.rectangles[i].x, spriteAnimation.rectangles[i].y, spriteAnimation.rectangles[i].width, spriteAnimation.rectangles[i].height);
	}

    return spriteAnimation;
}

bool DrawSpriteAnimationOnce(Texture2D atlas,
                             SpriteAnimation animation,
                             Rectangle destination,
                             Vector2 origin,
                             float rotation,
                             Color tint,
                             Shader shader,
                             float startTime)
{
    int texSizeLoc = GetShaderLocation(shader, "textureSize");

    float elapsed = GetTime() - startTime;

    int frame = (int)(elapsed * animation.framesPerSecond);

    if (frame >= animation.rectanglesLength)
        return true; // animation finished

    Rectangle source = animation.rectangles[frame];

    Vector2 texSize = { source.width, source.height };
    SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);

    DrawTexturePro(atlas, source, destination, origin, rotation, tint);

    return false; // still playing
}

void DrawSpriteAnimationPro(Texture2D atlas, SpriteAnimation animation, Rectangle destination, Vector2 origin, float rotation, Color tint, Shader shader)
{
	int texSizeLoc = GetShaderLocation(shader, "textureSize");
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLength;
    Rectangle source = animation.rectangles[index];
	Vector2 texSize = { animation.rectangles->width, animation.rectangles->height };
	SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
    DrawTexturePro(atlas, source, destination, origin, rotation, tint);
}

void FreeSpriteAnimation(SpriteAnimation animation)
{
    free(animation.rectangles);
}

TextureAtlas initTextureAtlas(SpriteMask spriteMasks[])
{
    TextureAtlas atlas;
    atlas.textureAtlas = LoadTexture("assets/atlas/atlas.png");
	// SetTextureFilter(atlas.textureAtlas, TEXTURE_FILTER_POINT);
	// SetTextureFilter(atlas.textureAtlas, TEXTURE_FILTER_BILINEAR);
	// SetTextureFilter(atlas.textureAtlas, TEXTURE_FILTER_ANISOTROPIC_8X);

	Image atlasImage = LoadImageFromTexture(atlas.textureAtlas);
    ImageFormat(&atlasImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

	int animCount = 0;
	for (int i = 0; i < SPRITE_COUNT; i++)
	{
		if (getSprite(i).numFrames > 1)
		{
			atlas.animations[animCount] = createSpriteAnimation(atlas.textureAtlas, i, 7, getSprite(i).numFrames);
			// printf("Animation %i: %i frames\n", animCount, getSprite(SpriteToAnimation[i]).numFrames);
			animCount++;
		}
		spriteMasks[i].pixels = getPixelsFromAtlas(atlasImage, getSprite(i), getSprite(i).numFrames);
		spriteMasks[i].width = getSprite(i).coords.width / getSprite(i).numFrames;
		spriteMasks[i].height = getSprite(i).coords.height;
	}

    UnloadImage(atlasImage);
    return atlas;
}

#endif 
