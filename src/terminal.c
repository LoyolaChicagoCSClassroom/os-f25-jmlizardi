#include "terminal.h"
#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 25
#define DEFAULT_ATTR 0x07 // set text color so we don't use magic numbers

struct video_buf *vram = (struct video_buf *)0xb8000; //starting point of VGA buffer
static int cur_row = 0;
static int cur_col = 0;

static inline int idx(int r, int c) {
    return r * TERMINAL_WIDTH + c; // Calculation for current index row * T_W + col = current index
}

static void scroll_up(void); // prototype, had to state this early. If I move it code has compile issues.

int putc(int data) { // puts a character at current index
if (data == '\r') {
 // carriage return, resets cursor back to pos 0
    cur_col = 0;
    return data;
}

if(data == '\n') {
 // Newline: next row, col 0
    cur_row++;
    cur_col = 0;
} else {
    // Write at current cursor
    int pos = idx(cur_row, cur_col); // grabs current index, and prints to position in array
    vram[pos].ascii = (char)data; // copying the value to the index
    vram[pos].color = DEFAULT_ATTR; // setting the attr to the new value in index

    // Advance cursor
    if (++cur_col >= TERMINAL_WIDTH){ // checks if at the end of the line to move cursor back to start and down a row 
	cur_col = 0;
	cur_row++;
    }
}

// Scroll if at the bottom of the screen
    if (cur_row >= TERMINAL_HEIGHT) { // compares if cur_row is at screen limit
	scroll_up();
	cur_row = TERMINAL_HEIGHT - 1; // puts current position back to 24, since the lines moved up
    }
   return data;
}

// Scroll screen method
static void scroll_up(void) {
    // Move rows 1 -> TERMINAL_HEIGHT - 1 up by one row
    for (int r = 1; r < TERMINAL_HEIGHT; r++){ // for loop to load each line from row 1-24 up one row. Second loop goes through each char to move it in array
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
// NOTE, I used MAKE RUN to run the program... ./launch_qema.sh was faulty for me and so in my make file you can see I made a phony make run falling back on x86
