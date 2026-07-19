
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#include "game.h"

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#define SCALE 4

static U32 *g_framebuffer;
static GLuint g_texture;
static int g_running = 1;

#define MAX_PLAYER_COUNT 4

static GameInput g_player_inputs[MAX_PLAYER_COUNT];

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_CLOSE:
		case WM_DESTROY:
			g_running = 0;
			break;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	WNDCLASSA window_class = {0};
	window_class.lpfnWndProc = wndproc;
	window_class.hInstance = hInstance;
	window_class.lpszClassName = "Pool Bomb";
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&window_class);

	RECT rc = {0};
	rc.top = 0;
	rc.left = 0;
	rc.right = FRAME_BUFFER_WIDTH * SCALE;
	rc.bottom = FRAME_BUFFER_HEIGHT * SCALE;
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowA(
		window_class.lpszClassName,
		window_class.lpszClassName,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

	HDC hdc = GetDC(hwnd);
	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	int pixel_format = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixel_format, &pfd);

	HGLRC glrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, glrc);

	uint64_t frame_index = 0;

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &g_texture);
	glBindTexture(GL_TEXTURE_2D, g_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	g_framebuffer = (uint32_t *)VirtualAlloc(0, FRAME_BUFFER_WIDTH * FRAME_BUFFER_WIDTH * 4, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers(hdc);
	ShowWindow(hwnd, SW_SHOW);

	GameInit();

	LARGE_INTEGER frequency = {0}, counter1 = {0};
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&counter1);

	F64 remaining_time = 0.0f;
	F64 update_delta = 1.0 / 60.0;

	while (g_running) {
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!g_running) break;

		LARGE_INTEGER counter2 = {0};
		QueryPerformanceCounter(&counter2);
		F64 delta = (F64)(counter2.QuadPart - counter1.QuadPart) / (F64)frequency.QuadPart;
		remaining_time += delta;
		counter1 = counter2;

		if (remaining_time >= update_delta) {

			{
				Bool up_button =	(GetAsyncKeyState('W') & 0x8000) != 0;
				Bool right_button = (GetAsyncKeyState('D') & 0x8000) != 0;
				Bool down_button =	(GetAsyncKeyState('S') & 0x8000) != 0;
				Bool left_button =	(GetAsyncKeyState('A') & 0x8000) != 0;

				GameInput *player_one_input = &g_player_inputs[0];

				player_one_input->previous_button_state = player_one_input->current_button_state;
				player_one_input->current_button_state = 0;
				player_one_input->current_button_state |= (BUTTON_UP * up_button);
				player_one_input->current_button_state |= (BUTTON_RIGHT * right_button);
				player_one_input->current_button_state |= (BUTTON_DOWN * down_button);
				player_one_input->current_button_state |= (BUTTON_LEFT * left_button);
			}

			if (remaining_time >= update_delta) {
				do {
					remaining_time -= update_delta;
					Bool should_render = remaining_time < update_delta;
					GameFrame(g_framebuffer, frame_index, g_player_inputs, 1, should_render);
					frame_index += 1;
				} while (remaining_time >= update_delta);
			}
		}

		glBindTexture(GL_TEXTURE_2D, g_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, g_framebuffer);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(-1,  1);
			glTexCoord2f(1, 0); glVertex2f( 1,  1);
			glTexCoord2f(1, 1); glVertex2f( 1, -1);
			glTexCoord2f(0, 1); glVertex2f(-1, -1);
		glEnd();

		SwapBuffers(hdc);
	}

	ExitProcess(0);
}
