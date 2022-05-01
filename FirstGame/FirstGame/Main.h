#pragma once

struct Player;
struct Vector2;
struct Vector4;
struct Stack;
struct Obstacle;
int main();
void MoveTo(Player& ship, Stack& path, SDL_Renderer* renderer);
void FindPath(Stack* currentCheck, unsigned char** arr, Stack* nextCheck, Stack& path, bool& PathGenerate);
void DrawObstacles(unsigned char** arr, Obstacle& RedBlock, SDL_Renderer* renderer);
SDL_Window* GetWindow(SDL_Window* window);
void ProcessEvents(SDL_Event& sdl_event, bool& done, Stack* CheckNow, Player& Ship, Vector2& GoalPosition, int WidthPixels, int HeightPixels, unsigned char** arr, bool& finding);
//void MoveToPosition(int* destinationX, int* destinationY, int* currentX, int* currentY);
//int Lerp(float a, float b, float t);
