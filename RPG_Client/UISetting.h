#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "stdafx.h"


class UI_SETTING {
private:
	sf::RenderWindow window;

	int MAX_WID_TILE = 50;
	int MAX_HEI_TILE = 50;
	int WINDOW_WIDTH = 1000;
	int WINDOW_HEIGHT = 700;

	sf::Clock anim_clock;
	sf::Font g_font;
	sf::Text text;
	sf::Texture* board;

public:
	UI_SETTING();

	void loop_client_ui_event();
	void process_ui_event(sf::Event event);

	void setBoard(string dirAndfName, sf::IntRect rect);
	void load_stage_ui();

	bool set_text(float x, float y, unsigned int size, sf::Color color, string cont = "");
};