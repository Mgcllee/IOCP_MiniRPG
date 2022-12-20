#include "stdafx.h"

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(SC_MOVE_OBJECT_PACKET);
		p.type = SC_MOVE_OBJECT;
		p.x = clients[c_id].x;
		p.y = clients[c_id].y;
		do_send(&p);
}
