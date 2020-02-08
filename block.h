#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifndef _BLOCK_H
#define _BLOCK_H

/*
#define RESET "\x1B[0m"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
*/

#define Block_cell_count 4
#define Block_model_count 4


class Block {
	public:
		Block(int _r_axis, int _c_axis, int _type);
		int r_axis, c_axis, type, model;
		static char ch_color[8][16];
		static int Tetrominoes[8][4][4][2]; 
		static int Drop_hitbox_count[8][4];
		static int Drop_hitbox[8][4][4][2];
		static int Left_hitbox_count[8][4];
		static int Left_hitbox[8][4][4][2];
		static int Right_hitbox_count[8][4];
		static int Right_hitbox[8][4][4][2];
};
#endif
