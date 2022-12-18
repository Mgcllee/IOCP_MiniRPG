#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include "protocol_2022.h"

extern "C"
{
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}
#pragma comment (lib, "lua54.lib")

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

constexpr int VIEW_RANGE = 5;

enum EVENT_TYPE { EV_RANDOM_MOVE, EV_STOP_MOVE, EV_START_MOVE, EV_NPC_DIST };

struct TIMER_EVENT {
	int obj_id;
	chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE event_id;
	int target_id;
	constexpr bool operator < (const TIMER_EVENT& L) const
	{
		return (wakeup_time > L.wakeup_time);
	}
};
concurrency::concurrent_priority_queue<TIMER_EVENT> timer_queue;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE, OP_AI_HELLO, OP_AI_BYE, OP_AI_DIST };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int _ai_target_obj;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	atomic_bool	_is_active;		// 주위에 플레이어가 있는가?
	int _id;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	int		_prev_remain;
	unordered_set <int> _view_list;
	mutex	_vl;
	int		last_move_time;

	lua_State* _L;
	mutex _ll;

	short move_count{};
	bool use_dist = false;

public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_chat_packet(int c_id, const char* mess);
	void send_remove_player_packet(int c_id)
	{
		_vl.lock();
		if (_view_list.count(c_id))
			_view_list.erase(c_id);
		else {
			_vl.unlock();
			return;
		}
		_vl.unlock();

		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		do_send(&p);
	}
};

HANDLE h_iocp;
array<SESSION, MAX_USER + MAX_NPC> clients;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

int API_get_y(lua_State* L);
int API_get_x(lua_State* L);
int API_SendMessage(lua_State* L);

bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

bool is_npc(int object_id)
{
	return !is_pc(object_id);
}

bool can_see(int from, int to)
{
	if (abs(clients[from].x - clients[to].x) > VIEW_RANGE) return false;
	return abs(clients[from].y - clients[to].y) <= VIEW_RANGE;
}

bool is_moving(int from, int to)
{
	if (abs(clients[from].x - clients[to].x) > 2) return false;
	return abs(clients[from].y - clients[to].y) <= 2;
}

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.move_time = clients[c_id].last_move_time;
	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, clients[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = clients[c_id].x;
	add_packet.y = clients[c_id].y;
	_vl.lock();
	_view_list.insert(c_id);
	_vl.unlock();
	do_send(&add_packet);
}

void SESSION::send_chat_packet(int p_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = p_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	do_send(&packet);
}

int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]._s_lock };
		if (clients[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

void WakeUpNPC(int npc_id, int waker)
{
	if (is_pc(npc_id)) {
		cout << "ERROR" << endl;
	}

	if (clients[npc_id]._is_active)	// 현재상태가 켜저있으면 깨울 필요없음
		return;

	OVER_EXP* exover = new OVER_EXP;
	exover->_comp_type = OP_AI_HELLO;
	exover->_ai_target_obj = waker;
	PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->_over);

	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&clients[npc_id]._is_active, &old_state, true))
		return;

	clients[npc_id].move_count = 0;

	TIMER_EVENT ev{ npc_id, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	timer_queue.push(ev);

	TIMER_EVENT dist_ev{ npc_id, chrono::system_clock::now() + 1ms, EV_NPC_DIST, 0 };
	timer_queue.push(dist_ev);
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(clients[c_id]._name, p->name);
		{
			lock_guard<mutex> ll{ clients[c_id]._s_lock };
			clients[c_id].x = rand() % W_WIDTH;
			clients[c_id].y = rand() % W_HEIGHT;
			clients[c_id]._state = ST_INGAME;
		}
		clients[c_id].send_login_info_packet();
		for (auto& pl : clients) {
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (pl._id == c_id) continue;
			if (false == can_see(c_id, pl._id))
				continue;
			if (is_pc(pl._id)) pl.send_add_player_packet(c_id);
			else WakeUpNPC(pl._id, c_id);
			clients[c_id].send_add_player_packet(pl._id);
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id].last_move_time = p->move_time;
		short x = clients[c_id].x;
		short y = clients[c_id].y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}

		clients[c_id].x = x;
		clients[c_id].y = y;

		unordered_set<int> near_list;
		clients[c_id]._vl.lock();
		unordered_set<int> old_vlist = clients[c_id]._view_list;
		clients[c_id]._vl.unlock();
		for (auto& cl : clients) {
			if (cl._state != ST_INGAME) continue;
			if (cl._id == c_id) continue;
			if (can_see(c_id, cl._id))
				near_list.insert(cl._id);
		}

		// 현재 플레이어에게 이동된 좌표 보내주기
		clients[c_id].send_move_packet(c_id);

		for (auto& pl : near_list) {
			auto& cpl = clients[pl];
			if (is_pc(pl)) {
				cpl._vl.lock();
				if (clients[pl]._view_list.count(c_id)) {
					cpl._vl.unlock();
					clients[pl].send_move_packet(c_id);

				}
				else {
					cpl._vl.unlock();
					clients[pl].send_add_player_packet(c_id);
				}
			}
			else WakeUpNPC(pl, c_id);	// pl : NPC_ID //// c_id : my_id

			if (old_vlist.count(pl) == 0) {
				clients[c_id].send_add_player_packet(pl);
			}
		}

		for (auto& pl : old_vlist)
			if (0 == near_list.count(pl)) {
				clients[c_id].send_remove_player_packet(pl);
				if (is_pc(pl))
					clients[pl].send_remove_player_packet(c_id);
			}
		break;
	}
	}
}

void disconnect(int c_id)
{
	clients[c_id]._vl.lock();
	unordered_set <int> vl = clients[c_id]._view_list;
	clients[c_id]._vl.unlock();
	for (auto& p_id : vl) {
		if (is_npc(p_id)) continue;
		auto& pl = clients[p_id];
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		pl.send_remove_player_packet(c_id);
	}
	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
}

void do_npc_random_move(int npc_id)
{
	SESSION& npc = clients[npc_id];

	unordered_set<int> old_vl;
	for (auto& obj : clients) {
		if (ST_INGAME != obj._state) continue;
		if (true == is_npc(obj._id)) continue;
		if (true == can_see(npc._id, obj._id))
			old_vl.insert(obj._id);
	}

	int x = npc.x;
	int y = npc.y;
	switch (rand() % 4) {
	case 0: if (x < (W_WIDTH - 1)) x++; break;
	case 1: if (x > 0) x--; break;
	case 2: if (y < (W_HEIGHT - 1)) y++; break;
	case 3:if (y > 0) y--; break;
	}
	npc.x = x;
	npc.y = y;

	unordered_set<int> new_vl;
	for (auto& obj : clients) {
		if (ST_INGAME != obj._state) continue;
		if (true == is_npc(obj._id)) continue;
		if (true == can_see(npc._id, obj._id))
			new_vl.insert(obj._id);
	}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			// 플레이어의 시야에 등장
			clients[pl].send_add_player_packet(npc._id);
		}
		else {
			// 플레이어가 계속 보고 있음.
			clients[pl].send_move_packet(npc._id);
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			clients[pl]._vl.lock();
			if (0 != clients[pl]._view_list.count(npc._id)) {
				clients[pl]._vl.unlock();
				clients[pl].send_remove_player_packet(npc._id);
			}
			else {
				clients[pl]._vl.unlock();
			}
		}
	}
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		
		// error 검출기
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}
		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				clients[client_id].x = 0;
				clients[client_id].y = 0;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket), h_iocp, client_id, 0);
				clients[client_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND: {
			delete ex_over;
			break;
		}
		case OP_NPC_MOVE:
		{
			bool keep_alive = false;
			int c_num;
			for (int j = 0; j < MAX_USER; ++j)
				if (can_see(static_cast<int>(key), j)) {
					keep_alive = true;
					c_num = j;
					break;
				}
			if (true == keep_alive) {
				do_npc_random_move(static_cast<int>(key));

				if ((clients[static_cast<int>(key)].move_count < 3) && clients[key].use_dist) {
					clients[static_cast<int>(key)].move_count += 1;
					TIMER_EVENT ev{ key, chrono::system_clock::now(), EV_START_MOVE, 0 };
					timer_queue.push(ev);
				}
				else if (clients[static_cast<int>(key)].move_count == 3) {
					clients[static_cast<int>(key)].move_count = 0;
					TIMER_EVENT ev{ key, chrono::system_clock::now(), EV_STOP_MOVE, 0 };
					timer_queue.push(ev);
				}
				else if ((clients[static_cast<int>(key)].move_count == 2) && !clients[key].use_dist) {
					clients[static_cast<int>(key)].move_count += 1;
					TIMER_EVENT ev{ key, chrono::system_clock::now(), EV_START_MOVE, 0 };
					timer_queue.push(ev);
				}
				else {
					clients[static_cast<int>(key)].move_count = 0;
				}

				TIMER_EVENT dist_ev{ key, chrono::system_clock::now() + 1ms, EV_NPC_DIST, 0 };
				timer_queue.push(dist_ev);

				TIMER_EVENT ev{ key, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
				timer_queue.push(ev);
			}
			else {
				clients[key]._is_active = false;
				clients[key].move_count = 0;
			}
			delete ex_over;
		}
		break;
		case OP_AI_HELLO:
		{
			clients[key]._ll.lock();
			auto L = clients[key]._L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			if (lua_pcall(L, 1, 0, 0) != 0) {
				string err = lua_tostring(L, -1);
				cout << err << endl;
			}
			lua_pop(L, 1);
			lua_settop(L, 0);
			clients[key]._ll.unlock();
			delete ex_over;
		}
		break;
		case OP_AI_BYE:
		{
			clients[key]._ll.lock();
			auto L = clients[key]._L;
			lua_getglobal(L, "event_player_bye");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			if (lua_pcall(L, 1, 0, 0) != 0) {
				string err = lua_tostring(L, -1);
				cout << err << endl;
			}
			lua_pop(L, 1);
			lua_settop(L, 0);
			clients[key]._ll.unlock();
			delete ex_over;
		}
		break;
		case OP_AI_DIST:
		{
			clients[key]._ll.lock();
			auto L = clients[key]._L;
			lua_getglobal(L, "dist_player_npc");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			if (lua_pcall(L, 1, 1, 0) != 0) {
				string err = lua_tostring(L, -1);
				cout << err << endl;
			}
			clients[key].use_dist = lua_tonumber(L, -1);
			if (clients[key].use_dist) {
				sprintf_s(clients[key]._name, "NPC%d", key);
			}
			lua_pop(L, 1);
			lua_settop(L, 0);
			clients[key]._ll.unlock();
			delete ex_over;
		}
		break;
		}
	}
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);		// NPC의 ID
	int player = (int)lua_tointeger(L, -2);		// 현재 플레이어의 ID
	char* message = (char*)lua_tostring(L, -1);	// 메시지 내용
	lua_pop(L, 4);

	clients[player].send_chat_packet(my_id, message);

	cout << "MSG: " << message << endl;

	return 0;
}

int API_get_x(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = clients[obj_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = clients[obj_id].y;
	lua_pushnumber(L, y);
	return 1;
}

void InitializeNPC()
{
	cout << "InitializeNPC Start\n";
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		clients[i].x = rand() % W_WIDTH;
		clients[i].y = rand() % W_HEIGHT;
		sprintf_s(clients[i]._name, "NPC%d", i);
		clients[i]._id = i;
		clients[i]._state = ST_INGAME;

		auto L = clients[i]._L = luaL_newstate();
		luaL_openlibs(L);

		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);

		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_SendMessage", API_SendMessage);
	}
	cout << "InitializeNPC End\n";
}

void do_timer()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		if (true == timer_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				timer_queue.push(ev);			// 아껴먹기
				this_thread::sleep_for(1ms);
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
			{
				OVER_EXP* ov = new OVER_EXP;
				ov->_comp_type = OP_NPC_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
			}
			break;
			case EV_STOP_MOVE:
			{
				OVER_EXP* bye_ov = new OVER_EXP;
				bye_ov->_comp_type = OP_AI_BYE;
				bye_ov->_ai_target_obj = ev.target_id;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &bye_ov->_over);
			}
			break;
			case EV_START_MOVE:
			{
				OVER_EXP* bye_ov = new OVER_EXP;
				bye_ov->_comp_type = OP_AI_HELLO;
				bye_ov->_ai_target_obj = ev.target_id;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &bye_ov->_over);
			}
			break;
			case EV_NPC_DIST:
			{
				OVER_EXP* bye_ov = new OVER_EXP;
				bye_ov->_comp_type = OP_AI_DIST;
				bye_ov->_ai_target_obj = ev.target_id;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &bye_ov->_over);
			}
			break;
			}
		}
		// 항상 1초씩 기다리기
		this_thread::sleep_for(1ms);
	}
}

int main()
{
	WSADATA WSAData;
	int err = WSAStartup(MAKEWORD(2, 2), &WSAData);

	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

	listen(g_s_socket, SOMAXCONN);

	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);

	InitializeNPC();

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;

	err = AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	thread timer_thread{ do_timer };

	timer_thread.join();
	for (auto& th : worker_threads)
		th.join();

	closesocket(g_s_socket);
	WSACleanup();
}
