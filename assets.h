#ifndef ASSETS_H
#define ASSETS_H

#include <stdlib.h>
#include "raylib.h"

typedef struct SpriteAnimation
{
    Texture2D atlas;
    int framesPerSecond;
    float timeStarted;
    Rectangle* rectangles;
    int rectanglesLentgh;
} SpriteAnimation;

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

#define ASSETS_IMPLEMENTATION
#ifdef ASSETS_IMPLEMENTATION

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

#endif // ASSETS_IMPLEMENTATION
#endif // ASSETS_H
