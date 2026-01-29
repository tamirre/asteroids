#pragma once
#include <stdlib.h>
#include "raylib.h"
#include "assetsData.h"

#define internal static
#define SHAPE_PADDING 1

typedef struct SpriteAnimation
{
    Texture2D atlas;
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
    SpriteAnimation playerAnimation;
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
        .atlas = atlas,
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

void DrawSpriteAnimationPro(SpriteAnimation animation, Rectangle destination, Vector2 origin, float rotation, Color tint, Shader shader)
{
	int texSizeLoc = GetShaderLocation(shader, "textureSize");
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLength;
    Rectangle source = animation.rectangles[index];
	Vector2 texSize = { animation.rectangles->width, animation.rectangles->height };
	SetShaderValue(shader, texSizeLoc, &texSize, SHADER_UNIFORM_IVEC2);
    DrawTexturePro(animation.atlas, source, destination, origin, rotation, tint);
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

	// int animCount = 0;
	// for (int i = 0; i < SPRITE_COUNT; i++)
	// {
	// 	if (getSprite(i).numFrames > 1) animCount++;
	// 	// fprintf(stdout, "Sprite %d has %d frames\n", i, getSprite(i).numFrames);
	// }

	// for (int i = 0; i < animCount; i++) {
	//
	// }

    atlas.playerAnimation = createSpriteAnimation(atlas.textureAtlas, SPRITE_PLAYER, 7, getSprite(SPRITE_PLAYER).numFrames);
	// printf("Player Animation: %d fps\n", atlas.playerAnimation.framesPerSecond);
	// printf("Player Animation: %d number of frames\n", atlas.playerAnimation.rectanglesLength );
	// for(int i = 0; i < atlas.playerAnimation.rectanglesLength; i++)
	// {
	// 	printf("Rectangle %i: %f %f %f %f\n", i, atlas.playerAnimation.rectangles[i].x, atlas.playerAnimation.rectangles[i].y, atlas.playerAnimation.rectangles[i].width, atlas.playerAnimation.rectangles[i].height);
	// }

	// SpriteMask spriteMasks2[SPRITE_COUNT];
	for (int i = 0; i < SPRITE_COUNT; i++)
	{
		spriteMasks[i].pixels = getPixelsFromAtlas(atlasImage, getSprite(i), getSprite(i).numFrames);
		spriteMasks[i].width = getSprite(i).coords.width / getSprite(i).numFrames;
		spriteMasks[i].height = getSprite(i).coords.height;
	}

    UnloadImage(atlasImage);

    return atlas;
}

#endif 
