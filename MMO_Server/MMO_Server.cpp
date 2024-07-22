#pragma once
#include "stdafx.h"
#include "DatabaseMgr.h"

HANDLE h_iocp; // IOCP Handle 객체
SOCKET g_s_socket, g_c_socket;

array<SESSION, MAX_USER + MAX_NPC> clients;
OVER_EXP g_a_over;

std::atomic_int new_client_id = -1;

void process_packet(int c_id, char* packet) 
{
	switch (packet[1])
	{
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		std::cout << "input Login name: " << p->name << endl;

		new_client_id++;
		// if (checking_DB(p->name, new_client_id)) {
		if (true) {
			// Success Login
			std::cout << "Success Login\n";
			strncpy_s(clients[new_client_id]._name, p->name, strlen(p->name));

			SC_LOGIN_OK_PACKET ok_p;
			ok_p.size = sizeof(ok_p);
			ok_p.type = SC_LOGIN_OK;
			clients[new_client_id].do_send(&ok_p);

			SC_LOGIN_INFO_PACKET info_p;
			info_p.size = sizeof(info_p);
			info_p.type = SC_LOGIN_INFO;
			info_p.id = new_client_id;
			info_p.x = 0;
			info_p.y = 0;
			strcpy_s(info_p.name, p->name);
			info_p.exp = 0;
			info_p.max_hp = 600;
			info_p.hp = 600;

			// 기존 클라와 신참 클라(본인)에게 신참 클라 정보 알려주기
			for (int i = 0; i <= new_client_id; ++i) {
				clients[i].do_send(&info_p);
			}

			SC_LOGIN_INFO_PACKET old_client;
			old_client.size = sizeof(old_client);
			old_client.type = SC_LOGIN_INFO;
			for (int i = 0; i < new_client_id; ++i) {
				old_client.id = i;
				old_client.x = clients[i].x;
				old_client.y = clients[i].y;
				strcpy_s(old_client.name, clients[i]._name);
				clients[new_client_id].do_send(&old_client);
			}
		}
		else {
			// Fail Login and try Logout
			std::cout << "Fail Loing\n";
			SC_LOGIN_FAIL_PACKET p;
			p.size = sizeof(p);
			p.type = SC_LOGIN_FAIL;
			clients[new_client_id].do_send(&p);
		}
	}
	break;
	case CS_MOVE:
	{

	}
	break;
	case CS_CHAT:
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);
		std::cout << p->mess << endl;

		SC_CHAT_PACKET sc_chat_packet;
		sc_chat_packet.size = sizeof(sc_chat_packet);
		sc_chat_packet.type = SC_CHAT;
		sc_chat_packet.id = c_id;
		strncpy_s(sc_chat_packet.mess, p->mess, sizeof(p->mess));
		for (SESSION& cl : clients) {
			if(0 != strcmp(cl._name, "empty"))
				cl.do_send(&sc_chat_packet);
		}
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
		
	}
	break;
	}
}

void worker_thread(HANDLE h_iocp) {

	/*
	GetQueuedCompletionStatus 함수에서 작업을 가져와서 진행하므로
	worker_thread에서 아래의 반복문을 무한으로 반복한다.
	*/

	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;

		// IOCP의 I/O Completion Queue에서 데이터가 입력될 때까지 [[무한 대기]] 합니다.
		BOOL ret
			= GetQueuedCompletionStatus(
				h_iocp,		// 대기중인 iocp 객체
				&num_bytes,	// 송/수신된 Byte 수
				&key,		// 
				&over,		// OVERLAPPED 객체 (WSAOVERLAPPED 구조체와 같다.)
				INFINITE	// 무한 대기
			);
		
		/*
		- GetQueuedCompletionStatus function() -
		1. [CompletionPort] : 대기를 수행할 IOCP 핸들
		2. [lpNumberOfBytesTransferred] : 송/수신된 Byte 수
		3. [lpCompletionKey] : 비동기 I/O 요청이 발생한 디바이스의 CompletionKey
		4. [lpOverlapped] : 비동기 호출시 전달한 Overlapped 구조체 주소
		5. [dwMilliseconds] : 대기를 수행할 시간(ms)
		
		반환 값이 true이면 성공, false이면 실패
		*/
		
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);

		if (FALSE == ret) {
			// Queue에 작업이 있으나, 유효하게 꺼내오지 못 하였다.
			// (작업이 없으면 GetQueuedCompletionStatus 함수 자체에서 대기함.
			// 즉, 작업이 있었기에 ret 변수로 값이 반환된 것.)

			if (ex_over->c_type == ACCEPT) std::cout << "Accept Error";
			else continue;
		}
		if ((0 == num_bytes) && (ex_over->c_type == RECV)) continue;

		// 정삭적으로 Queue에서 꺼내진 작업
		switch (ex_over->c_type)
		{
		case ACCEPT:
		{
			int new_c_id = get_player_number();
			if (new_c_id != -1) {
				clients[new_c_id].x = 0;
				clients[new_c_id].y = 0;
				clients[new_c_id]._id = new_c_id;
				clients[new_c_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket), h_iocp, new_c_id, 0);
				clients[new_c_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
				std::cout << "Accept New Client" << endl;
			}
			else {
				std::cout << "Accept Error" << endl;
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

	h_iocp
		= CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	/* 
	- CreateIoCompletionPort Function() -
		1. [FileHandle] : IOCP와 연결할 디바이스 핸들 or INVALID_HANDLE_VALUE
		2. [ExistingCompletionPort] : FileHandle과 연결할 IOCP 핸들을 전달,
								INVALID_HANDLE_VALUE 값이라면 NULL 전달
		3. [CompletionKey] : 연결할 디바이스 장비에 고유 키 등록, IO Queue에서 해당 키로 구분
		4. [NumberOfConcurrentThreads] : IOCP 핸들을 생성할 때만 사용.

		  i) 2번 변수가 NULL이면 새로운 IOCP 객체를 반환
		 ii) 1번과 2번 변수가 유효한 값이 있는 경우, 연동된 IOCP 핸들 반환
		iii) 반환 값이 NULL 이라면 GetLastError() 함수로 오류 확인 필요
	*/ 

	CreateIoCompletionPort(
		// g_s_socket 객체를 h_iocp와 연결.
		// 키 값으로는 9999 전달.
		reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0
	);
	
	
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
