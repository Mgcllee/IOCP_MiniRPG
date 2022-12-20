#pragma once
#include "stdafx.h"

HANDLE h_iocp;
SOCKET g_s_socket, g_c_socket;

array<SESSION, MAX_USER + MAX_NPC> clients;
OVER_EXP g_a_over;

int get_player_number() {
	// User index 범위 내에서 name이 default값인 empty 로 되어 있는 것 찾기
	for (int i = 0; i < MAX_USER; ++i) {
		if (0 == strcmp(clients[i]._name, "empty")) {
			return i;
			break;
		}
		continue;
	}
	return -1;
}

void process_packet(int c_id, char* packet) 
{
	switch (packet[1])
	{
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		cout << "input Login name: " << p->name << endl;
		
		
		
		int new_client_id = get_player_number();
		if (-1 == new_client_id) {
			// Login Fail

		}
		else {
			strncpy_s(clients[new_client_id]._name, p->name, strlen(p->name));
		}
	}
	break;
	case CS_MOVE:
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		
		short x = clients[c_id].x;
		short y = clients[c_id].y;
		switch (p->direction)
		{
		case DIRECTION::UP:		--y;	break;
		case DIRECTION::RIGHT:	++x;	break;
		case DIRECTION::DOWN:	++y;	break;
		case DIRECTION::LEFT:	--x;	break;
		}

		if (/*Collision*/false) {
			
		}
		else {
			clients[c_id].x = x;
			clients[c_id].y = y;
			clients[c_id].send_move_packet(c_id);
		}
	}
	break;
	case CS_CHAT:
	{

	}
	break;
	case CS_ATTACK:
	{

	}
	break;
	case CS_TELEPORT:
	{

	}
	break;
	case CS_LOGOUT:
	{
		// Disconect Client
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

		// error 검출기
		if (FALSE == ret) {
			if (ex_over->c_type == ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				continue;
			}
		}
		if ((0 == num_bytes) && (ex_over->c_type == RECV)) {
			continue;
		}

		switch (ex_over->c_type)
		{
		case ACCEPT:
		{
			int new_c_id = 0;
			if (new_c_id != -1) {
				clients[new_c_id].x = 0;
				clients[new_c_id].y = 0;
				clients[new_c_id]._id = new_c_id;
				clients[new_c_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket), h_iocp, new_c_id, 0);
				clients[new_c_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
				cout << "Accept New Client" << endl;
			}
			else {
				cout << "Accept Error" << endl;
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
		}
		break;
		case RECV:
		{
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
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
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
