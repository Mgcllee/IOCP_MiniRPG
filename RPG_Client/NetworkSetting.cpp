#pragma once

#include "NetworkSetting.h"

bool NETWORK_SETTING::connect_server() {
	cout << "working network module\n";

	sf::Socket::Status status = server_socket.connect(server_addr, PORT_NUM);
	server_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		cout << "Unable to connect to server\n";
		return false;
	}

	

	return true;
}
