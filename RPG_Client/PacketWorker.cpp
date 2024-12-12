#pragma once

#include "PacketWorker.h"

void PACKET_WORKER::send_packet(void* packet) {
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	server_socket.send(packet, p[0], sent);
}

void PACKET_WORKER::recv_packet(char* net_buf, size_t io_byte) {
	char* ptr = net_buf;

	size_t in_packet_size = 0;
	size_t saved_packet_size = 0;
	char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) {
			in_packet_size = ptr[0];
		}

		if ((io_byte + saved_packet_size) >= in_packet_size) {
			memcpy((packet_buffer + saved_packet_size), ptr, (in_packet_size - saved_packet_size));

			process_packet(packet_buffer);

			ptr += (in_packet_size - saved_packet_size);
			io_byte -= (in_packet_size - saved_packet_size);

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

void PACKET_WORKER::process_packet(char* ptr) {
	switch (ptr[1]) {
		case SC_LOGIN_FAIL: {

			break;
		}
		case SC_LOGIN_OK: {
			main_page();
			break;
		}
		case SC_LOGIN_INFO: {

			break;
		}
		case SC_MOVE_OBJECT: {
			SC_MOVE_OBJECT_PACKET* p = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
			moveObj(players[p->id], p->x, p->y, p->direction, NULL);
			break;
		}
	}
}