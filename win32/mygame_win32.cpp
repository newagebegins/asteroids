#include <windows.h>
#include <stdio.h>
#include "gl3w.h"
#include "mygame.h"

static void debugLog(char *format, ...) {
	va_list argptr;
	va_start(argptr, format);
	char str[1024];
	vsprintf_s(str, format, argptr);
	va_end(argptr);
	OutputDebugString(str);
}

void win32PlatformLog(const char *string) {
	debugLog("%s", string);
}

static HDC hDC;

static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		PIXELFORMATDESCRIPTOR pfd = { 0 };
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 32;

		hDC = GetDC(hWnd);
		int pixelFormat = ChoosePixelFormat(hDC, &pfd);
		SetPixelFormat(hDC, pixelFormat, &pfd);

		HGLRC hglrc = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hglrc);
		gl3wInit();

		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	case WM_SIZE: {
		float windowWidth = LOWORD(lParam);
		float windowHeight = HIWORD(lParam);
		setViewport(windowWidth, windowHeight);
		break;
	}
	default: {
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	}
	return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE /* hPrevInstance */,
	LPSTR /* lpCmdLine */,
	int nCmdShow) {
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "MyGameWindowClass";

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, "RegisterClass failed!", NULL, NULL);
		return 1;
	}

	RECT clientRect = { 0, 0, 960, 540 };
	DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	AdjustWindowRect(&clientRect, windowStyle, NULL);
	HWND hWnd = CreateWindowEx(
		NULL,
		wc.lpszClassName,
		"MyGame",
		windowStyle,
		300, 0,
		clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hWnd) {
		MessageBox(NULL, "CreateWindowEx failed!", NULL, NULL);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	platformLog = &win32PlatformLog;

	if (!initGame()) {
		return 1;
	}

	LARGE_INTEGER counterFrequency = { 0 };
	QueryPerformanceFrequency(&counterFrequency);
	LARGE_INTEGER counter = { 0 };
	QueryPerformanceCounter(&counter);
	LARGE_INTEGER prevCounter = { 0 };
	float dt = 0.0f;
	float targetDt = 1.0f / 60.0f;

	bool gameIsRunning = true;
	while (gameIsRunning) {
		prevCounter = counter;
		QueryPerformanceCounter(&counter);
		dt = (float)(counter.QuadPart - prevCounter.QuadPart) / (float)counterFrequency.QuadPart;
		if (dt > targetDt) {
			dt = targetDt;
		}

		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			switch (msg.message) {
			case WM_QUIT: {
				gameIsRunning = false;
				break;
			}
			case WM_KEYDOWN:
			case WM_KEYUP: {
				bool isDown = ((msg.lParam & (1 << 31)) == 0);
				switch (msg.wParam) {
				case VK_LEFT: {
					g_input.leftButton.isDown = isDown;
					break;
				}
				case VK_RIGHT: {
					g_input.rightButton.isDown = isDown;
					break;
				}
				case VK_UP: {
					g_input.forwardButton.isDown = isDown;
					break;
				}
				case VK_SPACE: {
					g_input.fireButton.isDown = isDown;
					break;
				}
				case VK_ESCAPE: {
					gameIsRunning = false;
					break;
				}
				}
				break;
			}
			default: {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			}
			}
		}

		float touches[] = { -1, -1, -1, -1 };
		int lButtonDown = GetKeyState(VK_LBUTTON) & (1 << 15);

		if (lButtonDown) {
			POINT mouseP;
			GetCursorPos(&mouseP);
			ScreenToClient(hWnd, &mouseP);
			touches[0] = (float)mouseP.x;
			touches[1] = (float)mouseP.y;
		}

		gameUpdateAndRender(dt, touches);
		SwapBuffers(hDC);
	}

	return 0;
}
