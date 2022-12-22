#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <Windows.h>
#include <array>
#include <thread>
using namespace std;

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#pragma comment (lib, "lib/freetype.lib")
#else
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#endif

// #pragma comment (lib, "lib/freetype.lib")

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#include "../MMO_Server/protocol_2022.h"

constexpr auto MAX_WID_TILE		= 50;
constexpr auto MAX_HEI_TILE		= 50;
constexpr auto WINDOW_WIDTH		= 1000;
constexpr auto WINDOW_HEIGHT	= 700;

enum STAGE { TITLE, HOME };
short current_stage = TITLE;

sf::RenderWindow* g_window;

sf::Clock anim_clock;

sf::Font g_font;
sf::Text text;
sf::Texture* board;
sf::Sprite m_sprite;
string user_id = "\0";

int MyId = -1;
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

	short direction = 2;

	int		id		= -1;
	int		hp		= -1;
	int		max_hp	= -1;
	int		exp		= -1;
	int		level	= -1;
	short	x, y;
};
array<PLAYER, MAX_USER + MAX_NPC> players;

sf::TcpSocket g_socket;

void login_page();
void main_page();

class MAPMgr {
public:
	sf::Texture* board;
	sf::Sprite m_sprite[MAX_WID_TILE][MAX_HEI_TILE];
};
MAPMgr mapMgr;

void send_packet(void* packet) {
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	g_socket.send(packet, p[0], sent);
}

void network_module() {
	cout << "working network module\n";
	sf::Socket::Status status = g_socket.connect("127.0.0.1", PORT_NUM);
	g_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		cout << "서버와 연결할 수 없습니다.\n";
		g_window->close();
	}
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_FAIL:
	{

	}
	break;
	case SC_LOGIN_OK:
	{
		main_page();
		cout << "Login OK\n";
	}
	break;
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* p = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		MyId = int(p->id);

		if (MyId != -1) {
			players[MyId].id		= MyId;
			players[MyId].hp		= static_cast<int>(p->hp);
			players[MyId].max_hp	= static_cast<int>(p->max_hp);
			players[MyId].exp		= static_cast<int>(p->exp);
			players[MyId].level		= static_cast<int>(p->level);
			players[MyId].x			= static_cast<short>(p->x);
			players[MyId].y			= static_cast<short>(p->y);

			for (int i = 0; i < 4; ++i) {
				players[MyId].p_texture[i] = new sf::Texture;
				players[MyId].p_texture[i]->loadFromFile("texture\\ChracterSprite.png");
				players[MyId].p_sprite[i].setTexture(*players[MyId].p_texture[i]);
				players[MyId].p_sprite[i].setTextureRect(sf::IntRect(0, 48 * i, 48, 48));
			}

			players[MyId].p_texture_a01 = new sf::Texture;
			players[MyId].p_texture_a01->loadFromFile("texture\\attack01.png");
			players[MyId].p_sprite_a01.setTexture(*players[MyId].p_texture_a01);
			players[MyId].p_sprite_a01.setTextureRect(sf::IntRect(20, 30, 70, 70));

			players[MyId].p_texture_a02 = new sf::Texture;
			players[MyId].p_texture_a02->loadFromFile("texture\\attack02.png");
			players[MyId].p_sprite_a02.setTexture(*players[MyId].p_texture_a02);
			players[MyId].p_sprite_a02.setTextureRect(sf::IntRect(0, 0, 165, 180));

			players[MyId].p_sprite[players[MyId].direction].setPosition(TILE_SIZE * players[MyId].x, TILE_SIZE * players[MyId].y);
		}
	}
	break;
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* p = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = p->id;
		players[other_id].x = p->x;
		players[other_id].y = p->y;
		players[other_id].p_sprite[players[other_id].direction].setPosition(TILE_SIZE * players[other_id].x, TILE_SIZE * players[other_id].y);
	}
	break;
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void login_page() 
{
	// 언어 설정
	wcout.imbue(locale("Korean"));
	// 아이디 입력을 위한 폰트 설정
	g_font.loadFromFile("cour.ttf");
	text.setFont(g_font);
	text.setCharacterSize(60);
	text.setPosition(190, 480);
	text.setFillColor(sf::Color::Black);
	// 배경 이미지 설정
	board = new sf::Texture;
	board->loadFromFile("texture\\title.png");
	m_sprite.setTexture(*board);
	m_sprite.setTextureRect(sf::IntRect(0, 0, 1000, 700));
}

void main_page() {
	current_stage = STAGE::HOME;

	board->loadFromFile("texture\\tileSprite.png");

	for (int hei = 0; hei < MAX_HEI_TILE; ++hei) {
		for (int wid = 0; wid < MAX_WID_TILE; ++wid) {
			mapMgr.m_sprite[wid][hei].setTexture(*board);
			mapMgr.m_sprite[wid][hei].setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
			mapMgr.m_sprite[wid][hei].setPosition(TILE_SIZE * wid, TILE_SIZE * hei);
		}
	}

	// text.setPosition(800, 480);
}

void client_main() {
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = g_socket.receive(net_buf, BUF_SIZE, received);

	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
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
					pl.p_sprite_a01.setPosition(TILE_SIZE* pl.x - 26, TILE_SIZE* pl.y - 34);
					pl.p_sprite_a01.setScale(2.f, 2.f);
					window.draw(pl.p_sprite_a01);
				}

				// player attack 02 animation
				if (pl.b_attack02) {
					if (anim_clock.getElapsedTime().asMilliseconds() > 100) {
						pl.attack_frame_02 += 187.4;

						anim_clock.restart();
						if (pl.attack_frame_02 > 749.6f && pl.attack_frame_02_y < 1100) {
							pl.attack_frame_02 = 0;
							pl.attack_frame_02_y += 184.4;
						}
						if (pl.attack_frame_02_y > 1100) {
							pl.attack_frame_02 = 0;
							pl.attack_frame_02_y = 0;
							pl.b_attack02 = false;
						}
						pl.p_sprite_a02.setTextureRect(sf::IntRect(pl.attack_frame_02, pl.attack_frame_02_y, 187.4, 184.4));
					}
					pl.p_sprite_a02.setPosition(TILE_SIZE * pl.x - 65, TILE_SIZE * pl.y - 47);
					window.draw(pl.p_sprite_a02);
				}
			}
		}
	}
	break;
	}
	window.draw(text);
}

int main()
{
	network_module();
	login_page();

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "User Name");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
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
						p.direction = players[MyId].direction = 0;
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
						p.direction = players[MyId].direction = 1;
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
						p.direction = players[MyId].direction = 2;
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
						p.direction = players[MyId].direction = 3;
						send_packet(&p);
					}
				}
				break;
				case sf::Keyboard::BackSpace:
				{
					if (!user_id.empty())
					{
						user_id.pop_back();
						text.setString(user_id);
					}
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
					}
				}
				break;
				case sf::Keyboard::Num1:
				{
					// attack Skill 01
					if (current_stage == STAGE::HOME && false == players[MyId].b_attack01 && false == players[MyId].b_attack02) {
						anim_clock.restart();
						players[MyId].b_attack01 = true;
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
		client_main();	// Server 연결시에만 사용할 것
		DrawMap(window);
		window.display();
	}

	return 0;
}
