#include "board.h"

int Board::score_list[5] = {0, 40, 100, 300, 1200};

Board::Board() {
	srand(time(NULL));
	prev_type = 0;
	score = 0;
	current_block = generate_block();
	next_block = generate_block();
	memset(status, 0, sizeof (status));
	memset(old_status, 0, sizeof (old_status));
	if (block_place() == false) {
		fprintf(stderr, "ERROR: Board::block_place() at Board::Board()\n");
		exit(1);
	}
	/*
	fprintf(stderr, "status:\n");
	for (int i = 0; i < MAXR; ++i) {
		for (int j = 0; j < MAXC; ++j) {
			fprintf(stderr, "%d", status[i][j]);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "old_status:\n");
	for (int i = 0; i < MAXR; ++i) {
		for (int j = 0; j < MAXC; ++j) {
			fprintf(stderr, "%d", old_status[i][j]);
		}
		fprintf(stderr, "\n");
	}
	*/
}

void Board::store_status() {
	memcpy(old_status, status, sizeof (old_status));
}

Block* Board::generate_block() {
	int random_type = (rand() % 7) + 1;	// [1, 7]
	while (random_type == prev_type) {	// rand() until generate a different type
		random_type = (rand() % 7) + 1;
	}
	prev_type = random_type;
	int r_axis, c_axis;
	switch (random_type) {
		case 1:
			r_axis = -2;
			c_axis = 3;
			break;
		case 2:
			r_axis = -1;
			c_axis = 4;
			break;
		case 3:
			r_axis = -1;
			c_axis = 4;
			break;
		case 4:
			r_axis = 0;
			c_axis = 4;
			break;
		case 5:
			r_axis = -1;
			c_axis = 4;
			break;
		case 6:
			r_axis = -1;
			c_axis = 4;
			break;
		case 7:
			r_axis = -1;
			c_axis = 4;
			break;
		default:
			fprintf(stderr, "switch error in Board::generate_block(), type = %d\n", random_type);
			exit(1);
	}
	Block *new_block = new Block(r_axis, c_axis, random_type);
	return new_block;
}

void Board::destroy_block(Block *block) {
	delete block;
	return;
}

bool Board::check_status() {
	/*
	   1. Clear full lines of board[player].status
	   2. Add board[player].score
	*/
	int cleared_lines = 0, cell_count;
	for (int r = MAXR - 1; r >= 0;) {
		cell_count = 0;
		for (int c = 0; c < MAXC; ++c) {
			if (status[r][c] > 0) {
				++cell_count;
			}
			else {
				break;
			}
		}
		if (cell_count == MAXC) {	// The line is full, clear line
			++cleared_lines;
			memmove(status[1], status, sizeof (int [r][MAXC]));
			memset(status[0], 0, sizeof (status[0]));
		}
		else {
			--r;
		}
	}
	if (cleared_lines > 4) {	// Impossible
		fprintf(stderr, "ERROR: Board::check_status(), cleared_lines = %d\n", cleared_lines);
		exit(1);
	}
	else if (cleared_lines > 0){
		score += score_list[cleared_lines];
		return true;
	}
	else {	// No line is cleared
		return false;
	}
}

bool Board::block_place() {
	int type = current_block->type;
	int model = current_block->model;
	// check if the block placing is valid
	for (int i = 0; i < Block_cell_count; ++i) {
		int temp_r_axis = current_block->r_axis + Block::Tetrominoes[type][model][i][0];
		int temp_c_axis = current_block->c_axis + Block::Tetrominoes[type][model][i][1];
		if (temp_r_axis < 0 || temp_r_axis >= MAXR) {
			// Out of bound
			return false;
		}
		else if (temp_c_axis < 0 || temp_c_axis >= MAXC){
			// Out of bound
			return false;
		}
		else if (status[temp_r_axis][temp_c_axis] > 0){
			// Obstacle
			return false;
		}
	}
	// block place
	for (int i = 0; i < Block_cell_count; ++i) {
		int temp_r_axis = current_block->r_axis + Block::Tetrominoes[type][model][i][0];
		int temp_c_axis = current_block->c_axis + Block::Tetrominoes[type][model][i][1];
		status[temp_r_axis][temp_c_axis] = type;
	}
	return true;
}

bool Board::block_erase() {
	int type = current_block->type;
	int model = current_block->model;
	// check if the block erasing is valid
	for (int i = 0; i < Block_cell_count; ++i) {
		int temp_r_axis = current_block->r_axis + Block::Tetrominoes[type][model][i][0];
		int temp_c_axis = current_block->c_axis + Block::Tetrominoes[type][model][i][1];
		if (temp_r_axis < 0 || temp_r_axis >= MAXR) {
			// Impossible
			std::cerr << "block_erase error about r_axis\n";
			exit(1);
		}
		else if (temp_c_axis < 0 || temp_c_axis >= MAXC){
			// Impossible
			std::cerr << "block_erase error about c_axis\n";
			exit(1);
		}
		else if (status[temp_r_axis][temp_c_axis] == 0){
			return false;
		}
	}
	// block erase
	for (int i = 0; i < Block_cell_count; ++i) {
		int temp_r_axis = current_block->r_axis + Block::Tetrominoes[type][model][i][0];
		int temp_c_axis = current_block->c_axis + Block::Tetrominoes[type][model][i][1];
		status[temp_r_axis][temp_c_axis] = 0;
	}
	return true;
}

bool Board::block_drop() {	// success: true; failed: false
	int type = current_block->type;
	int model = current_block->model;
	if (block_erase() == false) {
		fprintf(stderr, "block_erase error in block_drop\n");
		exit(1);
	}
	++(current_block->r_axis);
	if (block_place() == false) {	// block_drop failed
		--(current_block->r_axis);	// recover
		if (block_place() == false) {
			fprintf(stderr, "block_place error in block_drop\n");
			exit(1);
		}
		return false;
	}
	return true;
}
bool Board::block_left() {	// success: true; failed: false
	int type = current_block->type;
	int model = current_block->model;
	if (block_erase() == false) {
		fprintf(stderr, "block_erase error in block_left\n");
		exit(1);
	}
	--(current_block->c_axis);
	if (block_place() == false) {	// block_left failed
		++(current_block->c_axis);	// recover
		if (block_place() == false) {
			fprintf(stderr, "block_place error in block_left\n");
			exit(1);
		}
		return false;
	}
	return true;
}
bool Board::block_right() {	// success: true; failed: false
	int type = current_block->type;
	int model = current_block->model;
	if (block_erase() == false) {
		fprintf(stderr, "block_erase error in block_right\n");
		exit(1);
	}
	++(current_block->c_axis);
	if (block_place() == false) {	// block_right failed
		--(current_block->c_axis);	// recover
		if (block_place() == false) {
			fprintf(stderr, "block_place error in block_right\n");
			exit(1);
		}
		return false;
	}
	return true;
}
bool Board::block_rotate_clockwise() {	// success: true; failed: false
	int type = current_block->type;
	int model = current_block->model;
	if (block_erase() == false) {
		fprintf(stderr, "block_erase error in block_rotate_clockwise\n");
		exit(1);
	}
	current_block->model = (current_block->model + 1) % Block_model_count; 
	if (block_place() == false) {	// block_rotate_clockwise failed
		current_block->model = (current_block->model + Block_model_count - 1) % Block_model_count; // recover
		if (block_place() == false) {
			fprintf(stderr, "block_place error in block_rotate_clockwise\n");
			exit(1);
		}
		return false;
	}
	return true;
}
bool Board::block_rotate_counterclockwise() {	// success: true; failed: false
	int type = current_block->type;
	int model = current_block->model;
	if (block_erase() == false) {
		fprintf(stderr, "block_erase error in block_rotate_counterclockwise\n");
		exit(1);
	}
	current_block->model = (current_block->model + Block_model_count - 1) % Block_model_count;
	if (block_place() == false) {	// block_rotate_counterclockwise failed
		current_block->model = (current_block->model + 1) % Block_model_count; 	// recover
		if (block_place() == false) {
			fprintf(stderr, "block_place error in block_rotate_counterclockwise\n");
			exit(1);
		}
		return false;
	}
	return true;
}
