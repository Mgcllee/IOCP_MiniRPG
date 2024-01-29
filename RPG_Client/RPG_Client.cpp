#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <array>
using namespace std;

#ifdef _DEBUG
#pragma comment (lib, "sfml-graphics-d.lib")
#pragma comment (lib, "sfml-window-d.lib")
#pragma comment (lib, "sfml-system-d.lib")
#pragma comment (lib, "sfml-network-d.lib")
#else
#pragma comment (lib, "sfml-graphics.lib")
#pragma comment (lib, "sfml-window.lib")
#pragma comment (lib, "sfml-system.lib")
#pragma comment (lib, "sfml-network.lib")
#endif
#pragma comment (lib, "freetype.lib")

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
vector<sf::Text> chat_log;
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

	sf::RectangleShape s_max_hp;
	sf::RectangleShape s_cur_hp;
	sf::RectangleShape s_max_exp;
	sf::RectangleShape s_cur_exp;

	sf::Text cur_level;

	DIRECTION direction = DIRECTION::RIGHT;

	char	name[NAME_SIZE];
	int		id		= -1;
	int		hp		= -1;
	int		max_hp	= -1;
	int		exp		= -1;
	int		level	= -1;
	short	x, y;

public:
	PLAYER() { } 
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
			players[MyId].id		= MyId;
			players[MyId].hp		= 190;
			players[MyId].max_hp	= 200;
			
			players[MyId].exp		= static_cast<int>(p->exp);
			players[MyId].level		= (int)(players[MyId].exp / 60) + 1;
			players[MyId].x			= static_cast<short>(p->x);
			players[MyId].y			= static_cast<short>(p->y);
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
				players[new_id].hp =		static_cast<int>(p->hp);
				players[new_id].max_hp =	static_cast<int>(p->max_hp);
				players[new_id].exp =		static_cast<int>(p->exp);
				players[new_id].level =		static_cast<int>(p->level);
				players[new_id].x =			static_cast<short>(p->x);
				players[new_id].y =			static_cast<short>(p->y);
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
	~OPTIONS() { }


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

// Add inf tile and background class

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
					pl.p_sprite_a01.setPosition(TILE_SIZE* pl.x - 26, TILE_SIZE* pl.y - 34);
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

int main()
{
	network_module("127.0.0.1");
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
		client_main();	// Server 연결시에만 사용할 것
		DrawMap(window);
		window.display();
	}

	return 0;
}
