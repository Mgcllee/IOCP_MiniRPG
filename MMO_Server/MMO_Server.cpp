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

HANDLE h_iocp;
SOCKET g_s_socket, g_c_socket;

char _send_buf[BUF_SIZE];
WSABUF _wsabuf;
WSAOVERLAPPED _over{};

enum TYPE { ACCEPT, RECV };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	TYPE c_type;
	int _ai_target_obj;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		c_type = RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	//OVER_EXP(char* packet)
	//{
	//	_wsabuf.len = packet[0];
	//	_wsabuf.buf = _send_buf;
	//	ZeroMemory(&_over, sizeof(_over));
	//	// c_type = OP_SEND;
	//	memcpy(_send_buf, packet, packet[0]);
	//}
};

OVER_EXP g_a_over;

void do_recv()
{
	DWORD recv_flag = 0;
	memset(&_over, 0, sizeof(_over));
	_wsabuf.len = BUF_SIZE;
	_wsabuf.buf = _send_buf;
	WSARecv(g_c_socket, &_wsabuf, 1, 0, &recv_flag, &_over, 0);
}

void process_packet(int c_id, char* packet) {
	
	switch (packet[1])
	{
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		cout << p->name << endl;
	}
	break;
	case CS_MOVE:
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		cout << p->direction << endl;
	}
	break;
	}
}

void worker_thread(HANDLE h_iocp) {
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);

		// Recv를 해야하는데 비어있을 경우
		if ((0 == num_bytes) && (ex_over->c_type == RECV)) {
			continue;
		}

		switch (ex_over->c_type)
		{
		case ACCEPT:
		{
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket), h_iocp, 0, 0);
			do_recv();
			g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			cout << "Accept New Client" << endl;
		}
		break;
		case RECV:
		{
			int remain_data = num_bytes;
			char* p = _send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			do_recv();
		}
		break;
		}
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

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 0, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over.c_type = ACCEPT;

	err = AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);

	for (auto& th : worker_threads)
		th.join();

	closesocket(g_s_socket);
	WSACleanup();
}
