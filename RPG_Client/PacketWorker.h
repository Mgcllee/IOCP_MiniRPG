#pragma once

#include "NetworkSetting.h"

#include "../MMO_Server/protocol_2022.h"


class PACKET_WORKER : public NETWORK_SETTING {
private:

public:
	void send_packet(void* packet);
	void recv_packet(char* net_buf, size_t io_byte);
	void process_packet(char* ptr);

};