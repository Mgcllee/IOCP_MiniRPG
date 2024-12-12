#pragma once

#include <SFML/Network.hpp>

#include "stdafx.h"

class NETWORK_SETTING {
	string server_addr;
	sf::TcpSocket server_socket;
public:
	NETWORK_SETTING(string in_server_addr)
		: server_addr(in_server_addr) {

	}

	bool connect_server();
};