#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <chrono>
#include <vector>
#include <algorithm>
#include <ncursesw/ncurses.h>

int nScreenWidth = 120;
int nScreenHeight = 40;
int nMapHeight = 16;
int nMapWidth = 16;

float fPlayerX = 14.0f;
float fPlayerY = 12.0f;
float fPlayerA = 0.0f;
float fFOV = 3.14159f / 4.0f;
float fDepth = 16.0f;
float fSpeed = 150.0f;

int main(void) {
	// ncurses initialisation
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	nodelay(stdscr, true); // don't halt while waiting for input
	clear(); // clear screen before starting
	int termWidth;
	int termHeight;
	getmaxyx(stdscr, termHeight, termWidth); // get current terminal dimensions

	auto *screen = new wchar_t[nScreenWidth*nScreenHeight]; // unicode screen buffer

	std::wstring map;
	map += L"#########.......";
	map += L"#...............";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	auto tp1 = std::chrono::steady_clock::now();
	auto tp2 = std::chrono::steady_clock::now();

	bool shouldRun = true;
	// Game room
	while(shouldRun) {
		tp2 = std::chrono::steady_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count(); // in milliseconds
		// resize terminal
		getmaxyx(stdscr, termHeight, termWidth);
		while (termHeight != nScreenHeight || termWidth != nScreenWidth) {
			erase();
			std::ostringstream output;
			output << "Terminal size is incorrect. Please resize your terminal to " << nScreenWidth
				<< "*" << nScreenHeight << ". Current dimensions are " << termWidth << "*" << termHeight;
			mvaddstr(0, 0, output.str().c_str());
			refresh();
			getmaxyx(stdscr, termHeight, termWidth);
		}

		char input = getch();

		switch (input) {
			// CCW rotation
			case 'a':
				fPlayerA -= fSpeed * fElapsedTime;
				break;
				// CW rotation
			case 'd':
				fPlayerA += fSpeed * fElapsedTime;
				break;
				// move forward
			case 'w':
				fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;

				// if player in wall undo movement
				if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
					fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
					fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
				}

				break;
				// move backwards
			case 's':
				fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;

				// if player in wall undo movement
				if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
					fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
					fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
				}

				break;
				// exit
			case 'q':
				shouldRun = false;
				break;
			default:
				break;
		}

		// raycast for each column
		for (int x=0; x < nScreenWidth; x++) {
			// for each column, calculate the projected ray angle into world space
			// first part: finds the starting angle for FOV. second part: splits FOV into columns
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / float(nScreenWidth)) * fFOV;

			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundary = false;

			// create unit vector for ray in player space
			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			// Keep incrementing raycast length until reach a wall
			while (!bHitWall && fDistanceToWall < fDepth) {
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// check if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					bHitWall = true;
					fDistanceToWall = fDepth;
				} else {
					// Ray is inbounds so check if ray has hit wall
					// map indexing converts 2d coord to 1d for array
					if (map[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true;

						std::vector<std::pair<float, float>> p; // distance, dot product

						// iterate through 4 corners
						for(int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								// vector from corner to player
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}
						}
						// sort pairs from closest to farthest
						// lambda function returns which of the 2 points is closest
						std::sort(p.begin(), p.end(), [](const std::pair<float, float> &left, const std::pair<float, float> &right) {return left.first < right.first;});

						// if angle between corner ray and player ray then boundary
						float fBound = 0.01f;
						if (acos(p.at(0).second) < fBound)
							bBoundary = true;
						else if (acos(p.at(1).second) < fBound)
							bBoundary = true;
						//else if (acos(p.at(2).second) < fBound)
						//	bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			// midpoint of screen - screenHeight * distanceToWall
			// decreasing y increases y for viewer
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';

			// set character to be brighter if closer and vice versa
			if (fDistanceToWall <= fDepth / 4.0f)
				nShade = 0x2588; // █ - very close
			else if (fDistanceToWall < fDepth / 3.0f)
				nShade = 0x2593; // ▓
			else if (fDistanceToWall < fDepth / 2.0f)
				nShade = 0x2592; // ▒
			else if (fDistanceToWall < fDepth)
				nShade = 0x2591; // ░
			else
				nShade = ' ';	 // out of render distance

			if (bBoundary)
				nShade = ' ';    // black out boundaries

			// iterate through rows
			for (int y = 0; y < nScreenHeight; y++) {
				// if above wall i.e sky
				if (y <= nCeiling) {
					screen[y * nScreenWidth + x] = ' ';
				}
				// if lower than ceiling and higher than floor i.e wall
				else if (y > nCeiling && y <= nFloor) {
					screen[y * nScreenWidth + x] = nShade;
				}
				// if not ceiling or wall i.e floor
				else {
					// shade floor based on distance
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)
						nShade = '#';
					else if (b < 0.5)
						nShade = 'x';
					else if (b < 0.75)
						nShade = '.';
					else if (b < 0.9)
						nShade = '-';
					else
						nShade = ' ';
					screen[y * nScreenWidth + x] = nShade;
				}
			}
		}

		// display stats
		std::wstringstream stats;
		stats << "X=" << fPlayerX << " Y=" << fPlayerY << " A=" << fPlayerA << " FPS" << 1.0f / fElapsedTime << std::setw(6) << std::setprecision(2);

		// draw map
		for (int nx = 0; nx < nMapWidth; nx++) {
			for (int ny = 0; ny < nMapHeight; ny++) {
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}

		// player marker on map
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		// draw to terminal
		mvaddwstr(0, 0, screen);
		mvaddwstr(0, 0, stats.str().c_str());
		refresh();
	}

	endwin(); // restore terminal settings
	return 0;
}

