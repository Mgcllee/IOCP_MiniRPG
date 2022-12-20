#pragma once

#include <iostream>
#include <windows.h>  
#include <string>
#include <sqlext.h>

void show_error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
bool checking_DB(short p_id, short c_id);
bool write_DB(int game_id, int pos_x, int pos_y);
