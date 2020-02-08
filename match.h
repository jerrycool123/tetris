#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctime>
#include "server.h"
#include "board.h"
#include "block.h"
#include "screen.h"
#include "keyboard.h"
#ifndef _MATCH_H
#define _MATCH_H
#define MMAXBUF 40960	// 40KB
#define MMAXR 21
#define MMAXC 36 
#define TIMEOUT_U 800000	// 0.8s
#define count_down_filename "countdown.txt"
#define init_filename "initial_interface.txt"
#define win_box_filename "win_box.txt"
#define draw_box_filename "draw_box.txt"
#define lose_box_filename "lose_box.txt"
#define meow_filename "cat.txt"

class Match {
	public:
		Match(int fd[2], char *_name[2]);
		void count_down();	// At the begin of the match, count down to remind players
		void match_close();
		void send_init_interface();	// Send initial match interface to both clients
		void send_current_interface();	// Send current match interface to both clients
		void refresh(int player);	// Re-send the match interface to the player (for players who resize the terminal)
		void buf_clear();	// Clear buf and set buflen to 0
		void buf_append(char *str);	// Append str to buf
		void buf_append_name();	// Append player names to buf
		void buf_append_changed_status();	// Convert changed status to printable data, and then store in buf[]
		void buf_append_next(int player);	// Append next block printable information to buf for either player
		void buf_append_color(int color);	// Append color text to buf, defined in class Block
		void buf_append_score(int player);	// Append score to buf
		void trans_instruction(int player, char *buf);	// Translate client data into instructions
		long long utime_dif(int player, struct timeval tv);	// Return time difference between tv and tetris_tv[player], in usec
		void check_status(int player);	
		void check_timeout();	// Check if timeout
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
		void freeze(int player);	// Set player_freezed[player] = true
		void win(int player);	// Determine which player win or draw
		void meow(int player);	// Meow ><
		int cat_r[2];	// Cat row
		bool ended;	// Whether match is ended
		int spfd[2];	// Client file descriptor
		bool fd_isopen[2];	// Whether client fd is open
		bool player_freezed[2];	// If so, there would be no new block and player can't do any block operation
		struct timeval tetris_tv[2];	// Determine auto Board::block_drop()
		char buf[MMAXBUF];	// I/O buffer
		int buflen;			// I/O buffer length
		char local_buf[MMAXBUF];
		char newline_buf[16];	// newline buffer
		Board board[2];		// Tetris boards for each player
		char name[2][16];
		static char tetris_3s[16];
		static char empty_3s[16];
		static char empty_10s[16];
		static char player_header[16];
		static int bottom_coordinate[2];
		static int board_coordinate[2][2];
		static int next_block_coordinate[2][2];
		static int next_block_position_coordinate[8][2][2];
		static int result_coordinate[2][2];
};
#endif
