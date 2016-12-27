#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h> //for rand

#include "chip.h"

using namespace std;

void init(){
	program_pointer = 0x200;//start at 0x200
	stack[0] = program_pointer; 
	srand (time(NULL)); // seed random
}

int main(){
	string str;

	cout << "Starting\n";
	
	init();

	ifstream file;
	file.open("PONG",ios::in|ios::binary|ios::ate);
	int size = file.tellg();
	printf("%i\n",size);
	file.seekg (0, ios::beg);
	char buffer[size];
	file.read (buffer, size);
	memcpy(&memory[0x200],&buffer,sizeof(buffer));
	memcpy(&memory[FONT_ADDR],&chip8_fontset,sizeof(chip8_fontset));
	//disasemble
	// for(int i=0;i<size;i++){
	// 	unsigned short op_c = (unsigned char)memory[i+0x200] << 8 | (unsigned char)memory[i+1+0x200];
	// 	//printf(" %04X\n",op_c);
	// 	if(!skip_op){
	// 		execute_opcode(op_c);
	// 	}
	// 	skip_op = false;
	// 	i++;
		
	// }
	while(true){
		unsigned short op_c = (unsigned char)memory[program_pointer] << 8 | (unsigned char)memory[program_pointer+1];
		//print_opcode(op_c);
		execute_opcode(op_c);
		if(program_pointer<GAME_ADDR){
			printf("Invalid Memory Access\n");
			exit(1);
		}
		decrement_delay_timer();
		
	}
	printf("\n");
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
					printf("RET\n");
					stack_pointer--;
					program_pointer = stack[stack_pointer];
					
					break;
				}
			}
			break;
		}
		case 0x1000:		//1000
		{					//go to NNN
			unsigned short to_addr = op_c & 0x0FFF;
			program_pointer = to_addr;
			inc_pointer = false;
			printf("JMP to %02x\n",to_addr );
			break;
		}
		case 0x2000:		//2NNN
		{					//*(0xNNN)() call subroutine at NNN
			unsigned short to_addr = op_c & 0x0FFF;
			//need to know where program is at now
			stack[stack_pointer] = program_pointer;
			print_opcode(program_pointer);
			stack_pointer++;
			program_pointer = to_addr;
			inc_pointer = false;
			if(stack_pointer > 16){
				printf("ERROR stack to big\n");
				exit(1);
			}
			printf("!CALL push on stack ( %u), go to %02X\n",stack_pointer,to_addr );
			printf("%0x , %0x\n",memory[program_pointer],memory[program_pointer+1]);
			print_opcode(program_pointer);
			break;
		}

		case 0x3000:		//3XNN
		{					//if(Vx==NN) skip next intruction
			unsigned int x = byte_two(op_c);
			unsigned char val = op_c & 0x00FF;
			printf("SE V[%i] (%02x) == %02x \n",x,V[x],val);
			if(V[x] == val){
				program_pointer +=2;
				printf("skippig next op\n");
			}
			break;
		}
		case 0x4000:		//4XNN
		{					//if(Vx!=NN) skip next intruction
			unsigned int x = byte_two(op_c);
			unsigned char val = op_c & 0x00FF;
			printf("SNE V[%i] (%02x) == %02x \n",x,V[x],val);
			if(V[x] != val){
				program_pointer +=2;
				printf("skippig next op\n");
			}
			break;
		}
		case 0x6000:		//6XNN
		{					//Vx = NN
			unsigned int x = (byte_two(op_c));
			unsigned char val = op_c & 0x00FF;
			V[x] = val;
			printf("LD settting V[%i] to %02X\n",x,val);
			break;
		}
		case 0x7000:		//7XNN
		{					//Vx += NN
			unsigned int x = byte_two(op_c);
			unsigned int val = op_c & 0x00FF;
			printf("ADD %u to %u (V[%u])\n", val,V[x],x);
			V[x] += val;
			break;
		}
		case 0xA000:		//ANNN
		{					//I = NNN
			unsigned short mem = op_c & 0x0FFF;
			I = mem;
			printf("LD settting I to %04x\n",mem);
			break;
		}
		case 0xC000:		//CXNN "Random"
		{					//V[x] == rand()&NN
			unsigned int x = byte_two(op_c);
			unsigned int r = rand_255();
			unsigned int final = r & (op_c & 0x00FF);
			V[x] = final;
			printf("RND Using random: %u to set v[%u] to %u\n",r,x,final);

		}
		case 0xD000:		//DXYN
		{					//draw(Vx,Vy,N) , n is height, 8xn
			printf("DRW() at %04x\n",op_c);
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
					printf("GDLY v[%u] set to %u\n",x,delay_timer );
					break;
				}
				case 0x0015:	//FX15
				{	//set delay timer to v[x]
					unsigned int x = byte_two(op_c);
					printf("SDLY Set delay Timer to V[%u]\n",x);
					delay_timer = V[x];
					break;
				}
				case 0x0018:	//FX15 SOUND
				{	//set delay timer to v[x]
					unsigned int x = byte_two(op_c);
					printf("SNDT Set sound Timer to V[%u]\n",x);
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


					printf("BCD  V[%u] (%u)\n",x,temp);
					for(int i=2;i>=0;--i){
						
						printf("\t%u\n",temp%10);
						memory[I+i] = temp;
						temp /= 10;
					}
					
					break;
				}
				case 0x0029:	//FX29 load font to mem
				{				//I=sprite_addr[Vx]
					unsigned int x = byte_two(op_c);
					I = memory[FONT_ADDR + V[x]];
					printf("LD I:%02x V[%u] (%02x) \n",I,x,V[x]);
					break;
				}
				case 0x0065:	//FX29 load font to mem
				{				//I=sprite_addr[Vx]
					unsigned int x = byte_two(op_c);
					for(int i=0;i<=x;++i){
						V[i] = memory[I+i];
					}
					printf("LD I:%02x V[0-%u] \n",I,x);
					break;
				}
				default:
				{
				printf("!F %04X , %04X\n" , op_c,(op_c&0xF000));
				}
				
			}

		break;
		}//end 0xF000
		case 0x8000: //bit and math ops
		{
			switch (op_c & 0x000F)
			{
				case 0x0001:	//8XY2 "|"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					printf("OR  %02x , x: %02x y: %02x\n",op_c, x,y);
					V[x] = V[x] | V[y];
					break;
				}
				case 0x0002:	//8XY2 "&"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					printf("AND %02x , x: %02x y: %02x\n",op_c, x,y);
					V[x] = V[x] & V[y];
					break;
				}
				case 0x0003:	//8XY2 "^"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					printf("XOR %02x , x: %02x y: %02x\n",op_c, x,y);
					V[x] = V[x] ^ V[y];
				}
				case 0x0004:	//8XY2 "+"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					printf("ADD %02x , x: %02x y: %02x\n",op_c, x,y);
					unsigned int sum = V[x] + V[y];
					if(sum > 0xFF){
						V[0x0F] = 1;
					} else {
						V[0x0F] = 0;
					}
					V[x] = sum;
					break;
				}
				case 0x0005:	//8XY2 "-"
				{				//Vx=Vx&Vy
					
					unsigned int x = byte_two(op_c);
					unsigned int y = byte_three(op_c);
					printf("SUB %02x , x: %02x y: %02x\n",op_c, x,y);
					unsigned int diff = V[x] - V[y];
					if(V[y]>V[x]){
						V[0x0F] = 1;
					} else {
						V[0x0F] = 0;
					}
					V[x] = diff;
					break;
				}

			}
		break;
		}//end 0x8000
		default:
		{
			printf("! %04X , %04X\n" , op_c,(op_c&0xF000));
		}
	}
	if(inc_pointer){
		program_pointer+=2;
	}
	return 0;
}

void decrement_delay_timer(){
	if(delay_timer>0){
		delay_timer--;
	}
}