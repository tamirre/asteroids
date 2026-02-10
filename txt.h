#pragma once
// AUTO GENERATED - DO NOT EDIT
// Generated from: txt.csv

typedef enum {
	TXT_EMPTY,
	TXT_PLAY,
	TXT_INSTRUCTIONS,
	TXT_SCORE,
	TXT_GAME_OVER,
	TXT_EXPERIENCE,
	TXT_LEVEL_UP,
	TXT_CHOOSE_UPGRADE,
	TXT_GAME_PAUSED,
	TXT_TRY_AGAIN,
	TXT_PRESS_TO_PLAY,
	TXT_GAME_TITLE,
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
	"XP",
	"LEVEL UP!",
	"CHOOSE UPGRADE",
	"Game is paused…",
	"Press enter to try again",
	"Press enter to play",
	"Asteroids",
  },
  {
	"",
	"Spielen",
	"WASD zum bewegen, leertaste zum schiessen, p zum pausieren",
	"Punktzahl: %d",
	"Game Over",
	"XP",
	"LEVEL UP!",
	"WÄHLE EIN UPGRADE",
	"Spiel ist pausiert...",
	"Drück enter um es erneut zu versuchen",
	"Drück enter um zu spielen",
	"Asteroids",
  },
  {
	"",
	"玩",
	"WASD键移动, 空格键射击, P键暂停",
	"",
	"",
	"",
	"",
	"",
	"游戏暂停...",
	"",
	"",
	"",
  },
};
