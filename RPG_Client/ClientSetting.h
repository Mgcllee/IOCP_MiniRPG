#pragma once

#include "stdafx.h"

enum STAGE { TITLE, HOME };
short current_stage = TITLE;



sf::Sprite m_sprite;
string user_id = "\0";
int MyId = -1;

array<PLAYER, MAX_USER + MAX_NPC> players;
sf::TcpSocket g_socket;

class MAPMgr {
public:
	sf::Texture* board;
	sf::Sprite m_sprite[MAX_WID_TILE][MAX_HEI_TILE];
};
MAPMgr mapMgr;

void client_main() {
	char net_buf[BUF_SIZE];
	size_t	received{};

	auto recv_result = g_socket.receive(net_buf, BUF_SIZE, received);

	switch (recv_result) {
	case sf::Socket::Disconnected:
		printf("This Client is disconnect!\n");
		exit(EXIT_FAILURE);

		// return title menu

		break;
	case sf::Socket::Error:
		printf("This Client is Error!\n");
		exit(EXIT_FAILURE);
		break;
	case sf::Socket::Done:
	case sf::Socket::NotReady:
	case sf::Socket::Partial:

		break;
	}

	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);
}

void DrawMap(sf::RenderWindow& window) {
	switch (current_stage)
	{
	case STAGE::TITLE:
	{
		window.draw(m_sprite);
	}
	break;
	case STAGE::HOME:
	{
		// map tile
		for (int hei = 0; hei < MAX_HEI_TILE; ++hei) {
			for (int wid = 0; wid < MAX_WID_TILE; ++wid) {
				window.draw(mapMgr.m_sprite[wid][hei]);
			}
		}
		// players
		for (PLAYER& pl : players) {
			if (pl.id != -1) {
				// player character
				window.draw(pl.p_sprite[pl.direction]);

				// player attack 01 animation
				if (pl.b_attack01) {
					if (anim_clock.getElapsedTime().asMilliseconds() > 300) {
						pl.attack_frame_01 += 70;
						anim_clock.restart();
						if (pl.attack_frame_01 > 240) {
							pl.attack_frame_01 = 20;
							pl.b_attack01 = false;
						}
						pl.p_sprite_a01.setTextureRect(sf::IntRect(pl.attack_frame_01, 30, 70, 70));
					}
					pl.p_sprite_a01.setPosition(TILE_SIZE * pl.x - 26, TILE_SIZE * pl.y - 34);
					pl.p_sprite_a01.setScale(2.f, 2.f);
					window.draw(pl.p_sprite_a01);
				}

				// player attack 02 animation
				if (pl.b_attack02) {
					if (anim_clock.getElapsedTime().asMilliseconds() > 100) {
						pl.attack_frame_02 += 187.4f;

						anim_clock.restart();
						if (pl.attack_frame_02 > 749.6f && pl.attack_frame_02_y < 1100) {
							pl.attack_frame_02 = 0;
							pl.attack_frame_02_y += 184.4f;
						}
						if (pl.attack_frame_02_y > 1100) {
							pl.attack_frame_02 = 0;
							pl.attack_frame_02_y = 0;
							pl.b_attack02 = false;
						}
						pl.p_sprite_a02.setTextureRect(sf::IntRect((int)pl.attack_frame_02, (int)pl.attack_frame_02_y, (int)187.4, (int)184.4));
					}
					pl.p_sprite_a02.setPosition(TILE_SIZE * pl.x - 65, TILE_SIZE * pl.y - 47);
					window.draw(pl.p_sprite_a02);
				}
			}
		}

		if (!chat_log.empty()) {
			reverse(chat_log.begin(), chat_log.end());
			int pos_y = 640;
			for (sf::Text& t : chat_log) {
				pos_y -= 15;
				if (pos_y < 490)
					break;
				t.setPosition(700.f, (float)pos_y);
				window.draw(t);
			}
			reverse(chat_log.begin(), chat_log.end());
		}

		window.draw(players[MyId].s_max_exp);
		window.draw(players[MyId].s_cur_exp);
		window.draw(players[MyId].s_max_hp);
		window.draw(players[MyId].s_cur_hp);
		window.draw(players[MyId].cur_level);
	}
	break;
	}
	window.draw(text);
}