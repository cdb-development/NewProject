#include "pch.h"
#include "Memory.h"
#include "Task.h"
#include "Core.h"
#include "Init.h"
#include "Display.h"
#include "Engine.h"
std::shared_ptr<Engine> EngineInstance;
std::string ProcessName;

void main()
{
	bool gamefound = true;
	ProcessName = "DeadByDaylight-Win64-Shipping.exe";
	if (MemoryManager.Init("DeadByDaylight-Win64-Shipping.exe"))
	{
		ProcessName = "DeadByDaylight-Win64-Shipping.exe";
	}
	else if (MemoryManager.Init("DeadByDaylight-EGS-Shipping.exe"))
	{
		ProcessName = "DeadByDaylight-EGS-Shipping.exe";

	}
	else
	{
		printf("Failed to find game\n");
		return;
	}
	MemoryManager.FixCr3();

	uint64_t base = MemoryManager.GetBaseAddress(ProcessName);
	uint64_t size = MemoryManager.GetBaseSize(ProcessName);
	EngineInstance = std::make_shared<Engine>();
	EngineInstance->Cache();

	

	//uint64_t persistentlevel = 0x190;
	//persistentlevel = MemoryManager.Read<uint64_t>(gobjects + gameinstance);
}
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	InputWndProc(hWnd, message, wParam, lParam);
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, LIT("CONIN$"), LIT("r"), stdin);
	freopen_s(&fDummy, LIT("CONOUT$"), LIT("w"), stderr);
	freopen_s(&fDummy, LIT("CONOUT$"), LIT("w"), stdout);
	printf(LIT("Debugging Window:\n"));

	main();
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"Display Framework";
	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(WS_EX_APPWINDOW, wc.lpszClassName, L"Display Framework",
		WS_POPUP,
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return -1;


	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);

	ShowWindow(hWnd, nCmdShow);

	InitD2D(hWnd);
	CreateDisplay();
	MSG msg;
	SetProcessDPIAware();
	SetInput();
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
		}
		RenderFrame();
	}
	CleanD2D();
	return msg.wParam;
}
