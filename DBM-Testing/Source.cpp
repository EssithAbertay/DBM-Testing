#include <iostream>
#include <vector>


const int x_size = 10;
const int y_size = 10;

const int num_of_iterations = 30;

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
	int x, y;
	float potential;
};

void initialiseGrid()
{
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

void calculateGridStep()
{ 
	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			if (potential_grid[i][j] == 0 || potential_grid[i][j] == 1) //skip anything with 0 or 1
			{
				continue;
			}

			potential_grid_updates[i][j] = calculateLaplace(j, i);
		}
	}

	for (int i = 0; i < y_size; i++)
	{
		for (int j = 0; j < x_size; j++)
		{
			potential_grid[i][j] = potential_grid_updates[i][j];
		}
	}
}



void selectLightningCell()
{
	std::vector<candidate_cell> candidates;

	// get all candidate cells

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
					if (i + x >= x_size || j + y >= y_size) // if checking cells that are out of bounds skip
					{
						continue;
					}

					if (x == 0 && y == 0) // skip middle cell
					{
						continue;
					}

					if (starting_grid[i + x][j + y] == 3)
					{
						continue;
					}

					if (potential_grid[i + x][j + y] == 0) // if a surrounding cell is lightning then this is a candidate, might be picking up boundaries at the moment.
					{

						if (potential_grid[i][j] == 1) // check if candidate ground is next to lightning
						{
							std::cout << "Candidate was ground!" << std::endl;
						}

						candidate_cell temp;

						temp.potential = potential_grid[i][j];
						temp.x = j;
						temp.y = i;

						candidates.push_back(temp);

						x = 2; y = 2; // skip out the check cause we knw it's a candidate, this feels evil
					}
				}
			}
		}
	}

	candidate_cell largest_candidate = candidates[0];

	for (auto x : candidates)
	{
		if (x.potential > largest_candidate.potential)
		{
			largest_candidate = x;
		}

		std::cout << "X:" << x.x << "| Y:" << x.y << "| Potential:" << x.potential << std::endl;;
	}

	potential_grid[largest_candidate.y][largest_candidate.x] = 0; // the largest cell becomes a lightning spot
	potential_grid_updates[largest_candidate.y][largest_candidate.x] = 0; 
}


int main()
{
	initialiseGrid();

	std::cout << "Initial Grid" << std::endl;

	displayGrid();

	for (int i = 0; i < num_of_iterations; i++)
	{
		calculateGridStep();
	}

	displayGrid();

	selectLightningCell();

	for (int i = 0; i < num_of_iterations; i++)
	{

		calculateGridStep();
	}

	displayGrid();
	selectLightningCell();
	displayGrid();
	return 0;
}