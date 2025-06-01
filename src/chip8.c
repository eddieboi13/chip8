#include "chip8.h"
SDL_Scancode chip8_to_sdl[16] = {
    SDL_SCANCODE_X,  // 0
    SDL_SCANCODE_1,  // 1
    SDL_SCANCODE_2,  // 2
    SDL_SCANCODE_3,  // 3
    SDL_SCANCODE_Q,  // 4
    SDL_SCANCODE_W,  // 5
    SDL_SCANCODE_E,  // 6
    SDL_SCANCODE_A,  // 7
    SDL_SCANCODE_S,  // 8
    SDL_SCANCODE_D,  // 9
    SDL_SCANCODE_Z,  // A
    SDL_SCANCODE_C,  // B
    SDL_SCANCODE_4,  // C
    SDL_SCANCODE_R,  // D
    SDL_SCANCODE_F,  // E
    SDL_SCANCODE_V   // F
};
uint16_t fetch(uint8_t *memory,  uint16_t *PC){
	uint16_t val = memory[*PC] << 8 | memory[*PC+1];
	return val;
}

void decode(uint16_t val, uint16_t *PC, uint16_t *Stack, uint8_t *SP, uint16_t *I, uint8_t *delay, uint8_t *sound, uint8_t *V, uint8_t *memory, SDL *sdl, uint32_t *framebuffer){
	uint8_t X = (val & 0x0F00) >> 8;
	uint8_t Y = (val & 0x00F0) >> 4;
	uint8_t NN = val & 0x00FF;
	
	//declaring these variable within the case statements caused some issues
	int posX;
    int posY;
	static bool waiting_for_key = false;
    static uint8_t key_reg = 0;
	
	//debugging info
	printf("opcode: %x\n", val);
    printf("V[X]: %x\n",V[X]);
    printf("V[Y]: %x\n",V[Y]);
    printf("PC: %x\n", *PC);
    printf("SP: %x\n", *SP);

	switch(val & 0xF000){ //check for left most nibble to get instruction type
		case 0x0000:
            if((val & 0x00FF) == 0x00E0){ //clear the screen
				if(SDL_RenderClear(sdl->render)){
					fprintf(stderr, "Error clearing screen: %s", SDL_GetError());
				}
				memset(framebuffer, 0, 64*32*sizeof(uint32_t));
			}
			else if((val & 0x00FF) == 0x00EE){ //pops instruction of the stack
				if(*SP > 0){	
					*SP-=1;
					*PC = Stack[*SP];
					Stack[*SP] = 0;
				}
			}
            break;
		case 0x1000://1NNN
            *PC = (val & 0x0FFF);//sets PC to NNN
			break;
		case 0x2000://2NNN
			if(*SP < 16){
				Stack[*SP] = *PC + 2; //Adds instruction to stack
				*SP+=1;
            	*PC = (val & 0x0FFF); //Sets PC to NNN
			}
			break;
		case 0x3000: //3XNN
            if(V[X] == NN){ //skips following instruction if value at register VX is equal to NN
				*PC += 2;
			}
            break;
		case 0x4000: //4XNN
            if(V[X] != NN){ //skips following instruction if value at register VX != NN
				*PC += 2;
			}
			break;
		case 0x5000: //5XYN
            if(V[X] == V[Y]){ //skips following instruction if value at VX == VY
				 *PC += 2;
			}
            break;
		case 0x6000: //6XNN
            V[X] = NN; //sets value at VX equal to NN
			break;
		case 0x7000: //7XNN
            V[X] = V[X] + NN; //adds NN to value at VX
            V[X] = (V[X]& 0xFF);
			break;
		case 0x8000://8XYN
			switch(val & 0x000F){ 
				case 0x0:	
					V[X] =  V[Y];
					break;
				case 0x1:
					V[X] = (V[X] | V[Y]);
					break;
				case 0x2:
					V[X] = (V[X] & V[Y]);
					break;
				case 0x3:
					V[X] = (V[X] ^ V[Y]);
					break;
				case 0x4:
                	if(V[X] + V[Y] > 0XFF){//checks if overflow occurs and sets VF to 1 if it does
						V[0xF] = 0x01;
					}
					else{
						V[0xF] = 0x00;
					}
					V[X] = (V[X] + V[Y]);
                	break;
				case 0x5:
                	if(V[X] < V[Y]){//checks for overflow in subtraction and sets VF equal to 1 if there's overflow
                    	V[0xF] = 0x00;
                	}
                	else{
                    	V[0xF] = 0x01;
                	}
					V[X] = (V[X] - V[Y]) & 0xFF;
                	break;
				case 0x6:
					V[X] = (V[Y]>>1);//sets VX equal to VY shifted two the right 1 bit
        			V[0XF] = (V[Y] & 1); //stores the rightmost bit before the shift in VF
					break;
				case 0x7:
					if(V[X] < V[Y]){
            			V[0xF] = 0x00;
        			}
        			else{
            			V[0xF] = 0x01;
        			}
        			V[X] = V[Y] - V[X];
					break;
				case 0xE:
					V[X] = (V[Y]<<1);
					V[0xF] = (V[Y] & 1);
					break;
			}
			break;
		case 0x9000:
            if(V[X] != V[Y]){ //skips next instruction if VX's value doesn't equal VY's value
				*PC+= 2;
			}
            break;
		case 0xA000://ANNN
            *I = val & 0x0FFF; //updates I's value to be 0x0NNN
			break;
        case 0xB000://BNNN
            *PC = V[0] + (val & 0xFFF);//jumps to V0 + NNN
            break;
		case 0xC000://CXNN
            V[X] = ((rand() % 256) & NN);//sets VX equal to a random number with a bit mask of NN
            break;
		case 0xD000: //DXYN
			//account for reacharound
			posX = V[X] % 64;
    		posY = V[Y] % 32;

    		V[0xF] = 0;  // Reset collision flag
			for (int i = 0; i < (val & 0x000F); i++) {  // N rows
        		uint8_t sprite_data = memory[*I + i];
        		if (posY + i >= 32) break;
				for (int j = 0; j < 8; j++) {  // 8 bits per row
					bool bitj = (sprite_data & (0x80 >> j)) != 0;
            		int x = (posX + j) % 64;
            		int y = (posY + i) % 32;
            		int index = y * 64 + x;
            		if(bitj) {
                		if(framebuffer[index] == 0xFFFFFFFF) {  // If pixel was already set
                    		V[0xF] = 1;  // Collision detected
                		}
                		framebuffer[index] ^= 0xFFFFFFFF;  // Toggle pixel
            		}
        		}
    		}
    		break;
		case 0xE000:	
			if(NN == 0x9E){ //skips next instruction if key pressed is equal to value stored at VX
				const uint8_t *keystate = SDL_GetKeyboardState(NULL);
				if (keystate[chip8_to_sdl[V[X]]]) {
    				*PC += 2;  
				}
			}
			else {
            	const uint8_t *keystate = SDL_GetKeyboardState(NULL);
                if (!keystate[chip8_to_sdl[V[X]]]) {
                    *PC += 2;  
                }
			}
            break;
		case 0xF000:
			switch(NN){
				case 0x07:
					V[X] = *delay;
					break;
				case 0x0A:
    				if (!waiting_for_key) {
        				waiting_for_key = true;
        				key_reg = X;
        				*PC -= 2; //repeat this instruction
    				}
					else{
        				const uint8_t *keystate = SDL_GetKeyboardState(NULL);
        				for (int i = 0; i < 16; i++) {
            				if(keystate[chip8_to_sdl[i]]) {
                				V[key_reg] = i;
                				waiting_for_key = false;
                				break;
            				}
        				}
        				if(waiting_for_key) {
            				*PC -= 2; //repeat instruction
        				}
    				}
					break;
				case 0x15:
					*delay = V[X];
                	break;
				case 0x18:
					*sound = V[X];
					break;
				case 0x1E:
					*I+=V[X];
					break;
				case 0x29:
					*I = 0x50 + (V[X] * 5);//sets I equal to memory address w/ VX's corresponding font
					break;
				case 0x33:
					memory[*I] = V[X] / 100;
    				memory[*I + 1] = (V[X] / 10) % 10;
    				memory[*I + 2] = V[X] % 10;
					break;
				case 0x55:
					for(int i = 0; i <= X; i++){
						memory[(*I)+i] = V[i]; //save V0 to VX in memory[I -> I+x]
					}
					*I+=X+1;
					break;
				case 0x65:
					for(int i = 0; i <= X; i++){
                        V[i] = memory[(*I)+i];//loads V0 to VX from memory[I -> I+x]
                    }
					*I+=X+1;
					break;
			}
			break;
	}
	//update the PC if the instruction allows for it
	if ((val & 0xF000) != 0x1000 &&  // Not JP
    	(val & 0xF000) != 0x2000 &&  // Not CALL
    	(val != 0x00EE) &&  // Not RET
    	(val & 0xF000) != 0xB000) {  // Not BNNN (JP V0 + NNN)
    		*PC += 2;
	}

}
//highkey redundant but wtv (idrk what i was thinking when i did this, couldv'e just added the font directly into memeory)
void initFont(uint8_t *memory, char *name){ //initializing font data in memory
FILE *file = fopen(name, "rb");
    if(file == NULL){
        printf("Error opening file");
        return;
    }
	fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    char *buffer = calloc(file_size, 1);
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return;
    }
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        if (feof(file)) {
            fprintf(stderr, "Error: Unexpected end of file.\n");
        }
        else if (ferror(file)) {
            perror("Error reading file init");
        }
        free(buffer);
        fclose(file);
        return;
    }

    printf("Raw data read from file:\n");
    for (size_t i = 0; i < bytes_read; i++) {
        memory[i+80] = (unsigned char)buffer[i];
    }

        free(buffer);
        fclose(file);	
}
//loads rom into memory
void load(uint8_t *memory, char* name){
	FILE *file = fopen(name, "rb");
	if(file == NULL){
		printf("Error opening file");
		return;
	}
	fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);	
	char *buffer = calloc(file_size, 1);
    if (buffer == NULL) {
    	perror("Memory allocation error");
        fclose(file);
        return;
    }
	size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
		if (feof(file)) {
        	fprintf(stderr, "Error: Unexpected end of file.\n");
        } 
		else if (ferror(file)) {
        	perror("Error reading file load");
        }
        free(buffer);
        fclose(file);
        return;
    }

    printf("Raw data read from file:\n");
    for (size_t i = 0; i < bytes_read && (i + 512) < 4096; i++) {
		memory[i+512] = (unsigned char)buffer[i];
    }

        free(buffer);
        fclose(file);
}
//updates sound and delay timers
void timers(uint8_t *delay, uint8_t *sound){
	if(*delay > 0){
		*delay-=1;
	}
	if(*sound > 0){
		*sound-=1;
	}

}
