// AUTO-GENERATED — DO NOT EDIT
#pragma once
#include "raylib.h"
#define internal static

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
        case SPRITE_ASTEROID1: { s.coords = (Rectangle){416, 0, 64, 64}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_ASTEROID2: { s.coords = (Rectangle){0, 0, 64, 64}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_ASTEROID3: { s.coords = (Rectangle){118, 0, 96, 96}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_BULLET: { s.coords = (Rectangle){597, 0, 2, 7}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_HEART: { s.coords = (Rectangle){481, 0, 17, 17}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_PLAYER: { s.coords = (Rectangle){219, 0, 190, 64}; s.pivotOffset = (Vector2){-1, -24}; s.numFrames = 5; break; }
        case SPRITE_STAR1: { s.coords = (Rectangle){410, 0, 5, 5}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_STAR2: { s.coords = (Rectangle){65, 0, 3, 3}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_UPGRADEDAMAGE: { s.coords = (Rectangle){548, 0, 48, 67}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_UPGRADEFIRERATE: { s.coords = (Rectangle){69, 0, 48, 67}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        case SPRITE_UPGRADEMULTISHOT: { s.coords = (Rectangle){499, 0, 48, 67}; s.pivotOffset = (Vector2){0, 0}; s.numFrames = 1; break; }
        default: break;
    }
    return s;
}
