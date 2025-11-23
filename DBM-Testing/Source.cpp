
#include <iostream>
#include <vector>
#include <random>

#include "raylib.h"

const int x_size = 10;
const int y_size = 10;

const int LIGHTNING_STEPS = 10;
const int MAX_GRADIENT_LAPLACE_LOOPS = 50;

bool reached_edge = false;

int eta = 1;

//std::vector<Vector2> lightning_points;


float potential_grid[y_size][x_size] =
{
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
};

float potential_grid_updates[y_size][x_size] =
{
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
};

// 0 for air
// 1 for ground
// 2 for starting charge
// 3 for boundary

int starting_grid[y_size][x_size] =
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


void initialiseGrid()
{
	lightning_points.clear();
	reached_edge = false;
	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			switch (starting_grid[i][j])
			{
			case 0:
				potential_grid[i][j] = 0.5;
				potential_grid_updates[i][j] = 0.5;
				break;
			case 1:
				potential_grid[i][j] = 1;
				potential_grid_updates[i][j] = 1;
				break;
			case 2:
				potential_grid[i][j] = 0;
				potential_grid_updates[i][j] = 0;
				break;
			case 3:
				potential_grid[i][j] = 0;
				potential_grid_updates[i][j] = 0;
				break;
			default:
				break;
			}

		}
	}
}

void displayGrid()
{
	for (int i = 0;  i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			std::cout << potential_grid[i][j] << ",";
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
			switch (starting_grid[i][j])
			{
			case 0: // if air in starting we check if its still air or "lightning"
				if (potential_grid[i][j] != 0)
				{
					std::cout << "\033[36;46m" << "0" << "\033[0m"; // yellow
				}
				else
				{
					std::cout << "\033[33;43m" << "0" << "\033[0m"; // blue
				}
				break;
			case 1: // ground
				std::cout << "\033[32;42m" << "0" << "\033[0m"; // green
				break;
			case 2: // starting charge
				std::cout <<  "\033[31;41m" << "0" << "\033[0m"; // red
				break;
			case 3: //boundary
				std::cout << "\033[37;47m" << "0" << "\033[0m"; // white
				break;
			default:
				std::cout << "\033[0m" << "0" << "\033[0m"; // default
				break;
			}


	
		}
		std::cout << std::endl;
	}
}

float calculateLaplace(int x, int y)
{
	float left = potential_grid[y][x - 1];
	float right = potential_grid[y][x + 1];
	float up = potential_grid[y + 1][x];
	float down = potential_grid[y - 1][x];

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
			if (potential_grid[i][j] == 0 || potential_grid[i][j] == 1) //skip anything with 0 or 1
			{
				continue;
			}

			float new_value  = calculateLaplace(j, i);

			float old_value = potential_grid[i][j];

			potential_grid_updates[i][j] = new_value;

			//add tolerance check
			
			if (abs(old_value - new_value) >= tolerance)
			{
				is_within_tolerance = true;
			}
		}
	}

	std::swap(potential_grid, potential_grid_updates);

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
			if (potential_grid[i][j] == 0) //skip current lightning cells or boundaries
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
					if (starting_grid[i + x][j + y] == 3) continue; //skip boundaries

					if (potential_grid[i + x][j + y] == 0) // if a surrounding cell is lightning then this is a candidate, might be picking up boundaries at the moment.
					{
	
						if (potential_grid[i][j] == 1) // check if candidate ground is next to lightning
						{
							std::cout << "Candidate was ground!" << std::endl;
							is_ground_candidate_found = true;
							reached_edge = true;
						}

						candidate_cell temp;

						temp.potential = potential_grid[i][j];
						temp.x = j;
						temp.y = i;
						temp.parent_x = j + y;
						temp.parent_y = i + x;

						if (is_ground_candidate_found) // TODO: FINISH GRABBING GROUND!!!
						{
							candidates.clear();
							candidates.push_back(temp);
							break;
						}

						candidates.push_back(temp);

						x = 2; y = 2; // skip out the check cause we knw it's a candidate, this feels evil
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

		//std::cout << "Probability: " << x.probability << " | Potential: " << x.potential << " | X: " << x.x << " | Y: " << x.y << std::endl;
	}

	// taken from cpp reference  https://en.cppreference.com/w/cpp/numeric/random/generate_canonical.html
	  
	// baddddd

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

	potential_grid[chosen.y][chosen.x] = 0;
	potential_grid_updates[chosen.y][chosen.x] = 0;
}

void resetPotentialGrid()
{
	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			if (potential_grid[i][j] == 0 || potential_grid[i][j] == 1)
			{
				continue;
			}

			potential_grid[i][j] = 0.5;
		}
		
	}
}

void performLightningStep()
{
	bool is_within_tolerance = true;

	while(is_within_tolerance)
	{
		is_within_tolerance = calculateGridStep();
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

	displayGridColour();

}

int main()
{

	eta = 1;

	regen_lightning();

	const int screenWidth = 1200;
	const int screenHeight = 900;

	InitWindow(screenWidth, screenHeight, "DBM Testing");

	SetTargetFPS(60);

	while (!WindowShouldClose())
	{

		// check for num input, used to regen lightning and set eta
		
		if (IsKeyPressed(KEY_ONE)) { eta = 1; regen_lightning(); }
		if (IsKeyPressed(KEY_TWO)){ eta = 2; regen_lightning();}
		if (IsKeyPressed(KEY_THREE)){ eta = 3; regen_lightning();}
		if (IsKeyPressed(KEY_FOUR)) {eta = 4; regen_lightning(); }
		if (IsKeyPressed(KEY_FIVE)){ eta = 5; regen_lightning(); }

		BeginDrawing();

		ClearBackground(BLACK);

		int segment_size = 50;

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

		DrawText(TextFormat("Lightning Generation DBM Test"),0, ((y_size + 1) * segment_size) + y_offset, 40, WHITE);

		DrawText(TextFormat("Eta: %i", eta), 0, ((y_size + 3) * segment_size) + y_offset, 20, WHITE);

		DrawText(TextFormat("Use number keys to switch the Eta value (1/2/3/4/5)"), 0, ((y_size + 4) * segment_size) + y_offset, 20, WHITE);


		for (int i = 0; i < lightning_points.size(); i++)
		{
			DrawLine(lightning_points[i].parent_x * segment_size, (lightning_points[i].parent_y * segment_size) + y_offset, lightning_points[i].x * segment_size, (lightning_points[i].y * segment_size) + y_offset, YELLOW);
		}


		EndDrawing();

	}

	CloseWindow();    

	return 0;
}