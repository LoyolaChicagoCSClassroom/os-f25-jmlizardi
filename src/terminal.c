#include "terminal.h"
#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 25
#define DEFAULT_ATTR 0x07 // set text color so we don't use magic numbers

struct video_buf *vram = (struct video_buf *)0xb8000; //starting point of VGA buffer
static int cur_row = 0;
static int cur_col = 0;

static inline int idx(int r, int c) {
    return r * TERMINAL_WIDTH + c;
}

static void scroll_up(void); // prototype, had to state this early. If I move it code has compile issues.

int putc(int data) {
if (data == '\r') {
 // carriage return, resets cursor % returns current offset and (-)'s itself
    cur_col = 0;
    return data;
}

if(data == '\n') {
 // Newline: next row, col 0
    cur_row++;
    cur_col = 0;
} else {
    // Write at current cursor
    int pos = idx(cur_row, cur_col);
    vram[pos].ascii = (char)data;
    vram[pos].color = DEFAULT_ATTR;

    // Advance cursor
    if (++cur_col >= TERMINAL_WIDTH){
	cur_col = 0;
	cur_row++;
    }
}

// Scroll if at the bottom of the screen
    if (cur_row >= TERMINAL_HEIGHT) {
	scroll_up();
	cur_row = TERMINAL_HEIGHT - 1;
    }
   return data;
}

// Scroll screen method
static void scroll_up(void) {
    // Move rows 1 -> TERMINAL_HEIGHT - 1 up by one row
    for (int r = 1; r < TERMINAL_HEIGHT; r++){
	int from = idx(r, 0);
	int to = idx(r - 1, 0);
	for (int c = 0; c < TERMINAL_WIDTH; c++){
	    vram[to + c] = vram[from + c];
    }
}

    // Clear the last row
    int base = idx(TERMINAL_HEIGHT - 1, 0);
    for (int c = 0; c < TERMINAL_WIDTH; c++){
	vram[base + c].ascii = ' ';
	vram[base + c].color = DEFAULT_ATTR;
    }
}
