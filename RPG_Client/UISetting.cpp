#pragma once

#include "UISetting.h"


UI_SETTING::UI_SETTING() {
	window = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "User Name");

	wcout.imbue(locale("korean"));

	g_font.loadFromFile("cour.ttf");
	text.setFont(g_font);
	text.setCharacterSize(60);
	text.setPosition(190, 480);
	text.setFillColor(sf::Color::Black);

	board = new sf::Texture;

	setBoard("texture\\title.png", sf::IntRect(0, 0, 1000, 700));
	setStage(STAGE::TITLE);
}

void UI_SETTING::loop_client_ui_event() {
	while (window.isOpen()) {
		sf::Event event;

		while (window.pollEvent(event)) {
			process_ui_event(event);
		}

		window.clear();
		window.display();
	}
}

void UI_SETTING::process_ui_event(sf::Event event) {
	switch (event.type) {
		case sf::Event::Closed: {
			window.close();
			break;
		}
		case sf::Event::TextEntered: {

			break;
		}
		case sf::Event::KeyPressed: {
			switch (event.key.code) {
			case sf::Keyboard::Escape:
				window.close();
				break;
			case sf::Keyboard::Down:
			{
				if (STAGE::HOME == current_stage) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = players[MyId].direction = DIRECTION::DOWN;
					send_packet(&p);
				}
			}
			break;
			case sf::Keyboard::Left:
			{
				if (STAGE::HOME == current_stage) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = players[MyId].direction = DIRECTION::LEFT;
					send_packet(&p);
				}
			}
			break;
			case sf::Keyboard::Right:
			{
				if (STAGE::HOME == current_stage) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = players[MyId].direction = DIRECTION::RIGHT;
					send_packet(&p);
				}
			}
			break;
			case sf::Keyboard::Up:
			{
				if (STAGE::HOME == current_stage) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = players[MyId].direction = DIRECTION::UP;
					send_packet(&p);
				}
			}
			break;
			case sf::Keyboard::BackSpace:
				if (!user_id.empty())
				{
					user_id.pop_back();
					text.setString(user_id);
				}
				break;
			case sf::Keyboard::Space:
			{
				user_id.push_back(' ');
			}
			break;
			case sf::Keyboard::Return:
			{
				if (STAGE::TITLE == current_stage) {
					CS_LOGIN_PACKET p;
					p.size = sizeof(p);
					p.type = CS_LOGIN;
					strcpy_s(p.name, user_id.c_str());
					send_packet(&p);
				}
				else if (STAGE::HOME == current_stage) {
					// send chat
					CS_CHAT_PACKET p;
					p.size = sizeof(p);
					p.type = CS_CHAT;
					strcpy_s(p.mess, user_id.c_str());
					send_packet(&p);
					// 채팅창 지우기
					user_id.clear();
					text.setString(user_id);
				}
			}
			break;
			case sf::Keyboard::Num1:
			{
				// attack Skill 01
				if (current_stage == STAGE::HOME && false == players[MyId].b_attack01 && false == players[MyId].b_attack02) {
					anim_clock.restart();
					players[MyId].b_attack01 = true;
					CS_TELEPORT_PACKET p;
					p.size = sizeof(p);
					p.type = CS_ATTACK;
					send_packet(&p);
				}
			}
			break;
			case sf::Keyboard::Num2: {

			}
								   break;
			}
								  break;
		}
	}
}

void UI_SETTING::setBoard(string dirAndfName, sf::IntRect rect) {
	board->loadFromFile(dirAndfName);
	m_sprite.setTexture(*board);
	m_sprite.setTextureRect(rect);
}

void UI_SETTING::load_stage_ui() {
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

bool UI_SETTING::set_text(float x, float y, unsigned int size, sf::Color color, string cont) {
	text.setPosition(x, y);
	text.setCharacterSize(size);
	text.setFillColor(color);

	if (false == cont.empty()) text.setString(cont);
	else text.setString("");

	return true;
}