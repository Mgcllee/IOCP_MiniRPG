#pragma once

#include "stdafx.h"

class PLAYER {
public:
	sf::Texture* p_texture[4];
	sf::Sprite p_sprite[4];

	sf::Texture* p_texture_a01;
	sf::Sprite p_sprite_a01;
	bool b_attack01 = false;
	int attack_frame_01 = 20;

	sf::Texture* p_texture_a02;
	sf::Sprite p_sprite_a02;
	bool b_attack02 = false;
	float attack_frame_02 = 0;
	float attack_frame_02_y = 0;

	sf::RectangleShape s_max_hp;
	sf::RectangleShape s_cur_hp;
	sf::RectangleShape s_max_exp;
	sf::RectangleShape s_cur_exp;

	sf::Text cur_level;

	DIRECTION direction = DIRECTION::RIGHT;

	char	name[NAME_SIZE];
	int		id = -1;
	int		hp = -1;
	int		max_hp = -1;
	int		exp = -1;
	int		level = -1;
	short	x, y;

public:
	PLAYER() {}
	PLAYER(int insert_id, bool mine) {
		/*for (int i = 0; i < 4; ++i) {
			players[insert_id].p_texture[i] = new sf::Texture;
			players[insert_id].p_texture[i]->loadFromFile("texture\\ChracterSprite.png");
			players[insert_id].p_sprite[i].setTexture(*players[insert_id].p_texture[i]);

			if (mine) {
				players[MyId].p_sprite[i].setTextureRect(sf::IntRect(0, 48 * i, 48, 48));
			}
			else {
				players[insert_id].p_sprite[i].setTextureRect(sf::IntRect(290, 193 + 48 * i, 48, 48));
			}
		}

		players[insert_id].p_sprite[players[insert_id].direction].setPosition(TILE_SIZE * players[insert_id].x, TILE_SIZE * players[insert_id].y);

		players[insert_id].p_texture_a01 = new sf::Texture;
		players[insert_id].p_texture_a01->loadFromFile("texture\\attack01.png");
		players[insert_id].p_sprite_a01.setTexture(*players[insert_id].p_texture_a01);
		players[insert_id].p_sprite_a01.setTextureRect(sf::IntRect(20, 30, 70, 70));

		players[insert_id].p_texture_a02 = new sf::Texture;
		players[insert_id].p_texture_a02->loadFromFile("texture\\attack02.png");
		players[insert_id].p_sprite_a02.setTexture(*players[insert_id].p_texture_a02);
		players[insert_id].p_sprite_a02.setTextureRect(sf::IntRect(0, 0, 165, 180));*/
	}
};