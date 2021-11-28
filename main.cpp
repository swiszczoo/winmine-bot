#include "main.h"
#include "solver.h"
#include "tiles.h"
#include "window.h"

#include <random>
#include <string>

#include <Windows.h>

using namespace std;

int g_windowX;
int g_windowY;
int g_windowWidth;
int g_windowHeight;

void debug_draw_playfield(signed char* playfield, int width, int height);

int main()
{
	SetProcessDPIAware();

	cout << "Searching for Minesweeper window..." << endl;
	if (!find_minesweeper_window(true)) {
		cout << "Minesweeper not found! Waiting for it to appear on the screen" << endl;

		do {
			Sleep(1000);
		} while (!find_minesweeper_window(true));
	}

	cout << "Minesweeper found!" << endl;

	Sleep(1000);

	mt19937_64 gen(time(0));

	while (true)
	{
		cout << "Press any key to start a new game...";
		string dummy;
		getline(cin, dummy);

		if (!find_minesweeper_window(true)) {
			break;
		}

		Sleep(250);

		HWND h_window = find_minesweeper_window(false);

		window_start_new_game();
		auto size = window_get_board_size();
		cout << "Started a new game on " << size.first << " x "
			<< size.second << " playfield" << endl;

		Sleep(50);

		signed char* pf = new signed char[size.first * size.second];

		while (true) {
			window_capture_playfield(h_window, pf);
			//debug_draw_playfield(pf, size.first, size.second);

			bool gameLost = false;
			bool gameWon = true;

			for (int y = 0; y < size.second; y++) {
				for (int x = 0; x < size.first; x++) {
					if (pf[y * size.first + x] == TILE_BOMB) {
						gameLost = true;
						gameWon = false;
					}
					if (pf[y * size.first + x] == TILE_EMPTY) {
						gameWon = false;
					}
				}
			}

			if (gameLost) {
				cout << "Oh no, I've probably lost the game..." << endl;
				break;
			}
			else if (gameWon) {
				cout << "ez game" << endl;
				break;
			}

			move_t nextMove = get_next_move(pf, size.first, size.second);
			if (nextMove.certain) {
				if (nextMove.bomb) {
					window_flag_field(nextMove.x, nextMove.y);
				}
				else {
					window_open_field(nextMove.x, nextMove.y);
				}
			}
			else {
				bool island = false;
				for (int y = 0; y < size.second; y++) {
					for (int x = 0; x < size.first; x++) {
						if (pf[y * size.first + x] == 0) {
							island = true;
						}
					}
				}

				if (!island) {
					uniform_int_distribution<int> distrX(0, size.first - 1);
					uniform_int_distribution<int> distrY(0, size.second - 1);

					window_open_field(distrX(gen), distrY(gen));
				}
				else {
					cout << "I'm not certain..." << endl;

					move_t guess = draw_move(pf, size.first, size.second, gen);
					if (guess.certain) {
						cout << guess.x << ' ' << guess.y << endl;
						if (guess.bomb) {
							window_flag_field(guess.x, guess.y);
						}
						else {
							window_open_field(guess.x, guess.y);
						}
					}
					else {
						cout << "Something weird happened! Make your move!" << endl;
						Sleep(10000);
					}
				}
			}

			Sleep(16);
		}

		delete[] pf;
	}

	return 0;
}

void debug_draw_playfield(signed char* playfield, int width, int height) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			signed char val = playfield[y * width + x];
			if (val >= 0) {
				cout << (int)val;
			}
			else if (val == TILE_EMPTY) {
				cout << '.';
			}
			else if (val == TILE_BOMB) {
				cout << 'B';
			}
			else if (val == TILE_FLAG) {
				cout << 'P';
			}
			else if (val == TILE_QUESTION) {
				cout << '?';
			}

			cout << ' ';
		}
		cout << endl << endl;
	}
}
