#include "pch.h"
#include "Game.h"
#include "IExecute.h"
#include "EditorManager.h"

int Game::pendingWidth = 0;
int Game::pendingHeight = 0;

WPARAM Game::Run(GameDesc& desc)
{
	desc.isEditor = true;
    curSceneWidth = desc.sceneWidth;
    curSceneHeight = desc.sceneHeight;

	_desc = desc;
	assert(_desc.app != nullptr);

	// 1) 윈도우 창 정보 등록
	MyRegisterClass();

	// 2) 윈도우 창 생성
	if (!InitInstance(SW_SHOWNORMAL))
		return FALSE;
		
	GRAPHICS->Init(_desc.hWnd);
	TIME->Init();
	INPUT->Init(_desc.hWnd);
	EDITOR->Init();
	RESOURCES->Init();
	
	_desc.app->Init();
	SCENE->Init();

	MSG msg = { 0 };

	GRAPHICS->OnSize(true);

	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

        if (pendingWidth != 0 || pendingHeight != 0)
        {
			if (_desc.width != pendingWidth || _desc.height != pendingHeight)
			{
				_desc.width = pendingWidth;
				_desc.height = pendingHeight;
				GRAPHICS->OnSize(false);
			}
            pendingWidth = 0;
            pendingHeight = 0;
        }
		else if (curSceneWidth != _desc.sceneWidth || curSceneHeight != _desc.sceneHeight)
		{
			curSceneWidth = _desc.sceneWidth;
			curSceneHeight = _desc.sceneHeight;
			GRAPHICS->OnSize(false);
		}

		Update();
	}
	OutputDebugStringW(L"==============RENDER============\n");
	GET_SINGLE(RenderManager)->OnDestroy();
	OutputDebugStringW(L"==============SHADER============\n");
	ShaderManager::OnDestroy();
	OutputDebugStringW(L"==============SCENE============\n");
	SCENE->OnDestroy();
	OutputDebugStringW(L"==============RESOURCES============\n");
    RESOURCES->OnDestroy();
	OutputDebugStringW(L"==============GUI============\n");
	EDITOR->OnDestroy();
	OutputDebugStringW(L"==============GRAPHICS============\n");
    GRAPHICS->OnDestroy();
	return msg.wParam;
}


ATOM Game::MyRegisterClass()
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = _desc.hInstance;
	wcex.hIcon = ::LoadIcon(NULL, IDI_WINLOGO);
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _desc.appName.c_str();
	wcex.hIconSm = wcex.hIcon;

	return RegisterClassExW(&wcex);
}

BOOL Game::InitInstance(int cmdShow)
{
	RECT windowRect = { 0, 0, _desc.width, _desc.height };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

	_desc.hWnd = CreateWindowW(_desc.appName.c_str(), _desc.appName.c_str(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, _desc.hInstance, nullptr);

	if (!_desc.hWnd)
		return FALSE;

	::ShowWindow(_desc.hWnd, cmdShow);
	::UpdateWindow(_desc.hWnd);

	return TRUE;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Game::WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(handle, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			pendingWidth = LOWORD(lParam);
			pendingHeight = HIWORD(lParam);
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return ::DefWindowProc(handle, message, wParam, lParam);
	}
}

void Game::Update()
{
	TIME->Update();
	INPUT->Update();

	ShowFps();

	GRAPHICS->RenderBegin();

	SCENE->Update();
	GRAPHICS->SetBackBufferRenderTarget();
	EDITOR->Update();

	_desc.app->Update();
	_desc.app->Render();

	EDITOR->Render();
	GRAPHICS->RenderEnd();

}

void Game::ShowFps()
{
	uint32 fps = GET_SINGLE(TimeManager)->GetFps();

	WCHAR text[100] = L"";
	int len = ::wsprintf(text, L"FPS: %d ", fps);
	if(fps != 0)
		swprintf(text + len, 100 - len, L"Frame Time: %.2fms", 1000.f / fps);

	::SetWindowText(_desc.hWnd, text);
}

