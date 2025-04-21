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

typedef enum SpriteID {
   SPRITE_PLAYER_ANIMATION,
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

typedef struct TextureAtlas {
    SpriteAnimation playerAnimation;
    Texture2D playerTexture;
    Texture2D starTexture1;
    Texture2D starTexture2;
    Texture2D bulletTexture;
    Texture2D enemyTexture1;
    Texture2D enemyTexture2;
    Texture2D healthTexture;
    Texture2D upgradeMultishotTexture;
    Texture2D upgradeFirerateTexture;
    Texture2D upgradeDamageTexture;
} TextureAtlas;

typedef struct Sprite {
    Vector4 coords;
} Sprite;

internal Sprite getSprite(SpriteID spriteID)
{
    Sprite s = {};
    switch(spriteID)
    {
        case SPRITE_PLAYER_ANIMATION: { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_PLAYER:           { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_STAR1:            { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_STAR2:            { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_BULLET:           { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_ASTEROID1:        { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_ASTEROID2:        { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_HEART:            { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_MULTISHOT_UPGRADE:{ s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_DAMAGE_UPGRADE:   { s.coords = (Vector4){0,0,0,0}; break;}
        case SPRITE_FIRERATE_UPGRADE: { s.coords = (Vector4){0,0,0,0}; break;}
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

void initTextureAtlas(TextureAtlas* atlas)
{
    atlas->playerTexture = LoadTexture("assets/rocketNew.png");
    atlas->playerAnimation = createSpriteAnimation(atlas->playerTexture, 10, (Rectangle[]) {
        (Rectangle){0,0,32,62},   (Rectangle){32,0,32,62},  (Rectangle){64,0,32,62},  (Rectangle){96,0,32,62},
        (Rectangle){128,0,32,62}, (Rectangle){160,0,32,62}, (Rectangle){192,0,32,62}, (Rectangle){224,0,32,62},
        (Rectangle){256,0,32,62}, (Rectangle){288,0,32,62}, (Rectangle){320,0,32,62}, (Rectangle){352,0,32,62},
        (Rectangle){384,0,32,62}, (Rectangle){416,0,32,62}, (Rectangle){448,0,32,62}, (Rectangle){480,0,32,62},
        (Rectangle){512,0,32,62}, (Rectangle){544,0,32,62}, (Rectangle){576,0,32,62}, (Rectangle){608,0,32,62},        
    }, 20);
    atlas->starTexture1 = LoadTexture("assets/star1.png");
    atlas->starTexture2 = LoadTexture("assets/star2.png");
    atlas->bulletTexture = LoadTexture("assets/bullet.png");
    atlas->enemyTexture1 = LoadTexture("assets/asteroid1.png");
    atlas->enemyTexture2 = LoadTexture("assets/asteroid2.png");
    atlas->healthTexture = LoadTexture("assets/heart.png");
    atlas->upgradeMultishotTexture = LoadTexture("assets/upgradeMultiShot.png");
    atlas->upgradeDamageTexture = LoadTexture("assets/upgradeDamage.png");
    atlas->upgradeFirerateTexture = LoadTexture("assets/upgradeFireRate.png");
}

#endif 
