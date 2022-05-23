#pragma once

struct Queue;
struct Vector4;
struct Character;
struct Vector2;
struct Image;
int main();
void DrawObstacles(unsigned char** arr, Image* RedBlock, SDL_Renderer* renderer);
SDL_Window* GetWindow();
void ProcessEvents(SDL_Event* sdl_event, bool* done, Character* player, int* PIndex);
Queue GetPathGrassfire(Vector2 start_pos, Vector2 end_pos);
