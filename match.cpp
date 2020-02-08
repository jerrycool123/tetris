#include "match.h"

char Match::tetris_3s[16] = "███";
char Match::empty_3s[16] = "   ";	// 3 space
char Match::empty_10s[16] = "          ";	// 10 spce
char Match::player_header[16] = "Player ";
int Match::bottom_coordinate[2] = {44, 1};
int Match::board_coordinate[2][2] = {{1, 4}, {1, 58}};
int Match::next_block_coordinate[2][2] = {{19, 41}, {19, 95}};
int Match::next_block_position_coordinate[8][2][2] = {{{9, 40}, {9, 94}}, {{5, 40}, {5, 94}}, {{7, 41}, {7, 95}}, {{7, 41}, {7, 95}}, {{9, 43}, {9, 97}}, {{7, 41}, {7, 95}}, {{7, 41}, {7, 95}}, {{7, 41}, {7, 95}}};
int Match::result_coordinate[2][2] = {{18, 13}, {18, 67}};

Match::Match(int fd[2], char *_name[2]) {
	std::time_t result = std::time(nullptr);
	std::cerr << "match start at " << std::ctime(&result);

	ended = false;
	int write_ret;
	// Copy some necessary information
	for (int i = 0; i < 2; ++i) {
		spfd[i] = fd[i];
		cat_r[i] = 1;
		fd_isopen[i] = true;
		memset(name[i], 0, sizeof (name));
		strncpy(name[i], _name[i], strlen(_name[i]));
	}
	// Init player_freezed
	for (int i = 0; i < 2; ++i) {
		player_freezed[i] = false;
	}
	// Remind players the game is on
	count_down();
	// Send initial match interface to both clients
	send_init_interface();
	/* 	Force telnet clients into line mode,
			in order to get keyboard informations.	 
		Besides, close clients' input on their screen. */
	for (int i = 0; i < 2; ++i) {	// WARNING: This part should be rewriten into select mode
		if (fd_isopen[i] == true) {
			Screen::set_line_mode(spfd[i], &write_ret);
			if (write_ret < 0) {	// Client is closed
				fd_isopen[i] = false;
			}
		}
		if (fd_isopen[i] == true) {
			Screen::set_echo_mode(spfd[i], &write_ret);
			if (write_ret < 0) {	// Client is closed
				fd_isopen[i] = false;
			}
		}
	}
	// Put the current_block, next_block and score of each player onto the match interface and send to both clients
	buf_clear();
	buf_append_changed_status();
	send_current_interface();
	// Initailize the timer
	gettimeofday(&tetris_tv[0], NULL);
	tetris_tv[1] = tetris_tv[0];	// Now it's fair
}

void Match::count_down() {
	FILE *fp = fopen(count_down_filename, "rb");
	if (fp == NULL) {
		ERR_EXIT("Match::count_down(), fopen:");
	}
	int write_ret;
	for (int i = 0; i < 2; ++i) {	// WARNING: This part should be rewriten into select mode
		if (fd_isopen[i] == true) {
			Screen::clear(spfd[i], &write_ret);
			if (write_ret < 0) {	// Client is closed
				fd_isopen[i] = false;
			}
		}
	}
	buf_clear();
	while (fgets(buf, sizeof (buf), fp) != NULL) {
		send_current_interface();
		buf_clear();
		sleep(1);
	}
	fclose(fp);
	return;	// Under construction
}


void Match::match_close() {
	for (int i = 0; i < 2; ++i) {
		if (fd_isopen[i] == true) {
			buf_clear();
			Screen::append_gotorc(buf, &buflen, bottom_coordinate[0], bottom_coordinate[1]);	
			write(spfd[i], buf, strlen(buf));
			close(spfd[i]);
			fd_isopen[i] = false;
		}
	}
	std::time_t result = std::time(nullptr);
	std::cerr << "match close at " << std::ctime(&result);
	return;
}

void Match::send_init_interface() {	// Customize interface
	int write_ret;
	// Clear client screen
	for (int i = 0; i < 2; ++i) {
		if (fd_isopen[i] == true) {
			Screen::clear(spfd[i], &write_ret);
			if (write_ret < 0) {	// Client is closed
				fd_isopen[i] = false;
			}
		}
	}
	// filename is stored in init_filename macro
	FILE *fp = fopen(init_filename, "rb");
	if (fp == NULL) {
		ERR_EXIT("Match::send_init_interface(), fopen:");
	}
	buf_clear();
	memset(local_buf, 0, sizeof (local_buf));
	while (fread(local_buf, 1, 1024, fp) > 0) {
		buf_append(local_buf);
		memset(local_buf, 0, sizeof (local_buf));
	}
	fclose(fp);
	buf_append_name();
	for (int i = 0; i < 2; ++i) {
		buf_append_next(i);
		buf_append_score(i);
	}
	send_current_interface();
}

void Match::send_current_interface() {
	for (int i = 0; i < 2; ++i) {
		if (fd_isopen[i] == true) {
			fprintf(stderr, "send player %d, %d bytes\n", i, strlen(buf));
			int write_ret;
			write_ret = write(spfd[i], buf, strlen(buf));
			if (write_ret < 0) {
				fd_isopen[i] = false;
			}
		}
	}
	return;
}

void Match::refresh(int player) {
	int write_ret;
	// Clear client screen
	if (fd_isopen[player] == true) {
		Screen::clear(spfd[player], &write_ret);
		if (write_ret < 0) {	// Client is closed
			fd_isopen[player] = false;
		}
	}
	/* 	Force telnet clients into no-echo mode,
			in order to print the initial interface. */
	if (fd_isopen[player] == true) {
		Screen::set_no_echo_mode(spfd[player], &write_ret);
		if (write_ret < 0) {	// Client is closed
			fd_isopen[player] = false;
		}
	}
	// filename is stored in init_filename macro
	buf_clear();
	FILE *fp = fopen(init_filename, "rb");
	if (fp == NULL) {
		ERR_EXIT("Match::send_init_interface(), fopen:");
	}
	memset(local_buf, 0, sizeof (local_buf));
	while (fread(local_buf, 1, 1024, fp) > 0) {
		buf_append(local_buf);
		memset(local_buf, 0, sizeof (local_buf));
	}
	fclose(fp);

	buf_append_name();
	for (int i = 0; i < 2; ++i) {
		buf_append_next(i);
		buf_append_score(i);
	}
	for (int i = 0; i < 2; ++i) {
		for (int r = 0; r < MAXR; ++r) {
			for (int c = 0; c < MAXC; ++c) {
				if (board[i].status[r][c] == 0) {
					Screen::append_gotorc(buf, &buflen, (r * 2) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
					buf_append(empty_3s);
					Screen::append_gotorc(buf, &buflen, (r * 2 + 1) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
					buf_append(empty_3s);
				}
				else {
					Screen::append_gotorc(buf, &buflen, (r * 2) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
					buf_append_color(board[i].status[r][c]);
					buf_append(tetris_3s);
					buf_append_color(0);
					Screen::append_gotorc(buf, &buflen, (r * 2 + 1) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
					buf_append_color(board[i].status[r][c]);
					buf_append(tetris_3s);
					buf_append_color(0);
				}
			}
		}
	}
	if (fd_isopen[player] == true) {
		int write_ret;
		write_ret = write(spfd[player], buf, strlen(buf));
		if (write_ret < 0) {
			fd_isopen[player] = false;
		}
	}
	/* 	Force telnet clients into echo mode,
			in order to close clients' input on their screen. */
	if (fd_isopen[player] == true) {
		Screen::set_echo_mode(spfd[player], &write_ret);
		if (write_ret < 0) {	// Client is closed
			fd_isopen[player] = false;
		}
	}
	cat_r[player] = 1;
	return;	
}

void Match::buf_clear() {
	memset(buf, 0, sizeof (buf));
	buflen = 0;
	return;
}

void Match::buf_append(char *str) {
	strcpy(buf + buflen, str);
	buflen += strlen(str);
	return;
}

void Match::buf_append_name() {
	int temp_len = (MMAXC * 3 / 2) - 3 - strlen(player_header) - strlen(name[0]);
	buf_append(empty_3s);
	buf_append(player_header);
	buf_append(name[0]);
	for (int i = 0; i < temp_len; ++i)
		buf_append((char *)" ");
	buf_append(empty_3s);
	buf_append(player_header);
	buf_append(name[1]);
	return;
}

void Match::buf_append_changed_status() {
	// Each cell is converted into 2x3 blocks to keep the "square-like" property.
	// 0 in status array means the cell is empty.
	// Define tetris character '█h' (which is not a real "character", it is a "string" instead).
	// The tetris character have different colors, predefined in block.h (Block::ch_color). In status arry, it is represented by integer 1~7, which means 7 different tetrominoes & colors.
	// Check the difference between each board's status & old_status.
	for (int r = 0; r < MAXR; ++r) {
		for (int c = 0; c < MAXC; ++c) {
			fprintf(stderr, "%d", board[0].status[r][c]);
		}
		fprintf(stderr, "  ");
		for (int c = 0; c < MAXC; ++c) {
			fprintf(stderr, "%d", board[0].old_status[r][c]);
		}
		fprintf(stderr, "\n");
	}

	for (int i = 0; i < 2; ++i) {
		for (int r = 0; r < MAXR; ++r) {
			for (int c = 0; c < MAXC; ++c) {
//				fprintf(stderr, "i = %d, (%d, %d) \n", i, r, c);
				if (board[i].status[r][c] != board[i].old_status[r][c]) {
					if (board[i].status[r][c] == 0) {
						Screen::append_gotorc(buf, &buflen, (r * 2) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
						buf_append(empty_3s);
						Screen::append_gotorc(buf, &buflen, (r * 2 + 1) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
						buf_append(empty_3s);
					}
					else {
						Screen::append_gotorc(buf, &buflen, (r * 2) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
						buf_append_color(board[i].status[r][c]);
						buf_append(tetris_3s);
						buf_append_color(0);
						Screen::append_gotorc(buf, &buflen, (r * 2 + 1) + board_coordinate[i][0], (c * 3) + board_coordinate[i][1]);
						buf_append_color(board[i].status[r][c]);
						buf_append(tetris_3s);
						buf_append_color(0);
					}
				}
			}
		}
		board[i].store_status();
	}
	return;
}

void Match::buf_append_next(int player) {
	// Under construction
	int type = board[player].next_block->type;
	int r, c;
	for (int i = 0; i < 4; ++i) {
		r = next_block_position_coordinate[0][player][0] + i;
		c = next_block_position_coordinate[0][player][1];
		Screen::append_gotorc(buf, &buflen, r, c);
		for (int j = 0; j < 4; ++j)
			buf_append(empty_3s);
	}
	for (int i = 0; i < Block_cell_count; ++i) {
		r = next_block_position_coordinate[type][player][0] + Block::Tetrominoes[type][0][i][0] * 2;
		c = next_block_position_coordinate[type][player][1] + Block::Tetrominoes[type][0][i][1] * 3;
		Screen::append_gotorc(buf, &buflen, r, c);
		buf_append_color(type);
		buf_append(tetris_3s);
		buf_append_color(0);
		++r;	// 2 x 3
		Screen::append_gotorc(buf, &buflen, r, c);
		buf_append_color(type);
		buf_append(tetris_3s);
		buf_append_color(0);
	}
	return;	
}

void Match::buf_append_color(int color) {
	// Note: Color 0 is reset
	switch (color) {
		case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
			buf_append(Block::ch_color[color]);
			break;
		default:
			fprintf(stderr, "switch error in Match::buf_append_color(), color = %d\n", color);
			exit(1);
	}
	return;
}

void Match::buf_append_score(int player) {
	// Clear score space
	Screen::append_gotorc(buf, &buflen, next_block_coordinate[player][0], next_block_coordinate[player][1]);
	buf_append(empty_10s);
	// Print new score
	Screen::append_gotorc(buf, &buflen, next_block_coordinate[player][0], next_block_coordinate[player][1]);
	sprintf(local_buf, "%d", board[player].score);
	buf_append(local_buf);
	return;
}

void Match::trans_instruction(int player, char *_buf) {
	fprintf(stderr, "server received from player %d:\n", player);
	if (player_freezed[player] == false) {
		if (strcmp(KEY_DOWN, _buf) == 0) {
			fprintf(stderr, "%d: DOWN\n", player);
			if (board[player].block_drop() == true) {
				buf_clear();
				buf_append_changed_status();
				send_current_interface();
				gettimeofday(&tetris_tv[player], NULL);
			}
		}
		else if (strcmp(KEY_LEFT, _buf) == 0) {
			fprintf(stderr, "%d: LEFT\n", player);
			if (board[player].block_left() == true) {
				buf_clear();
				buf_append_changed_status();
				send_current_interface();
			}
		}
		else if (strcmp(KEY_RIGHT, _buf) == 0) {
			fprintf(stderr, "%d: RIGHT\n", player);
			if (board[player].block_right() == true) {
				buf_clear();
				buf_append_changed_status();
				send_current_interface();
			}
		}
		else if (strcmp(KEY_Z, _buf) == 0) {
			fprintf(stderr, "%d: z\n", player);
			if (board[player].block_rotate_counterclockwise() == true) {
				buf_clear();
				buf_append_changed_status();
				send_current_interface();
			}
		}
		else if (strcmp(KEY_X, _buf) == 0) {
			fprintf(stderr, "%d: x\n", player);
			if (board[player].block_rotate_clockwise() == true) {
				buf_clear();
				buf_append_changed_status();
				send_current_interface();
			}
		}
		else if (strcmp(KEY_R, _buf) == 0) {
			fprintf(stderr, "%d: r\n", player);
			refresh(player);
		}
	}
	if (strcmp(KEY_CTRL_C, _buf) == 0
			|| strcmp(KEY_CTRL_Z, _buf) == 0
			|| strcmp(KEY_CTRL_backslash, _buf) == 0
			|| strlen(_buf) == 0) {	// Player left unexpectedly
		fprintf(stderr, "close fd = %d\n", spfd[player]);
		buf_clear();
		Screen::append_gotorc(buf, &buflen, bottom_coordinate[0], bottom_coordinate[1]);	
		fprintf(stderr, "send player %d, %d bytes\n", player, strlen(buf));
		write(spfd[player], buf, strlen(buf));
		close(spfd[player]);
		fd_isopen[player] = false;
	}
	else if (strcmp(KEY_CAT, _buf) == 0) {
		fprintf(stderr, "player %s says MEOW MEOW MEOW\n", name[player]);
		meow(player);
	}
	return;
}

long long Match::utime_dif(int player, struct timeval tv) {
	return (1000000 * (tv.tv_sec - tetris_tv[player].tv_sec) + (tv.tv_usec - tetris_tv[player].tv_usec));
}

void Match::check_status(int player) {
	/* check_status():
	   // In Board::check_status
	   1. Clear full lines of board[player].status
	   2. Add board[player].score
	   // End of Board::check_status
	   3. If scored, buf_append_score() to buf
	   4. next_block >> current_block
	   5. next_block << Block::generate()
	   6. buf_append_next()
	   7. Place current block; If failed, declare player freeze
	   8. Send match interface to both clients
	   9. Reset tetris_tv[player]
	*/
	bool is_line_cleared;
	is_line_cleared = board[player].check_status();
	fprintf(stderr, "player %d is_line_cleared == %s\n", player, is_line_cleared == true?"true":"false");
	buf_clear();
	if (is_line_cleared == true) {	// Update player score
		buf_append_score(player);
	}
	fprintf(stderr, "destroy current block\n");
	board[player].destroy_block(board[player].current_block);
	fprintf(stderr, "current << next\n");
	board[player].current_block = board[player].next_block;
	fprintf(stderr, "next << generate\n");
	board[player].next_block = board[player].generate_block();
	buf_append_next(player);
	if (board[player].block_place() == false) {	// Failed to place current block, freeze the player
		fprintf(stderr, "block_place() failed\n");
		freeze(player);
	}
	else {
		fprintf(stderr, "block_place() failed\n");
	}
	fprintf(stderr, "append changed status\n");
	buf_append_changed_status();
	fprintf(stderr, "send\n");
	send_current_interface();
	fprintf(stderr, "reset tetris_tv\n");
	gettimeofday(&tetris_tv[player], NULL);
	return;
}

void Match::check_timeout() {
	struct timeval current_tv;	// Record current time for match timeout detection
	gettimeofday(&current_tv, NULL);

	// Check timeout
	if (player_freezed[0] == false && utime_dif(0, current_tv) > TIMEOUT_U) {	// Player 0 timeout
		fprintf(stderr, "player 0 timeout.\n");
		if (board[0].block_drop() == false) {	// Block landed
			fprintf(stderr, "player 0 check_status.\n");
			check_status(0);
		}
		else {
			buf_clear();
			buf_append_changed_status();
			send_current_interface();
			gettimeofday(&tetris_tv[0], NULL);
		}
	}
	if (player_freezed[1] == false && utime_dif(1, current_tv) > TIMEOUT_U) {	// Player 1 timeout
		fprintf(stderr, "player 1 timeout.\n");
		if (board[1].block_drop() == false) {	// Block landed
			fprintf(stderr, "player 1 check_status.\n");
			check_status(1);
		}
		else {
			buf_append_changed_status();
			send_current_interface();
			gettimeofday(&tetris_tv[1], NULL);
		}
	}
	return;
}

void Match::freeze(int player) {
	if (player_freezed[player] == true) {
		fprintf(stderr, "ERROR: Match::freeze(), player %d has been freezed.\n", player);
		exit(1);
	}
	player_freezed[player] = true;
	return;
}

void Match::win(int player) {
	FILE *fp_win = fopen(win_box_filename, "rb");
	FILE *fp_lose = fopen(lose_box_filename, "rb");
	FILE *fp_draw = fopen(draw_box_filename, "rb");
	int i = 0;
	buf_clear();
	memset(local_buf, 0, sizeof (local_buf));
	switch (player) {
		case 0:
			i = 0;
			while (fgets(local_buf, sizeof (local_buf), fp_win) != NULL) {
				Screen::append_gotorc(buf, &buflen, result_coordinate[0][0] + i, result_coordinate[0][1]);
				buf_append(local_buf);
				memset(local_buf, 0, sizeof (local_buf));
				++i;
			}
			i = 0;
			while (fgets(local_buf, sizeof (local_buf), fp_lose) != NULL) {
				Screen::append_gotorc(buf, &buflen, result_coordinate[1][0] + i, result_coordinate[1][1]);
				buf_append(local_buf);
				memset(local_buf, 0, sizeof (local_buf));
				++i;
			}
			break;
		case 1:
			i = 0;
			while (fgets(local_buf, sizeof (local_buf), fp_lose) != NULL) {
				Screen::append_gotorc(buf, &buflen, result_coordinate[0][0] + i, result_coordinate[0][1]);
				buf_append(local_buf);
				memset(local_buf, 0, sizeof (local_buf));
				++i;
			}
			i = 0;
			while (fgets(local_buf, sizeof (local_buf), fp_win) != NULL) {
				Screen::append_gotorc(buf, &buflen, result_coordinate[1][0] + i, result_coordinate[1][1]);
				buf_append(local_buf);
				memset(local_buf, 0, sizeof (local_buf));
				++i;
			}
			break;
		case 2:
			i = 0;
			while (fgets(local_buf, sizeof (local_buf), fp_draw) != NULL) {
				Screen::append_gotorc(buf, &buflen, result_coordinate[0][0] + i, result_coordinate[0][1]);
				buf_append(local_buf);
				memset(local_buf, 0, sizeof (local_buf));
				++i;
			}
			fseek(fp_draw, 0, SEEK_SET);
			i = 0;
			while (fgets(local_buf, sizeof (local_buf), fp_draw) != NULL) {
				Screen::append_gotorc(buf, &buflen, result_coordinate[1][0] + i, result_coordinate[1][1]);
				buf_append(local_buf);
				memset(local_buf, 0, sizeof (local_buf));
				++i;
			}
			break;
		default:
			fprintf(stderr, "ERROR: Match::win(), player = %d\n", player);
			break;
	}
	send_current_interface();
	fclose(fp_win);
	fclose(fp_lose);
	fclose(fp_draw);
	ended = 1;
	return;
}

void Match::meow(int player) {
	FILE *fp_cat = fopen(meow_filename, "rb");
	buf_clear();
	memset(local_buf, 0, sizeof (local_buf));
	while (fgets(local_buf, sizeof (local_buf), fp_cat) != NULL) {
		Screen::append_gotorc(buf, &buflen, cat_r[player], 109);
		buf_append(local_buf);
		memset(local_buf, 0, sizeof (local_buf));
		++cat_r[player];
	}
	write(spfd[player], buf, strlen(buf));
	return;
}
