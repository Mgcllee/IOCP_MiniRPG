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

		SC_LOGIN_INFO_PACKET* p = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		int new_id = int(p->id);

		// if (MyId == -1 && new_id >= 0) {
		if (MyId == -1) {
			MyId = new_id;
			players[MyId].id = MyId;
			players[MyId].hp = 190;
			players[MyId].max_hp = 200;

			players[MyId].exp = static_cast<int>(p->exp);
			players[MyId].level = (int)(players[MyId].exp / 60) + 1;
			players[MyId].x = static_cast<short>(p->x);
			players[MyId].y = static_cast<short>(p->y);
			strcpy_s(players[MyId].name, p->name);

			players[MyId].s_max_exp = sf::RectangleShape(sf::Vector2f(600.f, 10.f));
			players[MyId].s_max_exp.setFillColor(sf::Color::White);
			players[MyId].s_max_exp.setPosition(sf::Vector2f(0.f, 685.f));

			players[MyId].s_cur_exp = sf::RectangleShape(sf::Vector2f((float)(players[MyId].exp % 60) * 10.f, 10.f));
			players[MyId].s_cur_exp.setFillColor(sf::Color::Yellow);
			players[MyId].s_cur_exp.setPosition(sf::Vector2f(0.f, 685.f));

			players[MyId].s_max_hp = sf::RectangleShape(sf::Vector2f(200.f, 20.f));
			players[MyId].s_max_hp.setFillColor(sf::Color::White);
			players[MyId].s_max_hp.setPosition(sf::Vector2f(0.f, 660.f));

			players[MyId].s_cur_hp = sf::RectangleShape(sf::Vector2f((float)players[MyId].hp, 20.f));
			players[MyId].s_cur_hp.setFillColor(sf::Color::Red);
			players[MyId].s_cur_hp.setPosition(sf::Vector2f(0.f, 660.f));

			string view_level = "0";
			view_level[0] += players[MyId].level;
			players[MyId].cur_level.setFont(g_font);
			players[MyId].cur_level.setCharacterSize(40);
			players[MyId].cur_level.setPosition(10, 10);
			players[MyId].cur_level.setFillColor(sf::Color::White);
			players[MyId].cur_level.setOutlineColor(sf::Color::Black);
			players[MyId].cur_level.setOutlineThickness(2.f);
			players[MyId].cur_level.setString(string("Level: ").append(view_level));
		}
		else {
			if (new_id != MyId) {
				players[new_id].id = MyId;
				players[new_id].hp = static_cast<int>(p->hp);
				players[new_id].max_hp = static_cast<int>(p->max_hp);
				players[new_id].exp = static_cast<int>(p->exp);
				players[new_id].level = static_cast<int>(p->level);
				players[new_id].x = static_cast<short>(p->x);
				players[new_id].y = static_cast<short>(p->y);
				strcpy_s(players[new_id].name, p->name);
			}
		}
	}

	void moveObj(PLAYER& target, int newX, int newY, DIRECTION dir, int sprite) {
		// players[other_id].p_sprite[players[other_id].direction].setPosition(TILE_SIZE * players[other_id].x, TILE_SIZE * players[other_id].y);

		target.x = newX;
		target.y = newY;
		target.direction = dir;
	}
};