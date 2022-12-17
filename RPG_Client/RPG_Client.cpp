#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <Windows.h>
#include <thread>
using namespace std;

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#include "../MMO_Server/protocol_2022.h"

constexpr auto WINDOW_WIDTH = 1000;
constexpr auto WINDOW_HEIGHT = 700;

short current_stage = 0;
bool connect_net = false;

sf::RenderWindow* g_window;
sf::Font g_font;
sf::Text text;
sf::Texture* board;
sf::Sprite m_sprite;
string user_id = "\0";

sf::TcpSocket g_socket;

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
		if (!user_id.empty() && connect_net) {
			break;
		}
	}

	cout << "입력받은 ID: " << user_id << endl;

	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;
	strcpy_s(p.name, user_id.c_str());
	send_packet(&p);
}

void login_page() {
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

int main()
{
	login_page();

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "User Name");
	g_window = &window;

	thread net{ network_module };

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
			{
				int direction = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 4;
					break;
				case sf::Keyboard::Right:
					direction = 2;
					break;
				case sf::Keyboard::Up:
					direction = 1;
					break;
				case sf::Keyboard::Down:
					direction = 3;
					break;
				case sf::Keyboard::BackSpace:
					if (!user_id.empty() && current_stage == 0) {
						user_id.pop_back();
						text.setString(user_id);
					}
					break;
				case sf::Keyboard::Return:
					connect_net = true;
					break;
				case sf::Keyboard::Escape:
					net.join();
					window.close();
					break;
				}
				if (-1 != direction) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					send_packet(&p);
					cout << direction << endl;
				}
			}
			break;
			case sf::Event::TextEntered:
				if (event.text.unicode < 128) {
					if (user_id.size() < 18 && current_stage == 0
						&& (isupper(event.text.unicode) || islower(event.text.unicode) || isdigit(event.text.unicode))) 
					{
						user_id.push_back(event.text.unicode);
						text.setString(user_id);
					}
				}
				break;
			default:
				break;
			}
		}

		window.clear();
		window.draw(m_sprite);
		if (!user_id.empty()) {
			window.draw(text);
		}
		window.display();
	}

	return 0;
}