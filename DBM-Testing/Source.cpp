
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

#include "raylib.h"

#include "imgui.h"
#include "rlImGui.h"

int size = 5;

int x_size = 5;
int y_size = 5;
int z_size = 5;

int prev_x = x_size;
int prev_y = y_size;
int prev_z = z_size;

int test_x = 10;

const int LIGHTNING_STEPS = 30;
int MAX_GRADIENT_LAPLACE_LOOPS = 50;

bool reached_edge = false;

int eta = -1;

int segment_size = 4;

std::chrono::microseconds duration = std::chrono::microseconds();

bool enable_cubes = false;

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
	int x, y, z, parent_x, parent_y, parent_z;
	float potential, probability;
};

struct lightning_cell
{
	lightning_cell(int x, int y, int z, int px, int py, int pz) : x(x), y(y), z(z), parent_x(px), parent_y(py), parent_z(pz) {}

	int x, y, z, parent_x, parent_y, parent_z;
};

std::vector<lightning_cell> lightning_points;

std::vector<std::vector<std::vector<float>>> potentials;

std::vector<std::vector<std::vector<float>>> new_potentials;

std::vector<std::vector<std::vector<int>>> starting;

std::vector<candidate_cell> candidates;

void createStartingGrid()
{
	starting.clear();

	for (int z = 0; z < z_size; z++)
	{
		std::vector<std::vector<int>> face;

		for (int y = 0; y < y_size; y++)
		{
			std::vector<int> row;

			for (int x = 0; x < x_size; x++)
			{
				if (y == 0) //check for ceiling
				{
					row.push_back(3);
				}
				else if (y == y_size - 1) // check for ground
				{
					row.push_back(1);
				}
				else if (x == 0 || x == x_size - 1 || z == 0 || z == z_size - 1) // check for walls
				{
					row.push_back(3);
				}
				else // air
				{
					row.push_back(0);
				}
			}

			face.push_back(row);
		}

		starting.push_back(face);
	}

	int starting_x = x_size / 2;
	int starting_z = z_size / 2;

	starting[starting_x][0][starting_z] = 2;
}

void initialiseGrid()
{
	lightning_points.clear();
	potentials.clear();
	new_potentials.clear();

	MAX_GRADIENT_LAPLACE_LOOPS = std::max(x_size * 1.5 + 20, double(30));
	reached_edge = false;


	createStartingGrid();

	for (int z = 0; z < z_size; z++)
	{
		std::vector<std::vector<float>> face;

		for (int y = 0; y < y_size; y++)
		{
			std::vector<float> row;

			for (int x = 0; x < x_size; x++)
			{
				if (y == 0) //check for ceiling
				{
					row.push_back(0);
				}
				else if (y == y_size - 1) // check for ground
				{
					row.push_back(1);
				}
				else if (x == 0 || x == x_size - 1 || z == 0 || z == z_size - 1) // check for walls
				{
					row.push_back(0);
				}
				else // air
				{
					row.push_back(0.5);
				}
			}

			face.push_back(row);
		}

		potentials.push_back(face);
		new_potentials.push_back(face);
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

float calculateLaplace(int x, int y, int z)
{
	float left = potentials[y][x - 1][z];
	float right = potentials[y][x + 1][z];
	float forward = potentials[y + 1][x][z];
	float backward = potentials[y - 1][x][z];
	float up = potentials[y][x][z-1];
	float down = potentials[y][x][z+1];


	float average = left + right + forward + backward + up + down;
	average /= 6;

	return average;
}

bool calculateGridStep()
{ 
	bool is_within_tolerance = false;

	const float tolerance = 0.005;

	for (int z = 0; z < z_size; z++)
	{
		for (int y = 0; y < y_size; y++)
		{
			for (int x = 0; x < x_size; x++)
			{
				if (potentials[z][y][x] == 0 || potentials[z][y][x] == 1) //skip anything with 0 or 1
				{
					continue;
				}

				float new_value = calculateLaplace(x,y,z);

				float old_value = potentials[z][y][x];

				new_potentials[z][y][x] = new_value;

				if (abs(old_value - new_value) >= tolerance)
				{
					is_within_tolerance = true;
				}
			}
		}
	}

	std::swap(potentials, new_potentials);

	return is_within_tolerance;
}


void checkCandidacy(int x_pos, int y_pos,int z_pos)
{
	// check if cell has lightning on border
	bool is_ground_candidate_found = false;


	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			for (int z = -1; z <= 1; z++)
			{
				if (x_pos + x >= x_size || y_pos + y >= y_size || z_pos + z >= z_size) continue;  // if checking cells that are out of bounds skip

				if (x == 0 && y == 0 && z == 0) continue;// skip middle cell

				if (y_pos + y < 0) continue; //temp fix // why is this here?????
				if (x_pos + x < 0) continue; //temp fix // why is this here?????
				if (z_pos + z < 0) continue; //temp fix // why is this here?????


				if (starting[z_pos + z][y_pos + y][x_pos + x] == 3) continue; //skip boundaries 

				if (potentials[z_pos + z][y_pos + y][x_pos + x] == 0) // if a surrounding cell is lightning then this is a candidate
				{

					if (starting[z_pos + z][y_pos + y][x_pos + x] == 1) // check if candidate ground is next to lightning
					{
						std::cout << "Candidate was ground!" << std::endl;
						is_ground_candidate_found = true;
						reached_edge = true;
					}

					candidate_cell temp;

					temp.potential = potentials[z_pos][y_pos][x_pos];
					temp.x = x_pos;
					temp.y = y_pos;
					temp.z = z_pos;

					temp.parent_x = x_pos + x;
					temp.parent_y = y_pos + y;
					temp.parent_z = z_pos + z;

					if (is_ground_candidate_found)
					{
						candidates.clear();
						candidates.push_back(temp);
						break;
					}

					candidates.push_back(temp);

					x = 2; y = 2; z = 2; // skip out the check cause we knw it's a candidate // why not just use a break? or similar?
				}
			}
		}
	}
}

void selectLightningCell()
{
	candidates.clear();

	for (int z = 0; z < z_size; z++)
	{
		for (int y = 0; y < y_size; y++)
		{
			for (int x = 0; x < x_size; x++)
			{
				if (potentials[z][y][x] == 0) //skip current lightning cells or boundaries
				{
					continue;
				}

				checkCandidacy(x, y, z);
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

	lightning_points.push_back(lightning_cell(chosen.x, chosen.y, chosen.z, chosen.parent_x,chosen.parent_y, chosen.parent_z));

	potentials[chosen.z][chosen.y][chosen.x] = 0;
	new_potentials[chosen.z][chosen.y][chosen.x] = 0;
}

void resetPotentialGrid()
{
	for (int z = 0; z < z_size; z++)
	{
		for (int y = 0; y < y_size; y++)
		{
			for (int x = 0; x < x_size; x++)
			{
				if (potentials[z][y][x] == 0 || potentials[z][y][x] == 1)
				{
					continue;
				}

				potentials[z][y][x] = 0.5;
			}

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

void render_scene(Camera3D & camera)
{

	ClearBackground(BLACK);


	BeginMode3D(camera);

	if (segment_size * y_size > 50)
	{
		segment_size = 50 / y_size;
	}


	Vector3 centre = { float(x_size * segment_size / 2), float(y_size * segment_size / 2),  float(z_size * segment_size / 2) };

	camera.target = centre;      // Camera looking at point


	UpdateCamera(&camera, CAMERA_ORBITAL);

	//segment_size = 3;


	float spacing = segment_size + 0.1f; // spacing between cubes
	Vector3 cubePosition;

	Color cubeColour = WHITE;



	if (enable_cubes)
	{
		for (int z = 0; z < z_size; z++)
		{
			for (int y = 0; y < y_size; y++)
			{
				for (int x = 0; x < x_size; x++)
				{

					switch (starting[z][y][x])
					{
					case 0:
						cubeColour = BLUE;
						break;
					case 1:
						cubeColour = GREEN;
						break;
					case 2:
						cubeColour = RED;
						break;
					case 3:
						cubeColour = WHITE;
						break;
					default:
						break;
					}
					int flippedY = y_size - 1 - y;

					cubePosition = {
						x * spacing,
						flippedY * spacing,
						z * spacing
					};

					if (starting[z][y][x] != 0)
					{
						DrawCubeWires(cubePosition, segment_size, segment_size, segment_size, cubeColour);
					}
				}
				cubePosition.y -= segment_size + 0.1;
				cubePosition.x = 0;
			}
			cubePosition.z += segment_size + 0.1;
			cubePosition.y = 0;
		}
	}


	int y_offset = segment_size;

	float y_start = segment_size * (y_size - 1.5);

	for (int i = 0; i < lightning_points.size(); i++)
	{ 
		Vector3 start_pos = { lightning_points[i].parent_x *segment_size, y_start - (lightning_points[i].parent_y * segment_size) + y_offset, lightning_points[i].parent_z * segment_size };
		Vector3 end_pos = { lightning_points[i].x * segment_size, y_start - (lightning_points[i].y * segment_size) + y_offset, lightning_points[i].z * segment_size };

		DrawLine3D(end_pos, start_pos, YELLOW);
	}


	EndMode3D();




	//int font_size = segment_size / 2;

	//int y_offset = segment_size;

	//// background squares

	//for (int i = 0; i <= x_size - 1; i++)
	//{
	//	for (int j = 0; j <= y_size - 1; j++)
	//	{
	//		Color square = BLUE;

	//		if (j == y_size - 1)
	//		{
	//			square = GREEN;
	//		}

	//		DrawRectangleLines(i * segment_size, (j * segment_size) + y_offset, segment_size, segment_size, square);

	//		DrawCircle(i * segment_size + (segment_size / 2), segment_size / 2, (segment_size / 2) + (segment_size / 4), GRAY);
	//	}


	//	DrawText(TextFormat("%i", i + 1), x_size * segment_size, (i * segment_size) + y_offset, 20, WHITE);

	//}

	//for (int i = 0; i <= x_size - 1; i++)
	//{
	//	DrawText(TextFormat("%i", i + 1), i * segment_size, (y_size * segment_size) + y_offset, 20, WHITE);
	//}

	//// little house

	//// house body
	//DrawRectangleLines(((x_size - 3) * segment_size) + segment_size / 3, ((y_size)*segment_size) - segment_size / 3, segment_size / 3, segment_size / 3, RED);

	//// house Window

	//DrawRectangleLines(((x_size - 3) * segment_size) + segment_size / 3, ((y_size)*segment_size) - segment_size / 3 + segment_size / 9, segment_size / 9, segment_size / 3 / 2, BLUE);



	//Vector2 roof1; roof1.x = ((x_size - 3) * segment_size) + segment_size / 3 - segment_size / 6; roof1.y = ((y_size)*segment_size) - segment_size / 3;

	//Vector2 roof2; roof2.x = ((x_size - 3) * segment_size) + segment_size / 3 + segment_size / 3 + segment_size / 6; roof2.y = ((y_size)*segment_size) - segment_size / 3;

	//Vector2 roof3; roof3.x = ((x_size - 3) * segment_size) + segment_size / 3 + segment_size / 6; roof3.y = ((y_size)*segment_size) - segment_size / 2;

	//// house roof
	//DrawTriangleLines(roof1, roof2, roof3, RED);


	//// info text

	//DrawText(TextFormat("Lightning Generation DBM Test"), 0, ((y_size + 1) * segment_size) + y_offset, font_size * 2, WHITE);
	//DrawText(TextFormat("Press Spacebar to regenerate lightning!"), 0, ((y_size + 2) * segment_size) + y_offset, font_size * 2, WHITE);

	//DrawText(TextFormat("Latest generation time: % i  microseconds", duration), 0, ((y_size + 3) * segment_size) + y_offset, font_size * 2, WHITE);



	//DrawText(TextFormat("Eta: %i", eta), 0, ((y_size + 4.5) * segment_size) + y_offset, font_size, WHITE);

	//DrawText(TextFormat("Use number keys to switch the Eta value (1/2/3/4/5)"), 0, ((y_size + 5) * segment_size) + y_offset, font_size, WHITE);


	//DrawText(TextFormat("Grid Size: %i", x_size), 0, ((y_size + 5.5) * segment_size) + y_offset, font_size, WHITE);


	//DrawText(TextFormat("Use +/- keys to increase or decrease the grid size! (Minimum 5)"), 0, ((y_size + 6) * segment_size) + y_offset, font_size, WHITE);



	//// draw lightning last so that it is on top of everything

	//for (int i = 0; i < lightning_points.size(); i++)
	//{
	//	DrawLine(lightning_points[i].parent_x * segment_size, (lightning_points[i].parent_y * segment_size) + y_offset, lightning_points[i].x * segment_size, (lightning_points[i].y * segment_size) + y_offset, YELLOW);
	//	
	//}




}

int main()
{

	eta = 1;

	regen_lightning();

	DisableCursor();

	int screenWidth = 1920;
	int screenHeight = 1080;

	InitWindow(screenWidth, screenHeight, "DBM Testing");

	SetTargetFPS(60);


	// Define the camera to look into our 3d world
	Camera3D camera = { 0 };

	camera.position = Vector3({ 20.0f, 20.0f, 20.0f });  // Camera position
	camera.target = Vector3({ 0.0f, 0.0f, 0.0f });      // Camera looking at point
	camera.up = Vector3({ 0.0f, 1.0f, 0.0f });          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_ORTHOGRAPHIC;             // Camera mode type

	Camera3D camera2 = camera;

	rlImGuiSetup(false);


	// Define a render texture to render
	int renderTextureWidth = 300;
	int renderTextureHeight = 300;
	RenderTexture2D target = LoadRenderTexture(renderTextureWidth, renderTextureHeight);

	int num_of_points = 0;
	float point_cd = 0; 


	while (!WindowShouldClose())
	{

		if (prev_x != x_size || prev_y != y_size || prev_z != z_size)
		{
			regen_lightning();
			Vector3 position = { float(x_size * segment_size), (y_size + 1) * segment_size / 2,  -float(z_size * segment_size) };
			camera.position = position;      // Camera looking at point

			num_of_points = 0;
			camera2.position = position;

			prev_x = x_size;
			prev_y = y_size;
			prev_z = z_size;
		}

		// keyboard input now irrelevant due to imgui
		//// check for num input, used to regen lightning and set eta
		//
		//if (IsKeyPressed(KEY_ONE))  {eta = 1;}
		//if (IsKeyPressed(KEY_TWO))	{eta = 2;}
		//if (IsKeyPressed(KEY_THREE)){eta = 3;}
		//if (IsKeyPressed(KEY_FOUR)) {eta = 4;}
		//if (IsKeyPressed(KEY_FIVE)) {eta = 5;}

		//if (IsKeyPressed(KEY_EQUAL)) {
		//	x_size++; 
		//	y_size++; 
		//	z_size++; 
		//	regen_lightning();
		//	Vector3 position = { float(x_size * segment_size), (y_size + 1) * segment_size /2,  -float(z_size * segment_size) };
		//	camera.position = position;      // Camera looking at point

		//}

		//if (IsKeyPressed(KEY_MINUS)) {
		//	if (x_size > 5)
		//	{
		//		x_size--;
		//		y_size--;
		//		z_size--;
		//	}
		//	regen_lightning();
		//	Vector3 position = { float(x_size * segment_size), (y_size + 1) * segment_size / 2,  -float(z_size * segment_size) };
		//	camera.position = position;      // Camera looking at point

		//}


		//if (IsKeyPressed(KEY_C)) { enable_cubes = !enable_cubes; }


		//if (IsKeyPressed(KEY_SPACE)) { 
		//	auto time_at_start = std::chrono::high_resolution_clock::now(); 
		//	regen_lightning();
		//	auto time_at_end = std::chrono::high_resolution_clock::now();
		//	duration = std::chrono::duration_cast<std::chrono::microseconds>(time_at_end - time_at_start);
		//}
		
		{
			BeginTextureMode(target);
			ClearBackground(BLACK);

			Vector3 centre = { float(x_size * segment_size / 2), float(y_size * segment_size / 2),  float(z_size * segment_size / 2) };
			camera2.target = centre;


			BeginMode3D(camera2);
			int y_offset = segment_size;

			float y_start = segment_size * (y_size - 1.5);

			for (int i = 0; i < num_of_points; i++)
			{
				Vector3 start_pos = { lightning_points[i].parent_x * segment_size, y_start - (lightning_points[i].parent_y * segment_size) + y_offset, lightning_points[i].parent_z * segment_size };
				Vector3 end_pos = { lightning_points[i].x * segment_size, y_start - (lightning_points[i].y * segment_size) + y_offset, lightning_points[i].z * segment_size };

				DrawLine3D(end_pos, start_pos, YELLOW);
			}
			EndMode3D();
			EndTextureMode();

			point_cd += GetFrameTime();

			if (point_cd > 0.1)
			{
				num_of_points++;
				point_cd = 0;
			}

			if (num_of_points == lightning_points.size())
			{
				num_of_points = 0;
			}
		}



		BeginDrawing();
		render_scene(camera);


		
		rlImGuiBegin();

			ImGui::Begin("Alt View", NULL);
				rlImGuiImageRenderTexture(&target);
			ImGui::End();



			ImGui::Begin("DBM Test", NULL);
			ImGui::Text("Display");
			ImGui::Checkbox("Cube Display", &enable_cubes);

			ImGui::Text("Controls");


			ImGui::SliderInt("Size", &size, 5, 10);

			x_size = size;
			y_size = size;
			z_size = size;
			//ImGui::SliderInt("X Size", &x_size, 5,10);
			//ImGui::SliderInt("Y Size", &y_size, 5, 10);
			//ImGui::SliderInt("Z Size", &z_size, 5, 10);
			ImGui::SliderInt("Eta", &eta, 1, 10);

			if (ImGui::Button("Regenerate Lightning", { 300,50 }))
			{
				regen_lightning();
				num_of_points = 0;
			}

			ImGui::End();
		rlImGuiEnd();

		EndDrawing();
	}

	

	CloseWindow();    

	return 0;
}