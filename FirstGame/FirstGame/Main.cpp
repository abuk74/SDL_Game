#include <stdio.h>
#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "Main.h"

#include <Windows.h>

typedef unsigned char uchar;

const int screenWidth = 1920;
const int screenHight = 1080;

const int gridWidth = 15;
const int gridHeight = 11;

uchar grid[gridHeight][gridWidth];

const char defaultCharacterSpritePath[] = "image.png";

static float deltaTime = 0.0f;
static bool isMoving[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
static bool isPlayerRound = true;
static int roundIndex = 0;



#pragma region Vectors


struct Vector2
{
	float x;
	float y;

	Vector2(float valueX, float valueY);

	~Vector2();
};

Vector2::Vector2(float valueX, float valueY)
{
	x = valueX;
	y = valueY;
}

Vector2::~Vector2() {};

struct Vector2i
{
	int x;
	int y;

	Vector2i(int valueX, int valueY);

	~Vector2i();
};

Vector2i::Vector2i(int valueX, int valueY)
{
	x = valueX;
	y = valueY;
}

Vector2i::~Vector2i() {};

struct Vector4
{
	float x, y, z, w;

	Vector4();
	Vector4(float valueX, float valueY, float valueZ, float valueW);

	~Vector4();
};


Vector4::Vector4()
{
};

Vector4::Vector4(float valueX, float valueY, float valueZ, float valueW)
{
	x = valueX;
	y = valueY;
	z = valueZ;
	w = valueW;
}

Vector4::~Vector4() {};

#pragma endregion

#pragma region Image

struct Image
{
	SDL_Texture* texture;
	Vector2 textureSize;

	Image(SDL_Texture* tex, Vector2 size);

	void Render(SDL_Renderer* renderer, Vector2 position);
	void Destroy();

	~Image();
};

Image::Image(SDL_Texture* tex, Vector2 size)
	:textureSize(size.x, size.y)
{
	texture = tex;
}

void Image::Render(SDL_Renderer* renderer, Vector2 position)
{
	SDL_Rect rect;
	rect.x = position.x * textureSize.x;
	rect.y = position.y * textureSize.y;
	rect.w = textureSize.x;
	rect.h = textureSize.y;

	SDL_RenderCopyEx(renderer,
		texture,
		nullptr,
		&rect,
		0,
		nullptr,
		SDL_FLIP_NONE);
}

void Image::Destroy()
{
	SDL_DestroyTexture(texture);
}

Image::~Image()
{
	Destroy();
}

#pragma endregion

#pragma region Collections


struct Node
{
	Node* another_node;
	Vector2 position;
};

struct Queue
{
	void AddNode(Vector2 nodePosition);
	bool IsEmpty();
	void DeleteFirstNode();
	void Clear();

	Node* FirstNode = nullptr;
};

void Queue::DeleteFirstNode()
{
	Node* next_node = FirstNode->another_node;
	free(FirstNode);
	FirstNode = next_node;

}
void Queue::Clear()
{
	while (FirstNode)
	{
		DeleteFirstNode();
	}
}

void Queue::AddNode(Vector2 nodePosition)
{
	Node** last_node = &FirstNode;
	while (*last_node)
	{
		last_node = &(*last_node)->another_node;
	}
	*last_node = (Node*)malloc(sizeof(Node));
	(*last_node)->another_node = nullptr;
	(*last_node)->position = nodePosition;
}


bool Queue::IsEmpty()
{
	return !FirstNode;
}

#pragma endregion

#pragma region Character

struct Character
{
	Image image;
	Vector2 position;
	Queue path;
	int characterIndex;

	Character(SDL_Texture* tex, Vector2 size, Vector2 pos, int characterIndex);
	void Render(SDL_Renderer* renderer);
	void Move();
	void MoveInit(Vector2 destination);

	~Character();
};

Character::Character(SDL_Texture* tex, Vector2 size, Vector2 pos, int id)
	:image(tex, size), position(pos.x, pos.y)
{
	characterIndex = id;
}

void Character::Render(SDL_Renderer* renderer)
{
	image.Render(renderer, position);
}

void DrawMap(uchar arr[][15])
{
	for (int i = 0; i < gridHeight; i++)
	{
		for (int j = 0; j < gridWidth; j++)
		{
			printf("%i\t", arr[i][j]);
		}
		printf("\n");
	}
}

void Character::MoveInit(Vector2 destination)
{
	path = GetPathGrassfire(position, destination);
}
void Character::Move()
{
	if (!path.IsEmpty())
	{
		position = path.FirstNode->position;
		path.DeleteFirstNode();
		isMoving[characterIndex] = true;
		Sleep(150);
	}
	else
		isMoving[characterIndex] = false;
}

Character:: ~Character() {};

#pragma endregion

#pragma region SDL

bool InitializeSDL(SDL_Renderer* renderer, Vector4 backgroundColor)
{

	// Init SDL libraries
	SDL_SetMainReady(); // Just leave it be
	int result = 0;
	result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (result)
	{
		printf("Can't initialize SDL. Error: %s", SDL_GetError());
		return false;
	}

	result = IMG_Init(IMG_INIT_PNG);
	if (!(result & IMG_INIT_PNG))
	{
		printf("Can't initialize SDL image. Error: %s", SDL_GetError());
		return false;
	}
	SDL_SetRenderDrawColor(renderer, backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
	return true;
}

SDL_Renderer* GetRenderer(SDL_Window* window)
{
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer)
		abort();
	return renderer;
}

SDL_Window* GetWindow()
{
	SDL_Window* window = SDL_CreateWindow("FirstSDL",
		0, 0,
		screenWidth, screenHight,
		SDL_WINDOW_SHOWN);

	if (!window)
		abort();
	return window;
}

#pragma endregion

Queue GetPathGrassfire(Vector2 start_pos, Vector2 end_pos)
{
	Queue search_queue, path;
	uchar grid_copy[gridHeight][gridWidth];
	for (int i = 0; i < gridHeight; i++)
	{
		for (int j = 0; j < gridWidth; j++)
		{
			grid_copy[i][j] = grid[i][j];
		}
	}

	grid_copy[(int)end_pos.y][(int)end_pos.x]++;

	search_queue.AddNode(end_pos);

	while (!search_queue.IsEmpty() && path.IsEmpty())
	{
		const Vector2 current_pos = search_queue.FirstNode->position;
		search_queue.DeleteFirstNode();

		uchar next_value = grid_copy[(int)current_pos.y][(int)current_pos.x] + 1;

		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				if (i != j && i * j == 0)
				{
					Vector2 neighbour = { current_pos.x + j, current_pos.y + i };
					if ((int)neighbour.x == (int)start_pos.x && (int)neighbour.y == (int)start_pos.y)
					{
						path.AddNode(neighbour);
						grid_copy[(int)neighbour.y][(int)neighbour.x] = next_value;
					}
					else if (neighbour.y > 0 && neighbour.y < gridHeight && neighbour.x > 0 && neighbour.x < gridWidth &&
						grid_copy[(int)neighbour.y][(int)neighbour.x] == grid[(int)neighbour.y][(int)neighbour.x] && grid_copy[(int)neighbour.y][(int)neighbour.x] != 255)
					{
						search_queue.AddNode(neighbour); // add point to check-list 
						grid_copy[(int)neighbour.y][(int)neighbour.x] = next_value;
					}
				}
			}
		}
	}

	search_queue.Clear();
	DrawMap(grid_copy);

	if (!path.IsEmpty())
	{

		Vector2 current_pos = path.FirstNode->position;
		int next_value = grid_copy[(int)current_pos.y][(int)current_pos.x] - 1;
		while (next_value > grid_copy[(int)end_pos.y][(int)end_pos.x])
		{
			for (int i = -1; i <= 1; i++)
				for (int j = -1; j <= 1; j++)
					if (i * j == 0 && i != j)
					{
						Vector2 neighbour{ current_pos.x + i, current_pos.y + j };
						if (neighbour.x >= 0 && neighbour.y >= 0 && neighbour.x < gridWidth && neighbour.y < gridHeight)
						{
							if (grid_copy[(int)neighbour.y][(int)neighbour.x] == next_value)
							{
								path.AddNode(neighbour);
								current_pos = neighbour;
								next_value--;
							}
						}
					}
		}
		path.AddNode(end_pos);
	}
	return path;
}

Vector2 CastToGridPosition(Vector2 mousePosition)
{
	return Vector2(mousePosition.x / (screenWidth / gridWidth), mousePosition.y / (screenHight / gridHeight));
}

int GetRandomIndex(int value)
{
	int random = rand() % value;
	return random;
}

Vector2i GetRandomGrid()
{
	int randomX = GetRandomIndex(gridWidth);
	int randomY = GetRandomIndex(gridHeight);
	if (grid[randomX][randomY] != 255)
	{
		Vector2i vector = { randomX, randomY };
		return vector;
	}
	else
		GetRandomGrid();
}

bool IsMoving()
{
	for (int i = 0; i < 16; i++)
	{
		if (isMoving[i])
			return true;
	}
	return false;
}

int main()
{
	Vector4 backgroundColor = Vector4(3, 34, 48, 255);

	SDL_Window* window = GetWindow();
	SDL_Renderer* renderer = GetRenderer(window);
	if (!InitializeSDL(renderer, backgroundColor))
		return -1;

	Vector2 screenSize(screenWidth / gridWidth, screenHight / gridHeight);

	Character championA = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 3), 0);
	Character championB = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 4), 1);
	Character championC = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 5), 2);
	Character championD = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 6), 3);
	Character championE = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 7), 4);
	Character championF = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 8), 5);
	Character championG = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 9), 6);
	Character championH = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(0, 10), 7);

	Character championAIA = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 2), 8);
	Character championAIB = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 3), 9);
	Character championAIC = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 4), 10);
	Character championAID = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 5), 11);
	Character championAIE = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 6), 12);
	Character championAIF = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 7), 13);
	Character championAIG = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 8), 14);
	Character championAIH = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2(14, 9), 15);

	int amountPlayer = 8;
	int amountAI = 8;
	int playerIndex = 0;
	int AIIndex = 0;

	Character* playerChampions[8] = { &championA, &championB , &championC ,&championD ,&championE ,&championF ,&championG ,&championH };
	Character* agents[8] = { &championAIA, &championAIB ,&championAIC ,&championAID ,&championAIE ,&championAIF ,&championAIG, &championAIH };


	Image obstacleA = Image(SDL_CreateTextureFromSurface(renderer, IMG_Load("obstacle.png")), screenSize);
	Image obstacleB = Image(SDL_CreateTextureFromSurface(renderer, IMG_Load("obstacle.png")), screenSize);
	Image obstacleC = Image(SDL_CreateTextureFromSurface(renderer, IMG_Load("obstacle.png")), screenSize);
	Image obstacleD = Image(SDL_CreateTextureFromSurface(renderer, IMG_Load("obstacle.png")), screenSize);

	Image* obstacles[4] = { &obstacleA, &obstacleB, &obstacleC, &obstacleD };

	Vector2i obstacleAPos = GetRandomGrid();
	grid[obstacleAPos.y][obstacleAPos.x] = 255;

	Vector2i obstacleBPos = GetRandomGrid();
	grid[obstacleBPos.y][obstacleBPos.x] = 255;

	Vector2i obstacleCPos = GetRandomGrid();
	grid[obstacleCPos.y][obstacleCPos.x] = 255;

	Vector2i obstacleDPos = GetRandomGrid();
	grid[obstacleDPos.y][obstacleDPos.x] = 255;

	bool done = false;
	SDL_Event sdlEvent;

	float startTime = SDL_GetTicks();
	float lastTime = startTime;

	Queue path;

	while (!done)
	{
		startTime = lastTime;
		// Polling the messages from the OS.
		// That could be key downs, mouse movement, ALT+F4 or many others
		while (SDL_PollEvent(&sdlEvent))
		{
			ProcessEvents(&sdlEvent, &done, playerChampions[playerIndex % amountPlayer], &playerIndex);
		}

		for (int i = 0; i < 8; i++)
		{
			playerChampions[i]->Move();
			agents[i]->Move();
		}


		if (!isPlayerRound && !IsMoving())
		{
			Vector2i dest = GetRandomGrid();
			agents[AIIndex % amountAI]->MoveInit(Vector2(dest.x, dest.y));
			AIIndex++;
			isPlayerRound = true;
		}



		// Clearing the screen
		SDL_RenderClear(renderer);


		for (int i = 0; i < 8; i++)
		{
			playerChampions[i]->Render(renderer);
			agents[i]->Render(renderer);
		}

		obstacleA.Render(renderer, Vector2(obstacleAPos.x, obstacleAPos.y));
		obstacleB.Render(renderer, Vector2(obstacleBPos.x, obstacleBPos.y));
		obstacleC.Render(renderer, Vector2(obstacleCPos.x, obstacleCPos.y));
		obstacleD.Render(renderer, Vector2(obstacleDPos.x, obstacleDPos.y));

		// Showing the screen to the player
		SDL_RenderPresent(renderer);

		// next frame...
	}
	// Shutting down the renderer
	SDL_DestroyRenderer(renderer);

	// Shutting down the window
	SDL_DestroyWindow(window);

	// Quitting the Image SDL library
	IMG_Quit();
	// Quitting the main SDL library
	SDL_Quit();

	// Done.
	return 0;
}


void ProcessEvents(SDL_Event* event, bool* done, Character* player, int* PIndex)
{
	if (event->type == SDL_QUIT) // The user wants to quit
	{
		*done = true;
	}
	else if (event->type == SDL_KEYDOWN) // A key was pressed
	{
		switch (event->key.keysym.sym) // Which key?
		{
		case SDLK_ESCAPE: // Posting a quit message to the OS queue so it gets processed on the next step and closes the game
			SDL_Event event;
			event.type = SDL_QUIT;
			event.quit.type = SDL_QUIT;
			event.quit.timestamp = SDL_GetTicks();
			SDL_PushEvent(&event);
			break;
		default:
			break;
		}
	}
	else if (event->type == SDL_MOUSEBUTTONDOWN)
	{
		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
		{
			if (!IsMoving() && isPlayerRound)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				player->MoveInit(CastToGridPosition(Vector2(x, y)));
				*PIndex += 1;
				isPlayerRound = false;
			}
			break;
		}
		default:
			break;
		}
	}
}
