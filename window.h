#pragma once
#include "main.h"

#include <utility>
#include <Windows.h>

struct bmp_pixel {
    unsigned char b, g, r, a;
};

HWND find_minesweeper_window(bool bring_to_front);

std::pair<int, int> window_get_board_size();

void window_click_lmb(int x, int y);
void window_click_rmb(int x, int y);
void window_start_new_game();

void window_capture_playfield(HWND h_window, signed char* playfield);

void window_open_field(int x, int y);
void window_flag_field(int x, int y);
