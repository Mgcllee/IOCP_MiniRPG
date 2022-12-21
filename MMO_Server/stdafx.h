#pragma once

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

enum TYPE		{ ACCEPT, RECV, SEND };
enum DIRECTION	{ UP, RIGHT, DOWN, LEFT };

class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	TYPE c_type = RECV;
	int _ai_target_obj;

	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		c_type = RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		c_type = SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};
class SESSION {
	OVER_EXP _recv_over;

public:
	SOCKET	_socket;
	int		_id;
	short	x, y;
	char	_name[NAME_SIZE];
	int		_prev_remain;

	SESSION() {
		_socket = 0;
		_id = -1;
		x = y = -9999;
		strncpy_s(_name, "empty", strlen("empty"));
		_prev_remain = 0;
	};

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag, &_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}

	void send_move_packet(int c_id);
};

extern array<SESSION, MAX_USER + MAX_NPC> clients;
extern OVER_EXP g_a_over;
