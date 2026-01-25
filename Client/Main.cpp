#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include "pch.h"
#include "Main.h"
#include "Engine/Game.h"
#include "28. ParticleDemo.h"
#include "27. BillboardDemo.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//_CrtSetBreakAlloc(207);
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	GameDesc desc;
	desc.appName = L"GameCoding";
	desc.hInstance = hInstance;
	desc.vsync = false;
	desc.hWnd = NULL;
	desc.width = 1920;
	desc.height = 1080;
	desc.clearColor = Color(0.0f, 0.0f, 0.0f, 1.f);
	desc.app = make_shared<BillboardDemo>();

	GAME->Run(desc);

	//_CrtDumpMemoryLeaks();
	return 0;
}