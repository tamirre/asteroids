#pragma once
#include "assetsUtils.h"

#define internal static

typedef enum SpriteID {
   SPRITE_PLAYER,
   SPRITE_STAR1,
   SPRITE_STAR2,
   SPRITE_BULLET,
   SPRITE_ASTEROID1,
   SPRITE_ASTEROID2,
   SPRITE_ASTEROID3,
   SPRITE_HEART,
   SPRITE_MULTISHOT_UPGRADE,
   SPRITE_DAMAGE_UPGRADE,
   SPRITE_FIRERATE_UPGRADE,

   SPRITE_COUNT,
} SpriteID;

internal Sprite getSprite(SpriteID spriteID)
{
	Sprite s = {};
	switch(spriteID)
	{
		case SPRITE_PLAYER:           {s.coords = (Rectangle){292, 74, 190, 64}; break;}
		case SPRITE_STAR1:            {s.coords = (Rectangle){0,   69,   5,  5}; break;}
		case SPRITE_STAR2:            {s.coords = (Rectangle){5,   69,   3,  3}; break;}
		case SPRITE_BULLET:           {s.coords = (Rectangle){0,   52,   2,  7}; break;}
		case SPRITE_ASTEROID1:        {s.coords = (Rectangle){160, 160,  64, 64}; break;}
		case SPRITE_ASTEROID2:        {s.coords = (Rectangle){96,  160,  64, 64}; break;}
		case SPRITE_ASTEROID3:        {s.coords = (Rectangle){0,   160,  96, 96}; break;}
		case SPRITE_HEART:            {s.coords = (Rectangle){2,   52,   17, 17}; break;}
		case SPRITE_MULTISHOT_UPGRADE:{s.coords = (Rectangle){128, 74,   64, 80}; break;}
		case SPRITE_DAMAGE_UPGRADE:   {s.coords = (Rectangle){0,   74,   64, 80}; break;}
		case SPRITE_FIRERATE_UPGRADE: {s.coords = (Rectangle){64,  74,   64, 80}; break;}
		default: { } 
	}
	return s;
}

TextureAtlas initTextureAtlas(SpriteMaskCache* spriteMasks)
{

    TextureAtlas atlas;
    atlas.textureAtlas = LoadTexture("assets/textureAtlas.png");
	SetTextureFilter(atlas.textureAtlas, TEXTURE_FILTER_BILINEAR);

	Image atlasImage = LoadImageFromTexture(atlas.textureAtlas);
    ImageFormat(&atlasImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    float textureWidth = 38;
    float textureHeight = 64;
    atlas.playerAnimation = createSpriteAnimation(atlas.textureAtlas, 7, (Rectangle[]) {
                        (Rectangle){292,74,textureWidth,textureHeight},   
                        (Rectangle){292+1*textureWidth,74,textureWidth,textureHeight},  
                        (Rectangle){292+2*textureWidth,74,textureWidth,textureHeight},  
                        (Rectangle){292+3*textureWidth,74,textureWidth,textureHeight},
                        (Rectangle){292+4*textureWidth,74,textureWidth,textureHeight},
                        }, 5);

	spriteMasks->player.pixels = getPixelsFromAtlas(atlasImage, getSprite(SPRITE_PLAYER), 5);
	spriteMasks->player.width = getSprite(SPRITE_PLAYER).coords.width / 5;
	spriteMasks->player.height = getSprite(SPRITE_PLAYER).coords.height;

	spriteMasks->bullet.pixels = getPixelsFromAtlas(atlasImage, getSprite(SPRITE_BULLET), 1);
	spriteMasks->bullet.width = getSprite(SPRITE_BULLET).coords.width;
	spriteMasks->bullet.height = getSprite(SPRITE_BULLET).coords.height;

	spriteMasks->asteroid1.pixels = getPixelsFromAtlas(atlasImage, getSprite(SPRITE_ASTEROID1), 1);
	spriteMasks->asteroid1.width = getSprite(SPRITE_ASTEROID1).coords.width;
	spriteMasks->asteroid1.height = getSprite(SPRITE_ASTEROID1).coords.height;

	spriteMasks->asteroid2.pixels = getPixelsFromAtlas(atlasImage, getSprite(SPRITE_ASTEROID2), 1);
	spriteMasks->asteroid2.width = getSprite(SPRITE_ASTEROID2).coords.width;
	spriteMasks->asteroid2.height = getSprite(SPRITE_ASTEROID2).coords.height;

	spriteMasks->asteroid3.pixels = getPixelsFromAtlas(atlasImage, getSprite(SPRITE_ASTEROID3), 1);
	spriteMasks->asteroid3.width = getSprite(SPRITE_ASTEROID3).coords.width;
	spriteMasks->asteroid3.height = getSprite(SPRITE_ASTEROID3).coords.height;

    UnloadImage(atlasImage);

    return atlas;
}
