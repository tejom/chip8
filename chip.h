#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h> //for rand

using namespace std;

//debug
bool wait_for_input = false;
const bool diss = true;
#define OPCODE_DEBUG true
//

#define FONT_ADDR 0x00
#define GAME_ADDR 0x0200

#define byte_two(x) x >> 8 & 0x000F
#define byte_three(x) x >> 4 & 0x000F
#define rand_255() rand() %255




#if OPCODE_DEBUG
  #define opcode_debug(...) printf(__VA_ARGS__);
  #define video_debug(...) printf(__VA_ARGS__);
  #define print_opcode(c) printf("\tOPCODE %02x\n",c);
#else
  #define opcode_debug(...);
  #define print_opcode(c);
#define video_debug(...);
#endif

int execute_opcode(unsigned short);

void decrement_delay_timer();

void draw(unsigned short x,unsigned short y,unsigned short n);

void key_handler(char c);

void clear_key_states();

//variables for the machine
unsigned char memory[4096];

unsigned char V[16]; //16 registers , 16th for CF

unsigned char delay_timer =0;
unsigned char sound_timer =0;

unsigned short stack[16]; 
unsigned short stack_pointer = 0; //index of stack
unsigned short program_pointer;

unsigned short I = 0;

unsigned char view[65][65] = {0};

unsigned char key[16] = {0};

//fonts
unsigned char chip8_fontset[80] = //stolen from internet
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

//keyboard
fd_set rfds;
struct timeval tv;
struct termios old_tio, new_tio;
