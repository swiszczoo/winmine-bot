#include "solver.h"

#include "tiles.h"

#include <memory>
#include <stdint.h>
#include <vector>

#define COMB(x) (comb & (1 << x))

int factorials[] = {
    1,
    1,
    1 * 2,
    1 * 2 * 3,
    1 * 2 * 3 * 4,
    1 * 2 * 3 * 4 * 5,
    1 * 2 * 3 * 4 * 5 * 6,
    1 * 2 * 3 * 4 * 5 * 6 * 7,
    1 * 2 * 3 * 4 * 5 * 6 * 7 * 8
};

typedef uint8_t comb_t;
static bool is_number_valid(signed char* playfield, int width, int height,
    int x, int y) {
    int bombs = 0;
    int bombsPossible = 0;
    for (int cy = y - 1; cy <= y + 1; cy++) {
        for (int cx = x - 1; cx <= x + 1; cx++) {
            if (cx < 0 || cy < 0 || cx >= width || cy >= height) {
                continue;
            }

            if (playfield[cy * width + cx] == TILE_FLAG) {
                bombs++;
                bombsPossible++;
            }
            if (playfield[cy * width + cx] == TILE_EMPTY) {
                bombsPossible++;
            }
        }
    }

    return playfield[y * width + x] >= bombs
        && playfield[y * width + x] <= bombsPossible;
}

static bool process_number_field(signed char* playfield, signed char* buffer,
    int width, int height, int x, int y, move_t& move) {
    std::vector<comb_t> validCombinations;

    memcpy(buffer, playfield, width * height);

    for (int i = 0; i < 256; i++) {
        comb_t comb = (comb_t)i;

        // Check if number of bombs is ok
        int j = 0;
        int bombs = 0;
        bool ok = true;
        for (int cy = y - 1; cy <= y + 1; cy++) {
            for (int cx = x - 1; cx <= x + 1; cx++) {
                if (cx == x && cy == y) continue;

                if ((cx < 0 || cy < 0 || cx >= width || cy >= height) && COMB(j)) {
                    ok = false;
                    break;
                }

                if (cx >= 0 && cy >= 0 && cx < width && cy < height) {
                    buffer[cy * width + cx] = playfield[cy * width + cx];

                    if (playfield[cy * width + cx] == TILE_FLAG && !COMB(j)) {
                        ok = false;
                        break;
                    }

                    if (playfield[cy * width + cx] >= 0 && COMB(j)) {
                        ok = false;
                        break;
                    }

                    if (COMB(j)) {
                        bombs++;
                        buffer[cy * width + cx] = TILE_FLAG;
                    }
                    else if (buffer[cy * width + cx] == TILE_EMPTY) {
                        buffer[cy * width + cx] = 0;
                    }
                }

                j++;
            }
        }

        if (bombs != playfield[y * width + x]) {
            ok = false;
        }

        if (!ok) {
            continue;
        }

        // Check surrounding numbers
        bool surroundingOk = true;
        j = 0;
        for (int by = y - 1; by <= y + 1; by++) {
            for (int bx = x - 1; bx <= x + 1; bx++) {
                if (bx == x && by == y) continue;
                if ((bx < 0 || by < 0 || bx >= width || by >= height)) {
                    j++;
                    continue;
                }

                if (COMB(j)) {
                    for (int cy = y - 1; cy <= y + 1; cy++) {
                        for (int cx = x - 1; cx <= x + 1; cx++) {
                            if ((cx < 0 || cy < 0 || cx >= width || cy >= height)) {
                                continue;
                            }

                            if (playfield[cy * width + cx] > 0) {
                                surroundingOk = surroundingOk
                                    && is_number_valid(buffer, width, height, cx, cy);
                            }
                        }
                    }
                }
                j++;
            }
        }

        if (surroundingOk) {
            validCombinations.push_back(comb);
        }
    }

    comb_t openMask = 0xFF;
    comb_t bombMask = 0xFF;

    for (comb_t comb : validCombinations) {
        bombMask &= comb;
        openMask &= ~comb;
    }

    int j = 0;
    for (int cy = y - 1; cy <= y + 1; cy++) {
        for (int cx = x - 1; cx <= x + 1; cx++) {
            if (cx == x && cy == y) continue;

            if ((cx < 0 || cy < 0 || cx >= width || cy >= height)) {
                j++;
                continue;
            }

            if (playfield[cy * width + cx] == TILE_EMPTY)
            {
                if (openMask & (1 << j)) {
                    move.certain = true;
                    move.bomb = false;
                    move.x = cx;
                    move.y = cy;
                    return true;
                }
                if (bombMask & (1 << j)) {
                    move.certain = true;
                    move.bomb = true;
                    move.x = cx;
                    move.y = cy;
                    return true;
                }
            }

            j++;
        }
    }

    return false;
}

move_t get_next_move(signed char* playfield, int width, int height) {
    move_t ret = { 0 };
    signed char* buffer = new signed char[width * height];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (playfield[y * width + x] > 0) {
                if (process_number_field(playfield, buffer,
                    width, height, x, y, ret)) {
                    delete[] buffer;
                    return ret;
                }
            }
        }
    }

    delete[] buffer;
    return ret;
}

move_t draw_move(signed char* playfield, int width, 
    int height, std::mt19937_64& generator) 
{
    int bestProb = 1000;
    std::vector<std::vector<std::pair<int, int>>> possibilites;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (playfield[y * width + x] > 0) {
                int bombs = 0;
                int closed = 0;
                std::vector<std::pair<int, int>> fields;

                for (int cy = y - 1; cy <= y + 1; cy++) {
                    for (int cx = x - 1; cx <= x + 1; cx++) {
                        if (x == cx && y == cy) continue;
                        if (cx >= 0 && cy >= 0 && cx < width && cy < height) {
                            if (playfield[cy * width + cx] == TILE_FLAG) {
                                bombs++;
                            }
                            else if (playfield[cy * width + cx] == TILE_EMPTY) {
                                closed++;
                                fields.push_back(std::make_pair(cx, cy));
                            }
                        }
                    }
                }

                if (closed == 0) continue;

                bombs = playfield[y * width + x] - bombs;

                int binomCoeff = factorials[closed] 
                    / (factorials[bombs] * factorials[closed - bombs]);

                if (binomCoeff == bestProb) {
                    possibilites.push_back(fields);
                }
                else if (binomCoeff < bestProb) {
                    possibilites.clear();
                    possibilites.push_back(fields);
                    bestProb = binomCoeff;
                }
            }
        }
    }

    if (possibilites.empty()) {
        move_t mt;
        mt.bomb = false;
        mt.certain = false;
        mt.x = 0;
        mt.y = 0;
        return mt;
    }

    std::uniform_int_distribution<int> distr(0, possibilites.size() - 1);
    int pos = distr(generator);
    auto coords = possibilites[pos];

    std::uniform_int_distribution<int> distr2(0, coords.size() - 1);
    auto coord = coords[distr2(generator)];

    move_t mt;
    mt.bomb = false;
    mt.certain = true;
    mt.x = coord.first;
    mt.y = coord.second;
    return mt;
}
