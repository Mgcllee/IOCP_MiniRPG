#pragma once

#include "stdafx.h"

constexpr auto MAX_WID_TILE = 50;
constexpr auto MAX_HEI_TILE = 50;
constexpr auto WINDOW_WIDTH = 1000;
constexpr auto WINDOW_HEIGHT = 700;

enum STAGE { TITLE, HOME };
short current_stage = TITLE;

sf::RenderWindow* g_window;
sf::Clock anim_clock;
sf::Font g_font;
sf::Text text;
sf::Texture* board;
sf::Sprite m_sprite;
string user_id = "\0";
vector<sf::Text> chat_log;
int MyId = -1;

array<PLAYER, MAX_USER + MAX_NPC> players;
sf::TcpSocket g_socket;

class MAPMgr {
public:
	sf::Texture* board;
	sf::Sprite m_sprite[MAX_WID_TILE][MAX_HEI_TILE];
};
MAPMgr mapMgr;

class OPTIONS {
public:
	OPTIONS()
	{
		//Default Option
		setLanguage("korean");
		setFont();

		//Default Background
		board = new sf::Texture;
	}
	OPTIONS(string setting_language)
	{
		//Default Option
		setLanguage(setting_language);
		setFont();

		//Default Background
		board = new sf::Texture;
	}
	~OPTIONS() {}


	//Set client language and local
	bool setLanguage(string lang)
	{
		//if set success what is return value
		wcout.imbue(locale(lang));

		//global language
		// setlocale(LC_ALL, "");
		return true;
	}

	bool setFont() {
		if (g_font.loadFromFile("cour.ttf")) {
			text.setFont(g_font);
			text.setCharacterSize(60);
			text.setPosition(190, 480);
			text.setFillColor(sf::Color::Black);
			return true;
		}

		printf("failure load font!\n");
		return false;
	}

	bool setText(float x, float y, unsigned int size, sf::Color color, string cont = "")
	{
		//return false case

		text.setPosition(x, y);
		text.setCharacterSize(size);
		text.setFillColor(color);

		if (false == cont.empty()) text.setString(cont);
		else text.setString("");

		return true;
	}

	void setBoard(string dirAndfName, sf::IntRect rect)
	{
		board->loadFromFile(dirAndfName);
		m_sprite.setTexture(*board);
		m_sprite.setTextureRect(rect);
	}

	void setStage(STAGE st)
	{
		current_stage = st;
	}
};
OPTIONS options;

void login_page()
{
	// 배경 이미지 설정
	options.setBoard("texture\\title.png", sf::IntRect(0, 0, 1000, 700));
	options.setStage(STAGE::TITLE);
}

void main_page() {
	current_stage = STAGE::HOME;

	board->loadFromFile("texture\\tileSprite.png");

	for (int hei = 0; hei < MAX_HEI_TILE; ++hei) {
		for (int wid = 0; wid < MAX_WID_TILE; ++wid) {
			mapMgr.m_sprite[wid][hei].setTexture(*board);
			mapMgr.m_sprite[wid][hei].setTextureRect(sf::IntRect(0, 0, (int)TILE_SIZE, (int)TILE_SIZE));
			mapMgr.m_sprite[wid][hei].setPosition(TILE_SIZE * wid, TILE_SIZE * hei);
		}
	}

	user_id.clear();
	options.setText(720, 650, 20, sf::Color::White, user_id);
}

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