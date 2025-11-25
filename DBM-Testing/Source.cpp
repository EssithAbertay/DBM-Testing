
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

#include "raylib.h"

 int x_size = 10;
 int y_size = 10;

int test_x = 10;

const int LIGHTNING_STEPS = 10;
int MAX_GRADIENT_LAPLACE_LOOPS = 50;

bool reached_edge = false;

int eta = 1;

//std::vector<Vector2> lightning_points;
//float potential_grid[y_size][x_size] =
//{
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//};
//
//float potential_grid_updates[y_size][x_size] =
//{
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,
//};

// 0 for air
// 1 for ground
// 2 for starting charge
// 3 for boundary

int starting_grid[10][10] =
{
	3,3,3,3,2,3,3,3,3,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	1,1,1,1,1,1,1,1,1,1,
};


struct candidate_cell
{
	int x, y, parent_x,parent_y;
	float potential, probability;
};

struct lightning_cell
{
	lightning_cell(int x, int y, int px, int py) : x(x), y(y), parent_x(px), parent_y(py){}

	int x, y, parent_x, parent_y;
};

std::vector<lightning_cell> lightning_points;

std::vector<std::vector<float>> potentials;

std::vector<std::vector<float>> new_potentials;

std::vector<std::vector<int>> starting;

void createStartingGrid()
{
	starting.clear();

	for (int i = 0; i < y_size; i++)
	{
		std::vector<int> row;

		for (int j = 0; j < x_size; j++)
		{
			if (i == 0)
			{
				row.push_back(3);
			}
			else if (i == y_size - 1)
			{
				row.push_back(1);
			}
			else if (j == 0 || j == x_size - 1)
			{
				row.push_back(3);
			}
			else
			{
				row.push_back(0);
			}
		}

		starting.push_back(row);
	}

	int starting_x = x_size / 2;

	starting[0][starting_x] = 2;
}

void initialiseGrid()
{
	lightning_points.clear();
	potentials.clear();
	new_potentials.clear();

	MAX_GRADIENT_LAPLACE_LOOPS = std::max(x_size * 1.5 + 20, double(30));
	reached_edge = false;


	createStartingGrid();

	for (int i = 0; i < y_size; i++)
	{
		std::vector<float> row;

		for (int j = 0; j < x_size; j++)
		{
			switch (starting[i][j])
			{
			case 0:
				row.push_back(0.5);
				break;
			case 1:
				row.push_back(1);
				break;
			case 2:
				row.push_back(0);
				break;
			case 3:
				row.push_back(0);
				break;
			default:
				break;
			}

		}

		potentials.push_back(row);
		new_potentials.push_back(row);
	}
}

void displayGrid()
{
	for (int i = 0;  i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
		//	std::cout << potential_grid[i][j] << ",";
		}
		std::cout << std::endl;
	}

	std::cout << std::endl << "---------------------" << std::endl << std::endl;
}

void displayGridColour()
{
	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			//switch (starting_grid[i][j])
			//{
			//case 0: // if air in starting we check if its still air or "lightning"
			////	if (potential_grid[i][j] != 0)
			//	{
			//		std::cout << "\033[36;46m" << "0" << "\033[0m"; // yellow
			//	}
			//	else
			//	{
			//		std::cout << "\033[33;43m" << "0" << "\033[0m"; // blue
			//	}
			//	break;
			//case 1: // ground
			//	std::cout << "\033[32;42m" << "0" << "\033[0m"; // green
			//	break;
			//case 2: // starting charge
			//	std::cout <<  "\033[31;41m" << "0" << "\033[0m"; // red
			//	break;
			//case 3: //boundary
			//	std::cout << "\033[37;47m" << "0" << "\033[0m"; // white
			//	break;
			//default:
			//	std::cout << "\033[0m" << "0" << "\033[0m"; // default
			//	break;
			//}


	
		}
		std::cout << std::endl;
	}
}

float calculateLaplace(int x, int y)
{
	float left = potentials[y][x - 1];
	float right = potentials[y][x + 1];
	float up = potentials[y + 1][x];
	float down = potentials[y - 1][x];

	float average = left + right + up + down;
	average /= 4;

	return average;
}

bool calculateGridStep()
{ 
	bool is_within_tolerance = false;

	const float tolerance = 0.005;

	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			if (potentials[i][j] == 0 || potentials[i][j] == 1) //skip anything with 0 or 1
			{
				continue;
			}

			float new_value  = calculateLaplace(j, i);

			float old_value = potentials[i][j];

			new_potentials[i][j] = new_value;
			
			if (abs(old_value - new_value) >= tolerance)
			{
				is_within_tolerance = true;
			}
		}
	}

	std::swap(potentials, new_potentials);

	return is_within_tolerance;
}

void selectLightningCell()
{
	std::vector<candidate_cell> candidates;

	// get all candidate cells
	bool is_ground_candidate_found = false;


	for (int i = 0; i < y_size; i++) 
	{
		for (int j = 0; j < x_size; j++)
		{
			if (potentials[i][j] == 0) //skip current lightning cells or boundaries
			{
				continue;
			}

			// check if cell has lightning on border

			for (int x = -1; x <= 1; x++)
			{
				for (int y = -1; y <= 1; y++)
				{
					if (i + x >= x_size || j + y >= y_size) continue;  // if checking cells that are out of bounds skip
					if (x == 0 && y == 0) continue;// skip middle cell

					if (j + y < 0) continue; //temp fix

					if (starting[i + x][j + y] == 3) continue; //skip boundaries 

					if (potentials[i + x][j + y] == 0) // if a surrounding cell is lightning then this is a candidate
					{
	
						if (potentials[i][j] == 1) // check if candidate ground is next to lightning
						{
						//	std::cout << "Candidate was ground!" << std::endl;
							is_ground_candidate_found = true;
							reached_edge = true;
						}

						candidate_cell temp;

						temp.potential = potentials[i][j];
						temp.x = j;
						temp.y = i;
						temp.parent_x = j + y;
						temp.parent_y = i + x;

						if (is_ground_candidate_found) 
						{
							candidates.clear();
							candidates.push_back(temp);
							break;
						}

						candidates.push_back(temp);

						x = 2; y = 2; // skip out the check cause we knw it's a candidate
					}
				}

			}
		}
	}


	// formula for probability
	// https://onlinelibrary.wiley.com/cms/asset/6dcd580c-06ae-421c-acf7-f5449d06a5e4/cav1760-math-0002.png

	float total_potential = 0;

	for (int i = 0; i < candidates.size(); i++)
	{
		total_potential += pow(candidates[i].potential, eta);
	}

	for (auto & x : candidates)
	{
		x.probability = (pow(x.potential,eta)) / total_potential;
	}

	// taken from cpp reference  https://en.cppreference.com/w/cpp/numeric/random/generate_canonical.html
	std::random_device rd;
	std::mt19937 gen(rd());
	float rnd = std::generate_canonical<float, 10>(gen);

	int chosen_candidate = 0;

	for (int i = 0; i < candidates.size(); i++) {
		if (rnd < candidates[i].probability)
		{
			chosen_candidate = i;
			break;
		}
		rnd -= candidates[i].probability;
	}

	candidate_cell chosen = candidates[chosen_candidate];

	lightning_points.push_back(lightning_cell(chosen.x, chosen.y, chosen.parent_x,chosen.parent_y));

	potentials[chosen.y][chosen.x] = 0;
	new_potentials[chosen.y][chosen.x] = 0;
}

void resetPotentialGrid()
{
	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			if (potentials[i][j] == 0 || potentials[i][j] == 1)
			{
				continue;
			}

			potentials[i][j] = 0.5;
		}
		
	}
}

void performLightningStep()
{
	bool is_within_tolerance = true;

	int loops = 0;

	while(is_within_tolerance)
	{
		is_within_tolerance = calculateGridStep();
		loops++;

		if (loops >= MAX_GRADIENT_LAPLACE_LOOPS)
		{
			is_within_tolerance = false;
		}
	}

    selectLightningCell();
	resetPotentialGrid();
}

void regen_lightning()
{
	initialiseGrid();

	//for (int i = 0; i < LIGHTNING_STEPS; i++)
	//{
	//	performLightningStep();
	//}

	while (!reached_edge)
	{
		performLightningStep();
	}

	//displayGridColour();

}

int main()
{

	eta = 1;

	regen_lightning();

	const int screenWidth = 1200;
	const int screenHeight = 1200;

	InitWindow(screenWidth, screenHeight, "DBM Testing");

	SetTargetFPS(60);

	std::chrono::microseconds duration = std::chrono::microseconds();
	while (!WindowShouldClose())
	{

		// check for num input, used to regen lightning and set eta
		
		if (IsKeyPressed(KEY_ONE))  {eta = 1;}
		if (IsKeyPressed(KEY_TWO))	{eta = 2;}
		if (IsKeyPressed(KEY_THREE)){eta = 3;}
		if (IsKeyPressed(KEY_FOUR)) {eta = 4;}
		if (IsKeyPressed(KEY_FIVE)) {eta = 5;}

		if (IsKeyPressed(KEY_EQUAL)) { x_size++; y_size++; }
		if (IsKeyPressed(KEY_MINUS)) {
			if (x_size > 5)
			{
				x_size--;
				y_size--;
			}
		}


		if (IsKeyPressed(KEY_SPACE)) { 
			auto time_at_start = std::chrono::high_resolution_clock::now(); 
			regen_lightning();
			auto time_at_end = std::chrono::high_resolution_clock::now();
			duration = std::chrono::duration_cast<std::chrono::microseconds>(time_at_end - time_at_start);
		}

		BeginDrawing();

		ClearBackground(BLACK);

		int segment_size = 50;

		if (segment_size * (y_size + 8) > 1200)
		{
			segment_size = 1200 / (y_size + 8);
		}

		int font_size = segment_size / 2;

		int y_offset = segment_size;

		// background squares

		for (int i = 0; i <= x_size -1; i++)
		{
			for (int j = 0; j <= y_size-1; j++)
			{
				Color square = BLUE;

				if (j == y_size-1)
				{
					square = GREEN;
				}

				DrawRectangleLines(i * segment_size, (j * segment_size) + y_offset , segment_size, segment_size, square);

				DrawCircle(i * segment_size + (segment_size / 2), segment_size / 2, (segment_size / 2) + (segment_size/4), GRAY);
			}

			
			DrawText(TextFormat("%i", i+1), x_size*segment_size, (i * segment_size) + y_offset, 20, WHITE);

		}

		for (int i = 0; i <= x_size-1; i++)
		{
			DrawText(TextFormat("%i", i+1), i * segment_size,(y_size * segment_size) + y_offset, 20, WHITE);
		}

		// little house

		// house body
		DrawRectangleLines(((x_size - 3) * segment_size) + segment_size/3, ((y_size) * segment_size) - segment_size/3, segment_size / 3, segment_size / 3, RED);

		// house Window

		DrawRectangleLines(((x_size - 3) * segment_size) + segment_size / 3, ((y_size)*segment_size) - segment_size / 3 + segment_size / 9 , segment_size / 9, segment_size / 3 /2, BLUE);



		Vector2 roof1; roof1.x = ((x_size - 3) * segment_size) + segment_size / 3 - segment_size/6; roof1.y = ((y_size) * segment_size) - segment_size / 3;

		Vector2 roof2; roof2.x = ((x_size - 3) * segment_size) + segment_size / 3 + segment_size / 3 + segment_size / 6; roof2.y = ((y_size) * segment_size) - segment_size / 3;

		Vector2 roof3; roof3.x = ((x_size - 3) * segment_size) + segment_size / 3 + segment_size / 6; roof3.y = ((y_size) * segment_size) - segment_size/2;

		// house roof
		DrawTriangleLines(roof1, roof2, roof3, RED);


		// info text

		DrawText(TextFormat("Lightning Generation DBM Test"),0, ((y_size + 1) * segment_size) + y_offset, font_size*2, WHITE);
		DrawText(TextFormat("Press Spacebar to regenerate lightning!"), 0, ((y_size + 2) * segment_size) + y_offset, font_size * 2, WHITE);

		DrawText(TextFormat("Latest generation time: % i  microseconds", duration ), 0, ((y_size + 3) * segment_size) + y_offset, font_size * 2, WHITE);



		DrawText(TextFormat("Eta: %i", eta), 0, ((y_size + 4.5) * segment_size) + y_offset, font_size, WHITE);

		DrawText(TextFormat("Use number keys to switch the Eta value (1/2/3/4/5)"), 0, ((y_size + 5) * segment_size) + y_offset, font_size, WHITE);


		DrawText(TextFormat("Grid Size: %i", x_size), 0, ((y_size + 5.5) * segment_size) + y_offset, font_size, WHITE);


		DrawText(TextFormat("Use +/- keys to increase or decrease the grid size! (Minimum 5)"), 0, ((y_size + 6) * segment_size) + y_offset, font_size, WHITE);

	

		// draw lightning last so that it is on top of everything

		for (int i = 0; i < lightning_points.size(); i++)
		{
			DrawLine(lightning_points[i].parent_x * segment_size, (lightning_points[i].parent_y * segment_size) + y_offset, lightning_points[i].x * segment_size, (lightning_points[i].y * segment_size) + y_offset, YELLOW);
		}


		EndDrawing();

	}

	CloseWindow();    

	return 0;
}