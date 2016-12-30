#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h> //for rand
#include <unistd.h>
#include <termios.h>
#include <termios.h>
#include <unistd.h>

#include "chip.h"

using namespace std;

void init(){
	program_pointer = 0x200;//start at 0x200
	stack[0] = program_pointer; 
	srand (time(NULL)); // seed random
	
	//terminal set up
	//http://stackoverflow.com/questions/9547868/is-there-a-way-to-get-user-input-without-pressing-the-enter-key
    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO,&old_tio);
    /* we want to keep the old setting to restore them a the end */
    new_tio=old_tio;
    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &=(~ICANON & ~ECHO);
    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
    //end of the terminal setup

    //watch stdin for input
    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 500;
}

int main(){
	string str;
	int select_val=0;

	cout << "Starting\n";
	
	init();

	ifstream file;
	file.open("/Users/matthewtejo/CHIP-8-Emulator/roms/INVADERS",ios::in|ios::binary|ios::ate);
	int size = file.tellg();
	printf("%i\n",size);
	file.seekg (0, ios::beg);
	char buffer[size];
	file.read (buffer, size);
	memcpy(&memory[0x200],&buffer,sizeof(buffer));
	memcpy(&memory[FONT_ADDR],&chip8_fontset,sizeof(chip8_fontset));
	//disasemble
	if(diss){
		for(int i=0;i<size;i++){
			unsigned short op_c = (unsigned char)memory[i+0x200] << 8 | (unsigned char)memory[i+1+0x200];
			//printf(" %04X\n",op_c);
			printf("%02x:\t%02x\t",program_pointer,op_c );
			execute_opcode(op_c);	
			i++;
			
		}
	} else {
		while(true){
			//clear_key_states();
			unsigned short op_c = (unsigned char)memory[program_pointer] << 8 | (unsigned char)memory[program_pointer+1];
			usleep(500);
		//	delay_timer=0;
			if(wait_for_input){
				char inp;
				cin >> inp;
				delay_timer=0;
				if(inp=='i'){
					for(int i=0;i<16;i++){
						printf("\tV[%i] = %02x\n",i,V[i]);
					}
				}
				if(inp=='l'){
					printf("pointer memory at %02x [%02x : %02x]\n",program_pointer,memory[program_pointer],memory[program_pointer+1]);
				}
				if(inp=='m'){
					for(int i=0;i<255;i++){
						printf("\tMemory %02x [%02x : %02x]\n",program_pointer+i,memory[program_pointer+i],memory[program_pointer+1+i]);
						i+=1;
					}
				}
			}
				
			//user input
			select_val = select(1, &rfds, NULL, NULL, &tv);
			if(select_val == -1){
				cerr << "Select Error" << "\n";
			} else if(select_val){
				char c;			
		    	c=getchar();
				printf("Key: %d pressed\n",c);
				if(c=='p'){
					cout << "Quiting" << "\n";
					tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
					exit(0);
				}
				if(c=='m'){
					wait_for_input=true;
				}
				key_handler(c);
			} else{
			//	printf("no input\n");
				FD_SET(0, &rfds);
			}
		    

			//printf("pointer %02x\t",program_pointer);
			print_opcode(op_c);
			//printf("\tI: %02x , [%02x]\n",I,memory[I]);
						
			execute_opcode(op_c);
			
			if(program_pointer<GAME_ADDR){
				printf("Invalid Memory Access\n");
				exit(1);
			}
			
			decrement_delay_timer();

			if(sound_timer>0){
				printf("\a\n");
				sound_timer--;
			}
			
		}
	}	
	tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
	return 0;
}

int execute_opcode(unsigned short op_c){
	bool inc_pointer = true;
	switch(op_c & 0xF000) //get first byte
	{
		case 0x0000:
		{
			switch(op_c){
				case 0x00EE:
				{
					opcode_debug("RET\n");
					if(!diss){
						stack_pointer--;
						program_pointer = stack[stack_pointer];
					}
					
					break;
				}
			}
			break;
		}
		case 0x1000:		//1000
		{					//go to NNN
			unsigned short to_addr = op_c & 0x0FFF;
			if(!diss){
				program_pointer = to_addr;
				inc_pointer = false;
			}
			opcode_debug("JMP to %02x\n",to_addr );
			break;
		}
		case 0x2000:		//2NNN
		{					//*(0xNNN)() call subroutine at NNN
			unsigned short to_addr = op_c & 0x0FFF;
			//need to know where program is at now
			if(!diss){
				stack[stack_pointer] = program_pointer;
				stack_pointer++;
				program_pointer = to_addr;
				inc_pointer = false;
			}
			if(stack_pointer > 16){
				printf("ERROR stack to big\n");
				exit(1);
			}
			opcode_debug("CALL push on stack ( %u), go to %02X\n",stack_pointer,to_addr );
			break;
		}

		case 0x3000:		//3XNN
		{					//if(Vx==NN) skip next intruction
			unsigned int x = byte_two(op_c);
			unsigned char val = op_c & 0x00FF;
			opcode_debug("SE V[%i] (%02x) == %02x \n",x,V[x],val);
			if(!diss){
			if(V[x] == val){
				program_pointer +=2;
				//printf("skippig next op\n");
			}
			}
			break;
		}
		case 0x4000:		//4XNN
		{					//if(Vx!=NN) skip next intruction
			unsigned int x = byte_two(op_c);
			unsigned char val = op_c & 0x00FF;
			opcode_debug("SNE V[%i] (%02x) == %02x \n",x,V[x],val);
			if(!diss){
			if(V[x] != val){
				program_pointer +=2;
				//printf("skippig next op\n");
			}
			}
			break;
		}
		case 0x6000:		//6XNN
		{					//Vx = NN
			unsigned int x = (byte_two(op_c));
			unsigned char val = op_c & 0x00FF;
			V[x] = val;
			opcode_debug("LD settting V[%i] to %02X\n",x,val);
			break;
		}
		case 0x7000:		//7XNN
		{					//Vx += NN
			unsigned int x = byte_two(op_c);
			unsigned int val = op_c & 0x00FF;
			opcode_debug("ADD %u (%02x) to %u (V[%u])\n", val,val,V[x],x);
			V[x] += val;
			break;
		}
		case 0xA000:		//ANNN
		{					//I = NNN
			unsigned short mem = op_c & 0x0FFF;
			I = mem;
			opcode_debug("LD settting I to %04x\n",mem);
			break;
		}
		case 0xC000:		//CXNN "Random"
		{					//V[x] == rand()&NN
			unsigned int x = byte_two(op_c);
			unsigned int r = rand_255();
			unsigned int final = r & (op_c & 0x00FF);
			V[x] = final;
			opcode_debug("RND Using random: %u to set v[%u] to %u\n",r,x,final);
			break;

		}
		case 0xD000:		//DXYN
		{					//draw(Vx,Vy,N) , n is height, 8xn
			opcode_debug("DRW() at %04x\n",op_c);
			unsigned short n = op_c & 0x000F;
			unsigned short x = byte_two(op_c);
			unsigned short y = byte_three(op_c);
			if(!diss)
				draw(V[x],V[y],n);
			break;
		}
		case 0xE000:
		{
			switch(op_c & 0x00FF)
			{
				case 0x00A1:	//EXA1
				{				//skip next op if key in v[x] isnt pressed
					
					unsigned int x = byte_two(op_c);
					if(key[V[x]]==0){
						if(!diss)
							program_pointer+=2;
					}
					//printf("KEY at %02x\n",V[x] );
					break;
				}
			}
			break;
		}
		
		case 0xF000: //multiple start with 0xF000
		{
			switch(op_c &0x00FF)
			{
				case 0x0007: 	//FX07
				{				//Vx = get_delay()
					unsigned int x = byte_two(op_c);
					V[x] = delay_timer;
					opcode_debug("GDLY v[%u] set to %u\n",x,delay_timer );
					break;
				}
				case 0x0015:	//FX15
				{	//set delay timer to v[x]
					unsigned int x = byte_two(op_c);
					opcode_debug("SDLY Set delay Timer to V[%u]\n",x);
					delay_timer = V[x];
					break;
				}
				case 0x0018:	//FX15 SOUND
				{	//set delay timer to v[x]
					unsigned int x = byte_two(op_c);
					opcode_debug("SNDT Set sound Timer to V[%u]\n",x);
					sound_timer = V[x];
					break;
				}
				case 0x0033:	//FX33 BCD
				{	
					unsigned int x = byte_two(op_c);
					unsigned int temp =V[x];
					
					printf("\t%02x\n",I );
					printf("\t%02x\n",memory[I] );
					printf("\t%02x\n",memory[I+1] );
					printf("\t%02x\n",memory[I+2] );


					opcode_debug("BCD  V[%u] (%u)\n",x,temp);
					for(int i=2;i>=0;--i){
						
						printf("\t%u\n",temp%10);
						memory[I+i] = temp%10;
						temp /= 10;
					}
					
					break;
				}
				case 0x0029:	//FX29 load font to mem
				{				//I=sprite_addr[Vx]
					unsigned int x = byte_two(op_c);
					I = (FONT_ADDR + V[x]) * 0x5;
					opcode_debug("LD I:%02x V[%u] (%02x) \n",I,x,V[x]);
					break;
				}
				case 0x0065:	//FX29 load font to mem
				{				//I=sprite_addr[Vx]
					unsigned int x = byte_two(op_c);
					for(int i=0;i<=x;i++){
						printf("\tsetting V[%i] to 0x%02x\n",i,memory[I+i] );
						V[i] = memory[I+i];
					}
					opcode_debug("LD I:%02x V[0-%u] \n",I,x);
					break;
				}
				default:
				{
				opcode_debug("!F %04X , %04X\n" , op_c,(op_c&0xF000));
					break;
				}
				
			}

		break;
		}//end 0xF000
		case 0x8000: //bit and math ops
		{
			switch (op_c & 0x000F)
			{
				case 0x0000:	//8XY0	
				{				//Vx=Vy
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);

					opcode_debug("LD V[%u](%02x) V[%u](%02x)\n",x,V[x],y,V[y]);
					V[x] = V[y];
					break;
				}

				case 0x0001:	//8XY2 "|"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					opcode_debug("OR  %02x , x: %02x y: %02x\n",op_c, x,y);
					V[x] = V[x] | V[y];
					break;
				}
				case 0x0002:	//8XY2 "&"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					opcode_debug("AND %02x , x: V[%02x] (%02x) y: V[%02x] (%02x)\n",op_c, x,V[x],y,V[y]);
					V[x] = V[x] & V[y];
					break;
				}
				case 0x0003:	//8XY2 "^"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					opcode_debug("XOR %02x , x: %02x y: %02x\n",op_c, x,y);
					V[x] = V[x] ^ V[y];
					break;
				}
				case 0x0004:	//8XY4 "+"
				{				//Vx=Vx+Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					opcode_debug("ADD %02x , xV[%02x]: (%02x) yV[%02x]: (%02x)\n",op_c, x,V[x],y,V[y]);
					unsigned int sum = V[x] + V[y];
					//printf("SUM AFTER ADD %u\n",sum);
					if(sum > 0xFF){
						V[0x0F] = 1;
					} else {
						V[0x0F] = 0;
					}
					V[x] = sum;
					break;
				}
				case 0x0005:	//8XY2 "-"
				{				//Vx=Vx-Vy
					
					unsigned char x = byte_two(op_c);
					unsigned char y = byte_three(op_c);
					opcode_debug("SUB %02x , xV[%02x]: (%02x) yV[%02x]: (%02x)\n",op_c, x,V[x],y,V[y]);

					if(V[y]>V[x]){
						V[0x0F] = 0;
					} else {
						V[0x0F] = 1;
					}
					V[x] -= V[byte_three(op_c)];
					break;
				}
				default:
				{
					printf("! %04X , %04X\n" , op_c,(op_c&0xF000));
					break;
				}

			}
		break;
		}//end 0x8000
		default:
		{
			printf("! %04X , %04X\n" , op_c,(op_c&0xF000));
			break;
		}
	}
	if(inc_pointer){
		program_pointer+=2;
	}
	return 0;
}

void draw(unsigned short x,unsigned short y,unsigned short n){
	if(!wait_for_input)
		system("clear");
	V[0xF] = 0; //reset carry
	video_debug("x:%u %02x y:%u %02x n:%u %02x\n",x,x,y,y,n,n );
	for(int i=0;i<64;i++){
		printf("-");
	}
	printf("\n");
	for(unsigned int c=0;c<n;c++){
		video_debug("0x%02x,",memory[I+c]);
		for(unsigned int z=0;z<8;z++){
			unsigned char orig = view[y+c][x+z];
			view[y+c][x+z] ^= (memory[I+c] & 1 << (7-z));
			if(orig>0 && view[y+c][x+z] == 0){
				video_debug("\tPixel erased\n");
				V[0xF] = 1;
			}
		}
	}
	 printf("\n");
	for(int y=0;y<32;y++){
		for(int x=0;x<64;x++){
			if(view[y][x]){
				//printf("(%i,%i)",x,y);
				printf("x");
			} else {
				printf(" ");
			}
		}
		printf("\n");
	}
}

int ops = 60;
void decrement_delay_timer(){
	ops--;
	if(ops != 0){
		if(delay_timer>0){
			delay_timer--;
		} else {
		ops = 10;
		}
	}
}

void key_handler(char c){
	switch(c){
		case '1':
			key[0] = 1;
		case '2':
			key[1] = 1;
		case '3':
			key[2] = 1;
		case '4':
			key[3] = 1;
		case 'q':
			key[4] = 1;
		case 'w':
			key[5] = 1;
		case 'e':
			key[6] = 1;
		case 'r':
			key[7] = 1;
		case 'a':
			key[8] = 1;
		case 's':
			key[9] = 1;
		case 'd':
			key[10] = 1;
		case 'f':
			key[11] = 1;
		case 'z':
			key[12] = 1;
		case 'x':
			key[13] = 1;
		case 'c':
			key[14] = 1;
		case 'v':
			key[15] = 1;
		default:
			break;
	}

}

void clear_key_states(){
	for(int i=0;i<16;i++){
		key[i]=0;
	}
}