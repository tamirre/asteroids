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
    SpriteAnimation playerAnimation;
} TextureAtlas;

typedef enum SpriteID {
   SPRITE_PLAYER,
   SPRITE_STAR1,
   SPRITE_STAR2,
   SPRITE_BULLET,
   SPRITE_ASTEROID1,
   SPRITE_ASTEROID2,
   SPRITE_HEART,
   SPRITE_MULTISHOT_UPGRADE,
   SPRITE_DAMAGE_UPGRADE,
   SPRITE_FIRERATE_UPGRADE,

   SPRITE_COUNT,
} SpriteID;

typedef struct Sprite {
    Rectangle coords;
} Sprite;

internal Sprite getSprite(SpriteID spriteID)
{
    Sprite s = {};
    switch(spriteID)
    {
        case SPRITE_PLAYER:           {s.coords = (Rectangle){31,  0, 635, 62}; break;}
        case SPRITE_STAR1:            {s.coords = (Rectangle){0,  69,   5,  5}; break;}
        case SPRITE_STAR2:            {s.coords = (Rectangle){5,  69,   3,  3}; break;}
        case SPRITE_BULLET:           {s.coords = (Rectangle){0,  52,   2,  7}; break;}
        // case SPRITE_ASTEROID1:        {s.coords = (Rectangle){0,   0,  31, 28}; break;}
        // case SPRITE_ASTEROID2:        {s.coords = (Rectangle){0,  28,  27, 23}; break;}
        case SPRITE_ASTEROID1:        {s.coords = (Rectangle){192,   74,  50, 50}; break;}
        case SPRITE_ASTEROID2:        {s.coords = (Rectangle){242,   74,  50, 50}; break;}
        case SPRITE_HEART:            {s.coords = (Rectangle){2,  52,  17, 17}; break;}
        case SPRITE_MULTISHOT_UPGRADE:{s.coords = (Rectangle){128,74,  64, 80}; break;}
        case SPRITE_DAMAGE_UPGRADE:   {s.coords = (Rectangle){0,  74,  64, 80}; break;}
        case SPRITE_FIRERATE_UPGRADE: {s.coords = (Rectangle){64, 74,  64, 80}; break;}
        default: { } 
    }
    return s;
}
#define ASSETS_IMPLEMENTATION
#ifdef ASSETS_IMPLEMENTATION

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

void DrawSpriteAnimationPro(SpriteAnimation animation, Rectangle destination, Vector2 origin, float rotation, Color tint)
{
    int index = (int)(GetTime() * animation.framesPerSecond) % animation.rectanglesLentgh;
    Rectangle source = animation.rectangles[index];
    DrawTexturePro(animation.atlas, source, destination, origin, rotation, tint);
}

void FreeSpriteAnimation(SpriteAnimation animation)
{
    free(animation.rectangles);
}

TextureAtlas initTextureAtlas()
{
    TextureAtlas atlas;
    atlas.textureAtlas = LoadTexture("assets/textureAtlas.png");
    Texture2D playerTexture = LoadTexture("assets/rocketNew.png");
    atlas.playerAnimation = createSpriteAnimation(playerTexture, 10, (Rectangle[]) {
            (Rectangle){0,0,32,62},   (Rectangle){32,0,32,62},  (Rectangle){64,0,32,62},  (Rectangle){96,0,32,62},
            (Rectangle){128,0,32,62}, (Rectangle){160,0,32,62}, (Rectangle){192,0,32,62}, (Rectangle){224,0,32,62},
            (Rectangle){256,0,32,62}, (Rectangle){288,0,32,62}, (Rectangle){320,0,32,62}, (Rectangle){352,0,32,62},
            (Rectangle){384,0,32,62}, (Rectangle){416,0,32,62}, (Rectangle){448,0,32,62}, (Rectangle){480,0,32,62},
            (Rectangle){512,0,32,62}, (Rectangle){544,0,32,62}, (Rectangle){576,0,32,62}, (Rectangle){608,0,32,62},        
            }, 20);
    return atlas;
}

#endif 
