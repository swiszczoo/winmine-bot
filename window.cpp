#include "window.h"

#include "tiles.h"

#include <Windows.h>

HWND find_minesweeper_window(bool bring_to_front) {
	HWND h_minesweep = FindWindowA("Minesweeper", NULL);

	if (h_minesweep) {
		RECT window_rect;
		GetWindowRect(h_minesweep, &window_rect);

		g_windowX = window_rect.left;
		g_windowY = window_rect.top;
		g_windowWidth = window_rect.right - window_rect.left;
		g_windowHeight = window_rect.bottom - window_rect.top;

		if (bring_to_front) {
			SendMessage(h_minesweep, WM_SYSCOMMAND, SC_RESTORE, 0);
			BringWindowToTop(h_minesweep);
			SetFocus(h_minesweep);
			SetActiveWindow(h_minesweep);
			SetForegroundWindow(h_minesweep);
		}

		return h_minesweep;
	}
	else {
		return NULL;
	}
}

std::pair<int, int> window_get_board_size() {
	return std::make_pair(
		(int)round((g_windowWidth - 26) / 16.f), 
		(int)round((g_windowHeight - 123) / 16.f)
	);
}

void window_click_lmb(int x, int y) {
	SetCursorPos(x, y);

	INPUT ev = { 0 };
	ev.mi.dx = x;
	ev.mi.dy = y;
	ev.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;
	SendInput(1, &ev, sizeof(ev));

	//Sleep(30);

	ev.mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;
	SendInput(1, &ev, sizeof(ev));
}

void window_click_rmb(int x, int y) {
	SetCursorPos(x, y);

	INPUT ev = { 0 };
	ev.mi.dx = x;
	ev.mi.dy = y;
	ev.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	SendInput(1, &ev, sizeof(ev));

	//Sleep(30);

	ev.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	SendInput(1, &ev, sizeof(ev));
}

void window_start_new_game() {
	static const int SMILEY_OFFSET_Y = 85;

	int mouse_x = g_windowX + g_windowWidth / 2;
	int mouse_y = g_windowY + SMILEY_OFFSET_Y;

	window_click_lmb(mouse_x, mouse_y);
}

int window_process_tile(bmp_pixel* img, int x, int y, int width) {
	static const int TILE_OX = 9;
	static const int TILE_OY = 3;
	static const int TILE_FX = 8;
	static const int TILE_FY = 3;
	static const int FRAME_X = 15;
	static const int FRAME_Y = 112;
	static const int TILE_SIZE = 16;

	int frameoffset_x = FRAME_X + x * TILE_SIZE;
	int frameoffset_y = FRAME_Y + y * TILE_SIZE;
	int flagoffset_x = FRAME_X + x * TILE_SIZE + TILE_FX;
	int flagoffset_y = FRAME_Y + y * TILE_SIZE + TILE_FY;
	int offset_x = FRAME_X + x * TILE_SIZE + TILE_OX;
	int offset_y = FRAME_Y + y * TILE_SIZE + TILE_OY;

	bmp_pixel f_pix = img[frameoffset_y * width + frameoffset_x];
	bmp_pixel d_pix = img[offset_y * width + offset_x];
	bmp_pixel l_pix = img[flagoffset_y * width + flagoffset_x];

	if (f_pix.r == 255 && f_pix.g == 255 && f_pix.b == 255) {
		// This tile is not opened
		if (l_pix.r == 255 && l_pix.g == 0 && l_pix.b == 0) 
			return TILE_FLAG;

		if (l_pix.r == 0 && l_pix.g == 0 && l_pix.b == 0) 
			return TILE_QUESTION;

		return TILE_EMPTY;
	}
	
	if (d_pix.r == 192 && d_pix.g == 192 && d_pix.b == 192)
		return 0;

	if (d_pix.r == 0 && d_pix.g == 0 && d_pix.b == 255)
		return 1;

	if (d_pix.r == 0 && d_pix.g == 128 && d_pix.b == 0)
		return 2;

	if (d_pix.r == 255 && d_pix.g == 0 && d_pix.b == 0)
	{
		if(l_pix.r == 255 && l_pix.g == 0 && l_pix.b == 0)
			return 3;
		return TILE_BOMB;
	}

	if (d_pix.r == 0 && d_pix.g == 0 && d_pix.b == 128)
		return 4;

	if (d_pix.r == 128 && d_pix.g == 0 && d_pix.b == 0)
		return 5;

	if (d_pix.r == 0 && d_pix.g == 128 && d_pix.b == 128)
		return 6;

	if (d_pix.r == 0 && d_pix.g == 0 && d_pix.b == 0)
		return 7;

	if (d_pix.r == 128 && d_pix.g == 128 && d_pix.b == 128)
		return 8;

	return 0;
}

void window_capture_playfield(HWND h_window, signed char* playfield) {
	HDC h_msdc = GetDC(NULL);
	HDC h_memdc = CreateCompatibleDC(h_msdc);
	HBITMAP h_bmp = CreateCompatibleBitmap(h_msdc, g_windowWidth, g_windowHeight);
	HBITMAP h_oldbmp = (HBITMAP)SelectObject(h_memdc, h_bmp);

	int result = BitBlt(h_memdc, 0, 0, g_windowWidth, 
		g_windowHeight, h_msdc, g_windowX, g_windowY, SRCCOPY);

	BITMAPINFO binfo = { 0 };
	binfo.bmiHeader.biSize = sizeof(binfo.bmiHeader);
	int res = GetDIBits(h_memdc, h_bmp, 0, 0, NULL, &binfo, DIB_RGB_COLORS);

	binfo.bmiHeader.biBitCount = 32;
	binfo.bmiHeader.biCompression = BI_RGB;
	binfo.bmiHeader.biHeight = -binfo.bmiHeader.biHeight;

	bmp_pixel* pixels = new bmp_pixel[binfo.bmiHeader.biWidth * -binfo.bmiHeader.biHeight];

	if (GetDIBits(h_memdc, h_bmp, 0, binfo.bmiHeader.biHeight, 
		pixels, &binfo, DIB_RGB_COLORS)) {

		auto size = window_get_board_size();

		for (int y = 0; y < size.second; y++) {
			for (int x = 0; x < size.first; x++) {
				playfield[y * size.first + x] = window_process_tile(pixels, x, y, binfo.bmiHeader.biWidth);
			}
		}
	}
	
	SelectObject(h_memdc, h_oldbmp);
	DeleteDC(h_memdc);
	ReleaseDC(NULL, h_msdc);
	DeleteObject(h_bmp);

	delete[] pixels;
}

static std::pair<int, int> window_get_field_position(int x, int y) {
	static const int TILE_SIZE = 16;
	static const int FRAME_X = 15;
	static const int FRAME_Y = 112;

	return std::make_pair(
		g_windowX + FRAME_X + x * TILE_SIZE + TILE_SIZE / 2,
		g_windowY + FRAME_Y + y * TILE_SIZE + TILE_SIZE / 2
	);
}

void window_open_field(int x, int y) {
	auto wher = window_get_field_position(x, y);
	window_click_lmb(wher.first, wher.second);
}

void window_flag_field(int x, int y) {
	auto wher = window_get_field_position(x, y);
	window_click_rmb(wher.first, wher.second);
}
