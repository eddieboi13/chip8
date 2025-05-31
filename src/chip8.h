#include <stdio.h>
#include <stdint.h>
//#include <conio.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <string.h>
#define WIDTH 640
#define HEIGHT 320
#define WINDOW_TITLE "chip8"
uint8_t memory[4096];
uint8_t V[16];
uint16_t PC;
uint16_t I;
uint8_t SP;
uint16_t Stack[16];
uint8_t delay;
uint8_t sound;
typedef struct{
	SDL_Window *window;
	SDL_Renderer *render;
	SDL_Texture *texture;
}SDL;

//chip8.c functions
uint16_t fetch(uint8_t *memory,  uint16_t *PC);
void decode(uint16_t val, uint16_t *PC, uint16_t* Stack, uint8_t *SP, uint16_t *I, uint8_t *delay, uint8_t *sound, uint8_t *V, uint8_t *memory, SDL *sdl, uint32_t *framebuffer);
void initFont(uint8_t *memory, char* fontData);
void load(uint8_t *memory, char* name);
void timers(uint8_t *delay, uint8_t *sound);

//sdl functions located in main
bool initSDL(SDL *sdl);
void cleanUpSDL(SDL *sdl, int exit_status);
