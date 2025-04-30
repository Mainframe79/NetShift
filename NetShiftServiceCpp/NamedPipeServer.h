#ifndef NAMED_PIPE_SERVER_H
#define NAMED_PIPE_SERVER_H

#include <windows.h>
#include "NetShiftService.h"

DWORD WINAPI PipeServerThread(LPVOID lpParam);

#endif // NAMED_PIPE_SERVER_H
