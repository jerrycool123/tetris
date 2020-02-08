#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "block.h"
#ifndef _BOARD_H
#define _BOARD_H
#define MAXR 20
#define MAXC 10
class Board {
	public:
		Board();
		void store_status();	// status >> old_status
		Block *generate_block();	// Generate a new block (whose type is guaranteed to be different from prev_type) and return
		void destroy_block(Block *block);	// Free memory of a block*
		bool check_status();
		/*
		   1. Clear full lines of board[player].status
		   2. Add board[player].score
		   Return value:
		   true: At least 1 line is cleared
		   false: No line is cleared
		*/
		bool block_place();	// Put current_block onto the board; success: true; failed: false
		bool block_erase();	// Erase current_block out of the board; success: true; failed: false
		bool block_drop();	// success: true; failed: false
		bool block_left();	// success: true; failed: false
		bool block_right();	// success: true; failed: false
		bool block_rotate_clockwise();	// success: true; failed: false
		bool block_rotate_counterclockwise();	// success: true; failed: false
		int score;
		int status[MAXR][MAXC];
		int old_status[MAXR][MAXC];
		Block *current_block;
		Block *next_block;
		int prev_type;	// Previous generated block type
		static int score_list[5];	// Classic tetris: {0, 40, 100, 300, 1200}
};
#endif
