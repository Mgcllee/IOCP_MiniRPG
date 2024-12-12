#pragma once
#include <thread>
#include "NetworkSetting.h"
#include "UISetting.h"

int main() {
	thread network_thread(NETWORK_SETTING::connect_server, new NETWORK_SETTING("127.0.0.1"));
	thread ui_thread(UI_SETTING::loop_client_ui_event, new UI_SETTING());
	
	network_thread.join();
	ui_thread.join();

	return 0;
}