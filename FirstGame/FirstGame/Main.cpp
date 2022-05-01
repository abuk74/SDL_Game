#include <stdio.h>
#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "Main.h"

#include <Windows.h>
 
const int screenWidth = 1920;
const int screenHight = 1080;

const int gridWidth = 15;
const int gridHeight = 11;

const char image_path[] = "space-invaders.png";

static float deltaTime = 0.0f;

struct Vector2
{
	float x;
	float y;
};

struct Vector4 
{
	int x, y, z, w;
	void Init(int x_value, int y_value, int z_value, int w_value);
};
void Vector4::Init(int x_value, int y_value, int z_value, int w_value)
{
	x = x_value;
	y = y_value;
	z = z_value;
	w = w_value;
}

struct Obstacle
{
	SDL_Texture* texture;
	Vector2 size;
	void Init(SDL_Texture* tex, int width, int height);
	void Render(SDL_Renderer* renderer, float x, float y);
	void Destroy();
};
 
void Obstacle::Init(SDL_Texture* tex, int width, int height)
{
	texture = tex;
	size.x = width;
	size.y = height;
}
 
void Obstacle::Render(SDL_Renderer* renderer, float x, float y)
{
	SDL_Rect rect;
	rect.x = round(x * screenWidth/gridWidth); 
	rect.y = round(y * screenHight/gridHeight);
	rect.w = size.x;
	rect.h = size.y;
 
	SDL_RenderCopyEx(renderer,
		texture,
		nullptr,
		&rect,
		0,
		nullptr,
		SDL_FLIP_NONE);
}
 
void Obstacle::Destroy()
{
	SDL_DestroyTexture(texture);
}
 
struct Player
{
	Obstacle block;
	Vector2 position;
	void Init(SDL_Texture* tex, int width, int height, float PositionX, float PositionY);
	void Render(SDL_Renderer* renderer);
};
 
void Player::Init(SDL_Texture* tex, int width, int height, float positionX, float positionY)
{
	block.Init(tex, width, height);
	position.x = positionX;
	position.y = positionY;
}
 
void Player::Render(SDL_Renderer* renderer)
{
	block.Render(renderer, position.x, position.y);
}
 
struct List
{
	List* PastElement;
	unsigned char X;
	unsigned char Y;
};
 
struct Stack
{
	List* LastElement = nullptr;
	void AddElement(int x, int y);
	void DeleteLastElement();
 
	void Clear();
};

void Stack::AddElement(int x, int y)
{
	List* NewElement = (List*)malloc(sizeof(List));
	NewElement->PastElement = LastElement;
	NewElement->X = x;
	NewElement->Y = y;
	LastElement = NewElement;
}

void Stack::DeleteLastElement()
{
	if (LastElement)
	{
		List* PastElement = LastElement->PastElement;
		free(LastElement);
		LastElement = PastElement;
	}
	else
	{
		printf("You Delete non-existent element!");
		abort();
	}
}

void Stack::Clear()
{
	while (LastElement)
	{
		DeleteLastElement();
	}
}

SDL_Renderer* InitializeSDL(SDL_Window* window) 
{
	Vector4 backgroundColor;
	backgroundColor.Init(3, 34, 48, 255);

	// Init SDL libraries
	SDL_SetMainReady(); // Just leave it be
	int result = 0;
	result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); 
	if (result) 
	{
		printf("Can't initialize SDL. Error: %s", SDL_GetError()); 
		abort();
	}
 
	result = IMG_Init(IMG_INIT_PNG); 
	if (!(result & IMG_INIT_PNG)) 
	{
		printf("Can't initialize SDL image. Error: %s", SDL_GetError());
		abort();
	}
 
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer)
		abort();
 
 
	SDL_SetRenderDrawColor(renderer, backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
 
	
	SDL_Surface* surface = IMG_Load(image_path);
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", image_path, IMG_GetError());
		abort();
	}
 
 
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		abort();
	}
 
	// In a moment we will get rid of the surface as we no longer need that. But let's keep the image dimensions.
	int tex_width = surface->w;
	int tex_height = surface->h;

	// Bye-bye the surface
	SDL_FreeSurface(surface);
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
 
int main()
{
	SDL_Window* window = GetWindow();
	SDL_Renderer* renderer = InitializeSDL(window);
 
 
	int WidthPixels = 1920 / gridWidth;
	int HeightPixels = 1080 / gridHeight;
 
	Vector2 currentPosition{ 7, 10 };
	Vector2 destination = currentPosition;
	Vector2 CurrentBlockPosition = currentPosition;
 
	
	Player ship;
	Obstacle redBlock;
	SDL_Surface* s;

	s = SDL_CreateRGBSurface(0, WidthPixels, HeightPixels, 32, 0, 0, 0, 0);
 
	SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 255, 255, 255));
	ship.Init(SDL_CreateTextureFromSurface(renderer, IMG_Load(image_path)), WidthPixels, HeightPixels, 7, 10);
 
	SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 255, 0, 0));
	redBlock.Init(SDL_CreateTextureFromSurface(renderer, s), WidthPixels, HeightPixels);
 
	SDL_FreeSurface(s);
 
 
	unsigned char** arr = (unsigned char**)malloc(sizeof(unsigned char*) * gridHeight);
	for (int i = 0; i < gridHeight; i++)
	{
		arr[i] = (unsigned char*)malloc(sizeof(unsigned char) * gridWidth);
		for (int j = 0; j < gridWidth; j++)
		{
			arr[i][j] = 1;
		}
	}
 
 
	Stack stack1, stack2;
	Stack* currentCheck = &stack1;
	Stack* nextCheck = &stack2;
	Stack path;
 
 
	bool finding = false;
	bool existPath = false;
	bool done = false;
	SDL_Event sdl_event;
	
	float startTime = SDL_GetTicks();
	float lastTime = startTime;

	while (!done)
	{
		startTime = lastTime;
		// Polling the messages from the OS.
		// That could be key downs, mouse movement, ALT+F4 or many others
		while (SDL_PollEvent(&sdl_event))
		{
			ProcessEvents(sdl_event, done, currentCheck, ship, destination, WidthPixels, HeightPixels, arr, finding);
		}
 
		// Clearing the screen
		SDL_RenderClear(renderer);
 
		DrawObstacles(arr, redBlock, renderer);
 
 
		while (finding && !existPath)
		{
			FindPath(currentCheck, arr, nextCheck, path, existPath);

			currentCheck->DeleteLastElement();
			if (!currentCheck->LastElement)
			{
				Stack* temp = currentCheck;
				currentCheck = nextCheck;
				nextCheck = temp;
			}
 
			while (existPath)
			{
				unsigned char x = path.LastElement->X;
				unsigned char y = path.LastElement->Y;
				if (x - 1 >= 0 && arr[y][x - 1] == arr[y][x] - 1)
				{
					path.AddElement(x - 1, y);
				}
				else if (x + 1 < gridWidth && arr[y][x + 1] == arr[y][x] - 1)
				{
					path.AddElement(x + 1, y);
				}
				else if (y - 1 >= 0 && arr[y - 1][x] == arr[y][x] - 1)
				{
					path.AddElement(x, y - 1);
				}
				else if (y + 1 < gridHeight && arr[y + 1][x] == arr[y][x] - 1)
				{
					path.AddElement(x, y + 1);
				}
				if (arr[y][x] == 3)
				{
					for (int i = 0; i < gridHeight; i++)
					{
						for (int j = 0; j < gridWidth; j++)
						{
							arr[i][j] = 1;
						}
					}
					currentCheck->Clear();
					nextCheck->Clear();
					finding = false;
					existPath = false;
					break;
				}
			}
		}
		
		deltaTime = (lastTime - startTime);
		if (path.LastElement)
		{
			ship.position.x = path.LastElement->X;

			//MoveTo(ship, path, renderer);



			ship.position.y = path.LastElement->Y;



			path.DeleteLastElement();
			Sleep(100);

		}
 
		ship.Render(renderer);
// Showing the screen to the player
		SDL_RenderPresent(renderer);
 
		// next frame...
	}
 
	// If we reached here then the main loop stoped
	// That means the game wants to quit
	for (int i = 0; i < gridHeight; i++)
	{
		free(arr[i]);
	}
 
	free(arr);
 
	redBlock.Destroy();
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

void MoveTo(Player& ship, Stack& path, SDL_Renderer* renderer)
{
	while (ship.position.x < path.LastElement->X || ship.position.x > path.LastElement->X || ship.position.y < path.LastElement->Y || ship.position.y > path.LastElement->Y)
	{
	if (ship.position.x < path.LastElement->X)
		ship.position.x = ship.position.x + 5.0f ;
	else if (ship.position.x > path.LastElement->X)
		ship.position.x = ship.position.x - 5.0f ;
	if (ship.position.y < path.LastElement->Y)
		ship.position.y = ship.position.y + 5.0f;
	else if (ship.position.y > path.LastElement->Y)
		ship.position.y = ship.position.y - 5.0f;

	Sleep(100);
	ship.Render(renderer);
	}
}

void FindPath(Stack* currentCheck, unsigned char** arr, Stack* nextCheck, Stack& path, bool& PathGenerate)
{
	unsigned char x = currentCheck->LastElement->X;
	unsigned char y = currentCheck->LastElement->Y;
	if (x - 1 >= 0 && (arr[y][x - 1] == 1 || arr[y][x - 1] == 255))		// LEFT POINT
	{
		if (arr[y][x - 1] != 255)
		{
			arr[y][x - 1] = arr[y][x] + 1;
			nextCheck->AddElement(x - 1, y);
		}
		else
		{
			arr[y][x - 1] = arr[y][x] + 1;
			path.AddElement(x - 1, y);
			PathGenerate = true;
		}
	}


	if (y - 1 >= 0 && (arr[y - 1][x] == 1 || arr[y - 1][x] == 255))		//UP POINT
	{
		if (arr[y - 1][x] != 255)
		{
			arr[y - 1][x] = arr[y][x] + 1;
			nextCheck->AddElement(x, y - 1);
		}
		else
		{
			arr[y - 1][x] = arr[y][x] + 1;
			path.AddElement(x, y - 1);
			PathGenerate = true;
		}

	}

	if (x + 1 < gridWidth && (arr[y][x + 1] == 1 || arr[y][x + 1] == 255))			//RIGTH POINT
	{
		if (arr[y][x + 1] != 255)
		{
			arr[y][x + 1] = arr[y][x] + 1;
			nextCheck->AddElement(x + 1, y);
		}
		else
		{
			arr[y][x + 1] = arr[y][x] + 1;
			path.AddElement(x + 1, y);
			PathGenerate = true;
		}
	}

	if (y + 1 < gridHeight && (arr[y + 1][x] == 1 || arr[y + 1][x] == 255))		//DOWN POINT
	{
		if (arr[y + 1][x])
		{
			if (arr[y + 1][x] != 255)
			{
				arr[y + 1][x] = arr[y][x] + 1;
				nextCheck->AddElement(x, y + 1);
			}
			else
			{
				arr[y + 1][x] = arr[y][x] + 1;
				path.AddElement(x, y + 1);
				PathGenerate = true;
			}
		}
	}
}

void DrawObstacles(unsigned char** arr, Obstacle& RedBlock, SDL_Renderer* renderer)
{
	for (int i = 1; i < gridWidth - 1; i++)
	{
		arr[gridHeight / 2][i] = 0;
		RedBlock.Render(renderer, i, gridHeight / 2);
	}
}

void ProcessEvents(SDL_Event& sdl_event, bool& done, Stack* CheckNow, Player& Ship, Vector2& GoalPosition, int WidthPixels, int HeightPixels, unsigned char** arr, bool& finding)
{
	if (sdl_event.type == SDL_QUIT) // The user wants to quit
	{
		done = true;
	}
	else if (sdl_event.type == SDL_KEYDOWN) // A key was pressed
	{
		switch (sdl_event.key.keysym.sym) // Which key?
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
	else if (sdl_event.type == SDL_MOUSEBUTTONDOWN)
	{
		switch (sdl_event.button.button)
		{
		case SDL_BUTTON_LEFT:
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			CheckNow->AddElement((int)Ship.position.x, (int)Ship.position.y);
			GoalPosition.x = (int)(x / WidthPixels);
			GoalPosition.y = (int)(y / HeightPixels);
			arr[(int)Ship.position.y][(int)Ship.position.x] = 2;
			arr[(int)GoalPosition.y][(int)GoalPosition.x] = 255;
			finding = true;
			break;
		}
		default:
			break;
		}
	}
}
