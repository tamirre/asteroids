// AUTO-GENERATED — DO NOT EDIT
#pragma once
#include "raylib.h"
#define internal static
s
typedef struct Sprite {
    Texture2D texture;
    Rectangle coords;
    Vector2 pivotOffset;
    int numFrames;
} Sprite;

typedef enum SpriteID {
    SPRITE_ASTEROID1,
    SPRITE_ASTEROID2,
    SPRITE_ASTEROID3,
    SPRITE_BULLET,
    SPRITE_HEART,
    SPRITE_PLAYER,
    SPRITE_STAR1,
    SPRITE_STAR2,
    SPRITE_UPGRADEDAMAGE,
    SPRITE_UPGRADEFIRERATE,
    SPRITE_UPGRADEMULTISHOT,
    SPRITE_COUNT
} SpriteID;

static inline Sprite getSprite(SpriteID spriteID) {
    Sprite s = {0};
    switch(spriteID) {
        case SPRITE_ASTEROID1: { s.coords = (Rectangle){422, 0, 64, 64}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_ASTEROID2: { s.coords = (Rectangle){0, 0, 64, 64}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_ASTEROID3: { s.coords = (Rectangle){131, 0, 96, 96}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_BULLET: { s.coords = (Rectangle){631, 0, 2, 7}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_HEART: { s.coords = (Rectangle){486, 0, 17, 17}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_PLAYER: { s.coords = (Rectangle){227, 0, 190, 64}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_STAR1: { s.coords = (Rectangle){417, 0, 5, 5}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_STAR2: { s.coords = (Rectangle){64, 0, 3, 3}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_UPGRADEDAMAGE: { s.coords = (Rectangle){567, 0, 64, 80}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_UPGRADEFIRERATE: { s.coords = (Rectangle){67, 0, 64, 80}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_UPGRADEMULTISHOT: { s.coords = (Rectangle){503, 0, 64, 80}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        default: break;
    }
    return s;
}
