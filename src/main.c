#include "chip8.h"
int main(int argc, char *argv[]){
	char name[BUFSIZ];
	uint32_t framebuffer[64*32] = {0};
	if(argc >= 2){
		strcpy(name, argv[1]);
	}
	SDL sdl = {
		.window = NULL,
		.render = NULL,
		.texture = NULL,
	};
	initFont(memory, "roms/chip8_font.bin");
	char newname[BUFSIZ] = "roms/";
	strcat(newname, name);
	load(memory, newname);
	if(initSDL(&sdl)){
		cleanUpSDL(&sdl, EXIT_FAILURE);
	}
	bool quit = false;
	PC = 0x200;
	I = 0x50;
	SP = 0;
	sound = 0;
	delay = 0;
	uint16_t val;
	while (!quit) {
		val = fetch(memory, &PC);
        decode(val, &PC, Stack, &SP, &I, &delay, &sound, V, memory, &sdl, framebuffer);
		SDL_UpdateTexture(sdl.texture, NULL, framebuffer, 64 * sizeof(uint32_t));
    	SDL_RenderClear(sdl.render);
    	SDL_RenderCopy(sdl.render, sdl.texture, NULL, NULL);
    	SDL_RenderPresent(sdl.render);
		SDL_Event e;
		if(sound > 0 || delay > 0){
			SDL_Delay(16);
        	timers(&delay, &sound);
		}
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
	}
	cleanUpSDL(&sdl, EXIT_SUCCESS);
	return 0;
}


bool initSDL(SDL *sdl){
	if(SDL_Init(SDL_INIT_EVERYTHING)){
		fprintf(stderr,"SDL Initialization failed: %s\n", SDL_GetError());
		return true;
	}
	sdl -> window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if(!sdl->window){
		fprintf(stderr,"SDL Initialization failed: %s\n", SDL_GetError());
        return true;
	}
	sdl->render = SDL_CreateRenderer(sdl->window, 0, 0);
	if(!sdl->render){
		fprintf(stderr,"SDL Initialization failed: %s\n", SDL_GetError());
        return true;
	}
	sdl->texture = SDL_CreateTexture(sdl->render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
    if(!sdl->texture) {
        fprintf(stderr,"SDL_CreateTexture failed: %s\n", SDL_GetError());
        return true;
    }	
	return false;
}

void cleanUpSDL(SDL *sdl, int exit_status){
	SDL_DestroyRenderer(sdl->render);
	SDL_DestroyWindow(sdl->window);
	SDL_Quit();
	exit(exit_status);
}
