#pragma once

#include "stdafx.h"

int main() {
	network_module("127.0.0.1");
	login_page();

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "User Name");
	g_window = &window;

	while (window.isOpen()) {
		sf::Event event;

		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
			{
				window.close();
			}
			break;
			case sf::Event::TextEntered:
			{
				if (event.text.unicode < 128)
				{
					if (user_id.size() < 18 && (isupper(event.text.unicode) || islower(event.text.unicode) || isdigit(event.text.unicode)))
					{
						user_id.push_back(event.text.unicode);
						text.setString(user_id);
					}
				}
			}
			break;
			case sf::Event::KeyPressed:
			{
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
				case sf::Keyboard::Num2:
				{
					// attack 02
					if (current_stage == STAGE::HOME && false == players[MyId].b_attack01 && false == players[MyId].b_attack02) {
						anim_clock.restart();
						players[MyId].b_attack02 = true;
					}
				}
				break;
				}
			}
			break;
			}
		}

		window.clear();
		client_main();
		DrawMap(window);
		window.display();
	}

	return 0;
}
