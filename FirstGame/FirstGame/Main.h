#pragma once

struct Queue;
struct Vector4;
struct Character;
struct Vector2i;
struct Image;
int main();
void DrawObstacles(unsigned char** arr, Image* RedBlock, SDL_Renderer* renderer);
SDL_Window* GetWindow();
void ProcessEvents(SDL_Event* sdl_event, bool* done, Character** player, int* playerIndex, int amountPlayer);
Queue GetPathGrassfire(Vector2i start_pos, Vector2i end_pos);
//float CalculateDamage(float characterDamage, bool AI);
void GetTargetReferenceAndTakeDamage(Character* invoker, Vector2i position, Character** characters, float damage, int count);
char* CastToArray(int number);
