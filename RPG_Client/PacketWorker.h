#pragma once

#include "stdafx.h"

#include "../MMO_Server/protocol_2022.h"


class PACKET_WORKER {
private:

public:
	void send_packet(void* packet) {
		unsigned char* p = reinterpret_cast<unsigned char*>(packet);
		size_t sent = 0;
		g_socket.send(packet, p[0], sent);
	}

	bool network_module(string server_addr) {
		cout << "working network module\n";

		sf::Socket::Status status = g_socket.connect(server_addr, PORT_NUM);
		g_socket.setBlocking(false);

		if (status != sf::Socket::Done) {
			cout << "서버와 연결할 수 없습니다.\n";
			g_window->close();
			return false;
		}

		return true;
	}

	void addNewChat(string msg) {
		sf::Text t_buf;
		t_buf.setFont(g_font);
		t_buf.setCharacterSize(20);
		t_buf.setFillColor(sf::Color::White);
		t_buf.setString(msg);
		chat_log.push_back(t_buf);
	}

	void moveObj(PLAYER& target, int newX, int newY, DIRECTION dir, int sprite) {
		// players[other_id].p_sprite[players[other_id].direction].setPosition(TILE_SIZE * players[other_id].x, TILE_SIZE * players[other_id].y);

		target.x = newX;
		target.y = newY;
		target.direction = dir;
	}

	void ProcessPacket(char* ptr)
	{
		static bool first_time = true;
		switch (ptr[1])
		{
		case SC_LOGIN_FAIL:
		{
			user_id.clear();
			text.setString(user_id);
			printf("Fail Login\n");
		}
		break;
		case SC_LOGIN_OK:
		{
			main_page();
			printf("New Player Login!\n");
		}
		break;
		case SC_LOGIN_INFO:
		{
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
		break;
		case SC_MOVE_OBJECT:
		{
			SC_MOVE_OBJECT_PACKET* p = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
			moveObj(players[p->id], p->x, p->y, p->direction, NULL);
		}
		break;
		case SC_CHAT:
		{
			SC_CHAT_PACKET* p = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
			addNewChat(players[p->id].name + string(": ") + p->mess);
		}
		break;
		}
	}

	void process_data(char* net_buf, size_t io_byte)	// net_buf: 수신한 정보, io_byte: 수신정보의 총 길이
	{
		char* ptr = net_buf;

		static size_t in_packet_size = 0;		// 패킷에 담긴 전체 길이 정보를 저장
		static size_t saved_packet_size = 0;	// (32bit-4byte, 64bit-8byte)
		static char packet_buffer[BUF_SIZE]{};

		while (0 != io_byte) {
			// 패킷의 첫 부분은 패킷의 길이 정보가 담겨져 있음
			if (0 == in_packet_size) in_packet_size = ptr[0];

			if ((io_byte + saved_packet_size) >= in_packet_size) {
				memcpy((packet_buffer + saved_packet_size), ptr, (in_packet_size - saved_packet_size));

				// 수신된 패킷 처리.
				ProcessPacket(packet_buffer);

				// 패킷 내부 이동
				ptr += (in_packet_size - saved_packet_size);

				io_byte -= (in_packet_size - saved_packet_size);

				in_packet_size = 0;
				saved_packet_size = 0;
			}
			// 수신된 패킷의 처리가 완료됨(-> static(localScope)이므로 socpe가 종료되어도 변수가 유지됨.(프로그램 수명과 함께 끝난다.))
			else {
				memcpy(packet_buffer + saved_packet_size, ptr, io_byte);

				saved_packet_size += io_byte;
				io_byte = 0;
			}
		}
	}
};