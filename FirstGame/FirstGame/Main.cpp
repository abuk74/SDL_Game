#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "Main.h"

#include <Windows.h>

typedef unsigned char uchar;

const int screenWidth = 1920;
const int screenHight = 1080;

const int gridWidth = 15;
const int gridHeight = 11;

const int fontSize = 42;

uchar grid[gridHeight][gridWidth];

const char defaultCharacterSpritePath[] = "image.png";
const char fontPath[] = "OdibeeSans-Regular.ttf";
const char playerDeadPath[] = "x1.png";
const char agentDeadPath[] = "x2.png";

const int playerFieldValue = 100;
const int agantsFieldValue = 200;

static float deltaTime = 0.0f;
static bool isMoving[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
static bool isPlayerRound = true;
static int roundIndex = 0;

static int alivePlayerCharactersCount = 8;
static int aliveAgentsCount = 8;

SDL_Renderer* renderer;

Character** playerCharactersPointer;
Character** agentsPointer;

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
	Vector2i textureSize;

	Image(SDL_Texture* tex, Vector2i size);

	void Render(SDL_Renderer* renderer, Vector2i position);
	void Destroy();

	~Image();
};

Image::Image(SDL_Texture* tex, Vector2i size)
	:textureSize(size.x, size.y)
{
	texture = tex;
}

void Image::Render(SDL_Renderer* renderer, Vector2i position)
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
	Vector2i position;
};

struct Queue
{
	void AddNode(Vector2i nodePosition);
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

void Queue::AddNode(Vector2i nodePosition)
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

#pragma region UI
struct UI
{
	Image image;
	SDL_Color color;
	TTF_Font* font;
	Vector2i position;

	UI(SDL_Texture* tex, Vector2i size, Vector2i pos, TTF_Font* font);
	void InitText(SDL_Renderer* renderer, TTF_Font* font_, SDL_Color color_, const char* text_);
	void RenderText(SDL_Renderer* renderer, Vector2i pos);
	void SetNewText(SDL_Renderer* renderer, const char* text_);

};
UI::UI(SDL_Texture* tex, Vector2i size, Vector2i pos, TTF_Font* font)
	:image(tex, size), position(pos.x, pos.y)
{
	InitText(renderer, font, { 255, 255, 255 }, "NaN");
}
void UI::InitText(SDL_Renderer* renderer,TTF_Font* font_, SDL_Color color_, const char* text_)
{
	font = font_;
	color = color_;

	SDL_Surface* text = TTF_RenderText_Solid(font_, text_, color_);
	if (!text)
	{
		printf("Failed to render text: %s", TTF_GetError());
		return;
	}

	image = Image(SDL_CreateTextureFromSurface(renderer, text), Vector2i(text->w , text->h));

	SDL_FreeSurface(text);

}
//textSurface = TTF_RenderText_Solid(font, "Alive Player Champions: ", { 255, 255, 255 });
		//textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
		//SDL_Rect dest = { 0, 0, textSurface->w, textSurface->h };
		//SDL_RenderCopy(renderer, textTexture, NULL, &dest);
void UI::RenderText(SDL_Renderer* renderer, Vector2i pos)
{
	position = Vector2i(((pos.x + 1.0f) * 128) / image.textureSize.x, ((pos.y + 1.0f) * 98) / image.textureSize.y); //Vector2i(pos.x * 128, pos.y * 98); //
	image.Render(renderer, position);
}
void UI::SetNewText(SDL_Renderer* renderer,const char* text_)
{
	SDL_Surface* surface = TTF_RenderText_Solid(font, text_, color);

	image.texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);
}
#pragma endregion


#pragma region Character

struct Character
{
	Image image;
	Vector2i position;
	Queue path;
	UI ui;

	int characterIndex;
	float totalHealth;
	float health;
	float damage;
	bool isDead = false;
	bool AI = false;
	int entityCount = 1;


	Character(SDL_Texture* tex, Vector2i size, Vector2i pos, int characterIndex, float health, float damage, bool isAgent, int entityCount, SDL_Texture* fontText, Vector2i fontSurfaceSize, TTF_Font* font);
	void Render(SDL_Renderer* renderer);
	void Move();
	void MoveInit(Vector2i destination);
	void MarkAsObstacle(bool value);
	void MarkAsDead();
	void Resurect();
	void TakeDamage(float damge);
	void Die();

	~Character();
};

Character::Character(SDL_Texture* tex, Vector2i size, Vector2i pos, int id, float startHealth, float baseDamage,  bool isAgent, int entityCount, SDL_Texture* fontText, Vector2i fontSurfaceSize, TTF_Font* font)
	:image(tex, size), position(pos.x, pos.y), ui(fontText, fontSurfaceSize, pos, font)
{
	characterIndex = id;
	health = startHealth;
	this->entityCount = entityCount;
	totalHealth = health * entityCount;
	damage = baseDamage;
	AI = isAgent;
	MarkAsObstacle(true);
}

void Character::Render(SDL_Renderer* renderer)
{
	image.Render(renderer, position);
	ui.RenderText(renderer, position);
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

void Character::MoveInit(Vector2i destination)
{
	path = GetPathGrassfire(position, destination);
}
void Character::Move()
{
	if (isDead)
		return;
	if (!path.IsEmpty())
	{
		int value = agantsFieldValue;
		if (AI)
			value = playerFieldValue;
		Vector2i temPos = path.FirstNode->position;
		if (grid[temPos.y][temPos.x] == value) 
		{
			isMoving[characterIndex] = false;
			if(AI)
				GetTargetReferenceAndTakeDamage(this, temPos, playerCharactersPointer, damage, entityCount);
			else
				GetTargetReferenceAndTakeDamage(this, temPos, agentsPointer, damage, entityCount);
			path.DeleteFirstNode();
			MarkAsObstacle(true);
			Sleep(150);
			return;
		}
		MarkAsObstacle(false);
		position = temPos; //path.FirstNode->position;
		path.DeleteFirstNode();
		isMoving[characterIndex] = true;
		Sleep(150);
	}
	else
	{
		isMoving[characterIndex] = false;
		MarkAsObstacle(true);
	}
}

void Character::MarkAsObstacle(bool isObstacle)
{
	int value = playerFieldValue;
	if (AI)
		value = agantsFieldValue;

	if (isObstacle)
	{
		grid[position.y][position.x] = value;
	}
	else
	{
		grid[position.y][position.x] = 0;
	}
}

void Character::MarkAsDead() 
{
	isDead = true;
	//TODO: podmienić teksturkę
	image.texture = SDL_CreateTextureFromSurface(renderer, IMG_Load(playerDeadPath));
	grid[position.y][position.x] = 255;
}

void Character::Resurect() 
{
	isDead = false;
	//TODO: podmienić teksturkę
	image.texture = SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath));
	int value = playerFieldValue;
	if (AI)
		value = agantsFieldValue;
	grid[position.y][position.x] = value;
}

void Character::TakeDamage(float damageValue) 
{
	if (isDead)
		return;
	//float tempHealth = health - damage;
	totalHealth = totalHealth - damageValue;

	int killedEntity = (int)(damageValue / health);
	entityCount -= killedEntity;
	if (entityCount * health > totalHealth)
		entityCount--;


	if (totalHealth <= 0.0f || entityCount <= 0) 
	{
		//printf("Die KURWA!");
		MarkAsDead();
		Die();
	}

}

void Character::Die() 
{
	if (AI)
		aliveAgentsCount--;
	else
		alivePlayerCharactersCount--;
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

TTF_Font* GetFont() 
{
	if (TTF_Init() < 0)
		abort();
	TTF_Font* font = TTF_OpenFont(fontPath, fontSize);
	if (font)
		return font;
	else
		abort();
}

#pragma endregion

Queue GetPathGrassfire(Vector2i startPos, Vector2i endPos)
{
	Queue searchQueue, path;
	uchar gridCopy[gridHeight][gridWidth];
	for (int i = 0; i < gridHeight; i++)
	{
		for (int j = 0; j < gridWidth; j++)
		{
			gridCopy[i][j] = grid[i][j];
		}
	}

	if (grid[endPos.y][endPos.x] == 255 || (grid[endPos.y][endPos.x] == playerFieldValue && isPlayerRound)) //field is marked as obstacle or player wants attack own character
		return path;

	gridCopy[endPos.y][endPos.x]++;

	searchQueue.AddNode(endPos);

	while (!searchQueue.IsEmpty() && path.IsEmpty())
	{
		const Vector2i currentPosition = searchQueue.FirstNode->position;
		searchQueue.DeleteFirstNode();

		uchar next_value = gridCopy[currentPosition.y][currentPosition.x] + 1;

		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				if (i != j && i * j == 0)
				{
					Vector2i neighbour = { currentPosition.x + j, currentPosition.y + i };
					if (neighbour.x == startPos.x && neighbour.y == startPos.y)
					{
						path.AddNode(neighbour);
						gridCopy[neighbour.y][neighbour.x] = next_value;
					}
					else if (neighbour.y > 0 && neighbour.y < gridHeight && neighbour.x > 0 && neighbour.x < gridWidth &&
						gridCopy[neighbour.y][neighbour.x] == grid[neighbour.y][(int)neighbour.x] && gridCopy[neighbour.y][neighbour.x] != 255 && gridCopy[neighbour.y][neighbour.x] != 200 && gridCopy[neighbour.y][neighbour.x] != 100)
					{
						searchQueue.AddNode(neighbour); // add point to check-list 
						gridCopy[neighbour.y][neighbour.x] = next_value;
					}
				}
			}
		}
	}

	searchQueue.Clear();
	DrawMap(gridCopy);

	if (!path.IsEmpty())
	{

		Vector2i currentPos = path.FirstNode->position;
		int next_value = gridCopy[currentPos.y][currentPos.x] - 1;
		while (next_value > gridCopy[endPos.y][endPos.x])
		{
			for (int i = -1; i <= 1; i++)
				for (int j = -1; j <= 1; j++)
					if (i * j == 0 && i != j)
					{
						Vector2i neighbour{ currentPos.x + i, currentPos.y + j };
						if (neighbour.x >= 0 && neighbour.y >= 0 && neighbour.x < gridWidth && neighbour.y < gridHeight)
						{
							if (gridCopy[neighbour.y][neighbour.x] == next_value)
							{
								path.AddNode(neighbour);
								currentPos = neighbour;
								next_value--;
							}
						}
					}
		}
		path.AddNode(endPos);
	}
	return path;
}

Vector2i CastToGridPosition(Vector2i mousePosition)
{
	return Vector2i(mousePosition.x / (screenWidth / gridWidth), mousePosition.y / (screenHight / gridHeight));
}

int GetRandomIndex(int value)
{
	srand(time(nullptr));
	int random = rand() % value;
	return random;
}

Vector2i GetRandomGrid()
{
	int randomX = GetRandomIndex(gridWidth);
	int randomY = GetRandomIndex(gridHeight);
	while (grid[randomY][randomX] == 255 || grid[randomY][randomX] == 100 || grid[randomY][randomX] == 200)
	{
		randomX = GetRandomIndex(gridWidth);
		randomY = GetRandomIndex(gridHeight);
	}
	return Vector2i(randomX, randomY);
}

void GetTargetReferenceAndTakeDamage(Character* invoker, Vector2i position, Character** characters, float damage, int count) 
{
	int i;
	for (i = 0; i < 8; i++)
	{
		if (characters[i]->position.x == position.x && characters[i]->position.y == position.y) 
		{
			characters[i]->TakeDamage(damage * count);
			break;
		}
	}
	if (!characters[i]->isDead) 
	{
		invoker->TakeDamage(characters[i]->damage * characters[i]->entityCount);
	}
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

//float CalculateDamage(float characterDamege, bool AI) 
//{
	//if(AI)
		//return characterDamege * aliveAgentsCount;
	//return characterDamege * alivePlayerCharactersCount;
//}

int main()
{
	Vector4 backgroundColor = Vector4(3, 34, 48, 255);

	SDL_Window* window = GetWindow();
	renderer = GetRenderer(window);
	SDL_Texture* textTexture;
	SDL_Surface* textSurface;
	TTF_Font* font = GetFont();

	if (!InitializeSDL(renderer, backgroundColor))
		return -1;

	textSurface = TTF_RenderText_Solid(font, "Alive Player Champions: ", { 255, 255, 255 });
	Vector2i textSizeS = Vector2i(textSurface->w, textSurface->h);
	textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

	Vector2i screenSize(screenWidth / gridWidth, screenHight / gridHeight);

	Character championA = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 3), 0, 10.0f, 5.0f, false, 5, textTexture, textSizeS, font);
	Character championB = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 4), 1, 10.0f, 5.0f, false, 5, textTexture, textSizeS, font);
	Character championC = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 5), 2, 10.0f, 5.0f, false, 5, textTexture, textSizeS, font);
	Character championD = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 6), 3, 10.0f, 5.0f, false, 5, textTexture, textSizeS, font);
	Character championE = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 7), 4, 10.0f, 5.0f, false, 4, textTexture, textSizeS, font);
	Character championF = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 8), 5, 10.0f, 5.0f, false, 4, textTexture, textSizeS, font);
	Character championG = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 9), 6, 10.0f, 5.0f, false, 4, textTexture, textSizeS, font);
	Character championH = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load(defaultCharacterSpritePath)), screenSize, Vector2i(0, 10), 7, 10.0f, 5.0f, false, 4, textTexture, textSizeS, font);

	Character championAIA = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 2), 8, 10.0f, 3.0f, true, 4, textTexture, textSizeS, font);
	Character championAIB = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 3), 9, 10.0f, 3.0f, true, 4, textTexture, textSizeS, font);
	Character championAIC = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 4), 10, 10.0f, 3.0f, true, 4, textTexture, textSizeS, font);
	Character championAID = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 5), 11, 10.0f, 3.0f, true, 4, textTexture, textSizeS, font);
	Character championAIE = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 6), 12, 10.0f, 3.0f, true, 5, textTexture, textSizeS, font);
	Character championAIF = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 7), 13, 10.0f, 3.0f, true, 5, textTexture, textSizeS, font);
	Character championAIG = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 8), 14, 10.0f, 3.0f, true, 5, textTexture, textSizeS, font);
	Character championAIH = Character(SDL_CreateTextureFromSurface(renderer, IMG_Load("space-game.png")), screenSize, Vector2i(14, 9), 15, 10.0f, 3.0f, true, 5, textTexture, textSizeS, font);

	int amountPlayer = 8;
	int amountAI = 8;
	int playerIndex = 0;
	int AIIndex = 0;

	Character* playerChampions[8] = { &championA, &championB , &championC ,&championD ,&championE ,&championF ,&championG ,&championH };
	Character* agents[8] = { &championAIA, &championAIB ,&championAIC ,&championAID ,&championAIE ,&championAIF ,&championAIG, &championAIH };

	playerCharactersPointer = playerChampions;
	agentsPointer = agents;


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
			ProcessEvents(&sdlEvent, &done, playerChampions, &playerIndex, amountPlayer);
		}

		for (int i = 0; i < 8; i++)
		{
			playerChampions[i]->Move();
			agents[i]->Move();
		}


		if (!isPlayerRound && !IsMoving() && aliveAgentsCount > 0)
		{
			while (aliveAgentsCount > 0 && agents[AIIndex % amountAI]->isDead)
				AIIndex++;

			//TODO: randomowy indeks w characterach gracza, sprawdź czy nie jest martwy, weź jego pozycje i wywołaj Grassfire
			Vector2i dest(10, 10);
			if (GetRandomIndex(10) > 5) //Get Random Position Behavior
			{
				dest = GetRandomGrid();
			}
			else //Attack Behavior
			{
				int index = GetRandomIndex(8);
				dest = playerChampions[index]->position;
			}

			agents[AIIndex % amountAI]->MoveInit(dest);
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

		//textSurface = TTF_RenderText_Solid(font, "Alive Player Champions: ", { 255, 255, 255 });
		//textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
		//SDL_Rect dest = { 0, 0, textSurface->w, textSurface->h };
		//SDL_RenderCopy(renderer, textTexture, NULL, &dest);

		//textSurface = TTF_RenderText_Solid(font, "Alive Agents: ", { 255, 255, 255 });
		//textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
		//dest = { 1600, 0, textSurface->w, textSurface->h };
		//SDL_RenderCopy(renderer, textTexture, NULL, &dest);

		obstacleA.Render(renderer, Vector2i(obstacleAPos.x, obstacleAPos.y));
		obstacleB.Render(renderer, Vector2i(obstacleBPos.x, obstacleBPos.y));
		obstacleC.Render(renderer, Vector2i(obstacleCPos.x, obstacleCPos.y));
		obstacleD.Render(renderer, Vector2i(obstacleDPos.x, obstacleDPos.y));

		// Showing the screen to the player
		SDL_RenderPresent(renderer);

		// next frame...
	}

	SDL_DestroyTexture(textTexture);
	SDL_FreeSurface(textSurface);
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


void ProcessEvents(SDL_Event* event, bool* done, Character** player, int* playerIndex, int amountPlayer)
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
				while (isPlayerRound && alivePlayerCharactersCount > 0 && player[*playerIndex % amountPlayer]->isDead)
					*playerIndex += 1;
				int x, y;
				SDL_GetMouseState(&x, &y);
				player[*playerIndex % amountPlayer]->MoveInit(CastToGridPosition(Vector2i(x, y)));
				*playerIndex += 1;
				isPlayerRound = false;
			}
			break;
		}
		default:
			break;
		}
	}
}
