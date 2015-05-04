#ifndef MYGAME_H
#define MYGAME_H

#include "mygame_math.h"

struct Button {
	bool isDown;
	bool wasDown;
	vec2 position;
	vec2 dimensions;
};

#define BUTTONS_COUNT 4
struct Input {
	union {
		Button buttons[BUTTONS_COUNT];
		struct {
			Button leftButton;
			Button rightButton;
			Button forwardButton;
			Button fireButton;
		};
	};
};

extern Input g_input;
extern void(*platformLog)(const char *, ...);

void setViewport(float windowWidth, float windowHeight);
bool initGame();
void gameUpdateAndRender(float dt, float *touches);

#endif
