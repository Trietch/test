#pragma once
#include <iostream>

#include "Renderer.h"
#include "Window.h"

class Game
{
public:
	void run();

private:
	void renderInit();
	void windowInit();
	void mainLoop();

	Window window;
	Renderer renderer;
};