#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <Windows.h>
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

constexpr auto TILE_SIZE		= 42.5;
constexpr auto MAX_WID_TILE		= 50;
constexpr auto MAX_HEI_TILE		= 50;
constexpr auto WINDOW_WIDTH		= 1000;
constexpr auto WINDOW_HEIGHT	= 700;

enum STAGE { TITLE, HOME };
short current_stage = TITLE;

sf::RenderWindow* g_window;
sf::Font g_font;
sf::Text text;
sf::Texture* board;
sf::Sprite m_sprite;
string user_id = "\0";

//int MyId;
//class PLAYER {
//public:
//	sf::Texture* p_texture;
//	sf::Sprite p_sprite;
//
//	int		id = -1;
//	int		hp = -1;
//	int		max_hp = -1;
//	int		exp = -1;
//	int		level = -1;
//	short	x, y;
//};
//array<PLAYER, MAX_USER + MAX_NPC> players;

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

	while (true) {
		if (!user_id.empty() && (STAGE::HOME == current_stage)) {
			break;
		}
	}
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);

		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		break;
	}
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
	board->loadFromFile("texture\\tileSprite.png");

	for (int hei = 0; hei < MAX_HEI_TILE; ++hei) {
		for (int wid = 0; wid < MAX_WID_TILE; ++wid) {
			mapMgr.m_sprite[wid][hei].setTexture(*board);
			mapMgr.m_sprite[wid][hei].setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
			mapMgr.m_sprite[wid][hei].setPosition(TILE_SIZE * wid, TILE_SIZE * hei);
		}
	}
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
		window.draw(m_sprite);
		window.draw(text);
		break;
	case STAGE::HOME:
		for (int hei = 0; hei < MAX_HEI_TILE; ++hei) {
			for (int wid = 0; wid < MAX_WID_TILE; ++wid) {
				window.draw(mapMgr.m_sprite[wid][hei]);
			}
		}
		break;
	}
}


int main()
{
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
			case sf::Event::KeyPressed:
			{
				short direction = -1;
				switch (event.key.code) {
				case sf::Keyboard::Up:
					if (STAGE::HOME == current_stage)
						direction = 0;
					break;
				case sf::Keyboard::Right:
					if (STAGE::HOME == current_stage)
						direction = 1;
					break;
				case sf::Keyboard::Down:
					if (STAGE::HOME == current_stage)
						direction = 2;
					break;
				case sf::Keyboard::Left:
					if (STAGE::HOME == current_stage)
						direction = 3;
					break;
				case sf::Keyboard::BackSpace:
					if (!user_id.empty() && (current_stage == STAGE::TITLE))
					{
						user_id.pop_back();
						text.setString(user_id);
					}
					break;
				case sf::Keyboard::Return:
				if(STAGE::TITLE == current_stage) {
					CS_LOGIN_PACKET p;
					p.size = sizeof(p);
					p.type = CS_LOGIN;
					strcpy_s(p.name, user_id.c_str());
					send_packet(&p);
				}
				else if (STAGE::HOME == current_stage) {
					// send chat 
				}
				break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
				if (-1 != direction) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					send_packet(&p);
				}
			}
			break;
			case sf::Event::TextEntered:
			{
				if (event.text.unicode < 128)
				{
					if (user_id.size() < 18 && current_stage == 0 && (isupper(event.text.unicode) || islower(event.text.unicode) || isdigit(event.text.unicode)))
					{
						user_id.push_back(event.text.unicode);
						text.setString(user_id);
					}
				}
			}
			break;
			}

			window.clear();
			// client_main();	// Server 연결시에만 사용할 것
			DrawMap(window);
			window.display();
		}
	}

	return 0;
}
