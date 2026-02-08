#pragma once
// AUTO GENERATED - DO NOT EDIT
// Generated from: txt.csv

typedef enum {
    TXT_EMPTY,
    TXT_PLAY,
    TXT_INSTRUCTIONS,
    TXT_SCORE,
    TXT_GAME_OVER,
    TXT_COUNT
} TextID;

typedef enum {
    LANG_EN,
    LANG_DE,
    LANG_ZH,
    LANG_COUNT
} Language;

static const char *gText[LANG_COUNT][TXT_COUNT] = {
  {
    "",
    "Play",
    "WASD to move, space to shoot, p to pause",
    "Score: %d",
    "Game Over",
  },
  {
    "",
    "Spielen",
    "WASD zum bewegen, leertaste zum schiessen, p zum pausieren \n ausserdem: ä, ö und ü ",
    "Punktzahl: %d",
    "Game Over",
  },
  {
    "",
    "",
    "",
    "",
    "",
  },
};
