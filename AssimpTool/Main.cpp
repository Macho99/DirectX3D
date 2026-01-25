#include "pch.h"
#include "Main.h"
#include "Engine/Game.h"
#include "SkyBoxDemo.h"
#include "AssimpTool.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	GameDesc desc;
	desc.appName = L"GameCoding";
	desc.hInstance = hInstance;
	desc.vsync = false;
	desc.hWnd = NULL;
	desc.sceneWidth = 800;
	desc.sceneHeight = 600;
	desc.clearColor = Color(0.0f, 0.0f, 0.0f, 1.f);
	desc.app = make_shared<AssimpTool>();

	GAME->Run(desc);

	return 0;
}