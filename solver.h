#pragma once
#include <random>

struct move_t {
    bool certain;
    bool bomb;
    int x;
    int y;
};

move_t get_next_move(signed char* playfield, int width, int height);
move_t draw_move(signed char* playfield, int width, 
    int height, std::mt19937_64& generator);
