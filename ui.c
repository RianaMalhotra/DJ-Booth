#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define FB_ADDR 0xFF203020 // front buffer address
#define PS2_BASE 0xFF200100 // ps2 data register address
#define PS2_SPEED 30
#define SW_BASE 0xFF200040 // switch data register address
#define KEY_BASE 0xFF200050 // push button data register address
#define LED_BASE 0xFF200000 // led data register address

//#define M_PI 3.14159265
#define DEG2RAD(deg) ((deg) * M_PI / 180.0)
#define BG_COLOUR_S 0xFC0E
#define BG_COLOUR_M 0xECBB
    
// FOR SWITCHES
#define P_PURPLE 0x8C9F
#define P_BLUE 0x7D9F
#define B_BLUE 0x0E5F
#define ORANGE 0xFBE0
#define TEAL 0x1CF5
#define B_TEAL 0x4ED5
short int COLOURS[3] = {B_BLUE, ORANGE, B_TEAL};
// ---------------
    
#define GREEN 0xA6D4
#define RED 0xE2C8
//#define GREEN 0x7E60
#define L_GREY 0xAD55
#define YELLOW 0xFEE0
#define GREY 0x8410
#define BLACK 0x0000
#define WHITE 0xFFFF
#define L_BLACK 0x528A
#define BLUE 0x6F7F

#define FONT_W 5
#define FONT_H 7
   
// FONT TABLE (5X7) --> bitmap created by ChatGPT
/* 128 (ASCII characters), 7 (height of letters)
1 means draw pixel */
const unsigned char FONT[128][7] = {
[' '] = {0,0,0,0,0,0,0},

['W'] = {
    0b10001,
    0b10001,
    0b10001,
    0b10101,
    0b10101,
    0b11011,
    0b10001
},

['e'] = {
    0b00000,
    0b00000,
    0b01110,
    0b10001,
    0b11111,
    0b10000,
    0b01111
},

['l'] = {
    0b00110,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
},

['c'] = {
    0b00000,
    0b00000,
    0b01110,
    0b10000,
    0b10000,
    0b10000,
    0b01111
},

['o'] = {
    0b00000,
    0b00000,
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b01110
},

['m'] = {
    0b00000,
    0b00000,
    0b11010,
    0b10101,
    0b10101,
    0b10101,
    0b10101
},

['t'] = {
    0b00100,
    0b00100,
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00011
},

['C'] = {
    0b01110,
    0b10001,
    0b10000,
    0b10000,
    0b10000,
    0b10001,
    0b01110
},

['h'] = {
    0b10000,
    0b10000,
    0b10110,
    0b11001,
    0b10001,
    0b10001,
    0b10001
},

['&'] = {
    0b01100,
    0b10010,
    0b10100,
    0b01000,
    0b10101,
    0b10010,
    0b01101
},

['R'] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10100,
    0b10010,
    0b10001
},

['i'] = {
    0b00100,
    0b00000,
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
},

['a'] = {
    0b00000,
    0b00000,
    0b01110,
    0b00001,
    0b01111,
    0b10001,
    0b01111
},

['n'] = {
    0b00000,
    0b00000,
    0b10110,
    0b11001,
    0b10001,
    0b10001,
    0b10001
},

['\''] = {
    0b00100,
    0b00100,
    0b01000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
},

['s'] = {
    0b00000,
    0b00000,
    0b01111,
    0b10000,
    0b01110,
    0b00001,
    0b11110
},

['D'] = {
    0b11110,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11110
},

['J'] = {
    0b00001,
    0b00001,
    0b00001,
    0b00001,
    0b10001,
    0b10001,
    0b01110
},

['B'] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10001,
    0b10001,
    0b11110
},
   
['E'] = {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b10000,
    0b10000,
    0b11111
},

['N'] = {
    0b10001,
    0b11001,
    0b10101,
    0b10101,
    0b10011,
    0b10001,
    0b10001
},

['T'] = {
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100
},

['R'] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10100,
    0b10010,
    0b10001
},
    
['P'] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10000,
    0b10000,
    0b10000
},
    
['F'] = {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b10000,
    0b10000,
    0b10000
},

['V'] = {
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01010,
    0b00100
},
    
['S'] = {
    0b01111,
    0b10000,
    0b10000,
    0b01110,
    0b00001,
    0b00001,
    0b11110
},
    
['H'] = {
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b10001
},
    
['H'] = {
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b10001
},
    
['O'] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01110
},
    
['A'] = {
    0b01110,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b10001
},
    
['L'] = {
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b11111
},
    
['T'] = {
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100
},
    
['r'] = {
    0b00000,
    0b00000,
    0b10110,
    0b11001,
    0b10000,
    0b10000,
    0b10000
},
    
['r'] = {
    0b00000,
    0b00000,
    0b10110,
    0b11001,
    0b10000,
    0b10000,
    0b10000
},
    
['k'] = {
    0b10000,
    0b10000,
    0b10010,
    0b10100,
    0b11000,
    0b10100,
    0b10010
},
    
['1'] = {
    0b00100,
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
},
    
['2'] = {
    0b01110,
    0b10001,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b11111
},
    
['3'] = {
    0b11110,
    0b00001,
    0b00001,
    0b01110,
    0b00001,
    0b00001,
    0b11110
},
    
['4'] = {
    0b00010,
    0b00110,
    0b01010,
    0b10010,
    0b11111,
    0b00010,
    0b00010
},
    
['5'] = {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b00001,
    0b00001,
    0b11110
}

};
   
// FUNCTION DECLARATIONS
void plot_pixel(int x, int y, short int colour);
void plot_scaled_pixel(int x, int y, short int colour, float scale);
void wait_for_vsync();
void fill_background(short int colour);

void draw_char(int x, int y, char c, short int fill_colour, short int outline_colour, float scale);
void draw_letters(int x, int y, const char *letters, short int fill_colour, short int outline_colour, float scale);
void draw_letters_centered(int y, const char *letters, short int fill_colour, short int outline_colour, float scale);
void draw_disc(int x, int y, int r, bool show_button, float theta, volatile int* mouse_ptr, bool show_echo);
void draw_circle(int x, int y, int r, short int colour);
void draw_line (int x0, int y0, int x1, int y1, short int line_colour);
void swap(int *a, int *b);
void draw_rotating_line(int x, int y, int r1, int r2, float theta, short int colour);
   
void draw_enter_button (int x, int y, int w, int h);
void draw_rectangle(int x, int y, int w, int h, short int colour);
void draw_letters_box(int x, int y, int w, int h, const char* letters, short int fill_colour, short int outline_colour, float scale);
void draw_exit_button(int x0, int y0, int w, int h, short int line_colour, short int outer_colour, short int inner_colour, int scale);
void draw_push_button(int x, int y, int w, int h, int r, short int outer_colour, short int inner_colour, int scale, int mode, short int on);
void draw_switch(int x, int y, int w, int h, short int base, short int off, short int on, int scalex, int scaley, int mode);
void draw_play_button(int x, int y_ctr, int h, int w, short int colour);
void draw_led(int x, int y, int w, int h, short int outer, short int inner, int scale, int mode, short int colour);
void draw_control_bar(int x, int y, int w, int h, int scale, short int outer, short int inner, short int control);

unsigned char read_ps2(volatile int* mouse_ptr);
void config_ps2(volatile int* mouse_ptr);
void process_ps2(volatile int* mouse_ptr);
int ps2_avail(volatile int* mouse_ptr);

void draw_arc_highlight(int x, int y, int r, float c_deg, float s_deg, int m_thick, short int colour);

// MOUSE STRUCT
typedef struct {
    // x, y coordinates
    volatile int x, y;
    volatile bool left, right; // left, right buttons
    volatile bool prev_left, prev_right; // previous click
} mouse;

// ENTER BUTTON STRUCT
typedef struct {
    int x_min, x_max;
    int y_min, y_max;
} button;

// GLOBAL VARIABLES
volatile short int* pixel_buffer_start;
short int Buffer1[240][512];
short int Buffer2[240][512];

mouse MOUSE;
button ENTER_B;
button EXIT_B;

bool LOADING = 0;
bool MAIN = 1;
int IND = 0;
int NUM_COL = 3;
int TRACK_NUM = 1;
int TRACK_TOTAL = 5;
int PAUSE = false;

int main(void) {
    volatile int* pixel_ctrl_ptr = (int *)FB_ADDR; // location of pixel buffer
    volatile int* PS2_ptr = (volatile int *)PS2_BASE;
    PS2_ptr = (volatile int *)PS2_BASE;
    volatile int* SW_ptr = (volatile int *)SW_BASE;
    volatile int* KEY_ptr = (volatile int *)KEY_BASE;
    volatile int* LED_ptr = (volatile int *)LED_BASE;
    /* set front pixel buffer to Buffer 1*/
    *(pixel_ctrl_ptr + 1) = (int) &Buffer1; // first store address of Buffer1 in back buffer
    wait_for_vsync(); // swap front and back buffers
    pixel_buffer_start = (volatile short int*) *pixel_ctrl_ptr; // initialize pointer to front buffer
    fill_background(BLACK);
   
    /* set back pixel buffer to Buffer 2*/
    *(pixel_ctrl_ptr + 1) = (int) &Buffer2; // store address of Buffer2 in back buffer
    pixel_buffer_start = (volatile short int*) *(pixel_ctrl_ptr + 1); // initialize pointer to back buffer for rendering later
    fill_background(BLACK);
   
    /* PS/2 MOUSE setup */
    config_ps2(PS2_ptr);
   
    MOUSE.x = SCREEN_W / 2;
    MOUSE.y = SCREEN_H / 2;
   
    int sw_data, key_data, led_data;
    int sw_sum = 0, key_sum = 0, led_sum = 0;
    
    float theta = 0;
   
    while (1) {
        if (LOADING) {
            fill_background(BG_COLOUR_S);
            theta -= 15;
            if (theta >= 360) {
                theta = -360;
            }
            draw_letters_centered(25, "Welcome to", YELLOW, BLACK, 2);
            draw_letters_centered(45, "Chloe & Riana's", YELLOW, BLACK, 2);
            draw_letters_centered(65, "DJ Booth", YELLOW, BLACK, 2);
            draw_disc(160, 155, 65, 1, theta, PS2_ptr, 0); // 1 means true for boolean
            
            process_ps2(PS2_ptr);

            // drawing NEW + cursor
            draw_line (MOUSE.x - 3, MOUSE.y, MOUSE.x + 3, MOUSE.y, WHITE);
            draw_line(MOUSE.x, MOUSE.y - 3, MOUSE.x, MOUSE.y + 3, WHITE);
            printf("xcoord : %d, ycoord: %d\n", MOUSE.x, MOUSE.y);

            // check bounds on enter button
            if (((MOUSE.left && !MOUSE.prev_left) || (MOUSE.right && !MOUSE.prev_right)) &&
                (MOUSE.x >= ENTER_B.x_min && MOUSE.x <= ENTER_B.x_max) &&
                (MOUSE.y >= ENTER_B.y_min && MOUSE.y <= ENTER_B.y_max)) {
                LOADING = 0;
                MAIN = 1;

            }

        }
           
        else if (MAIN) {
            // REVERSE
            if ((*SW_ptr & 0x3FF) == 0b1) {
				if (*(KEY_ptr+3) & 0x2) {
					*(KEY_ptr+3) = 0b10; // write 1 back into it
					PAUSE = true;
				}
				else if (PAUSE) {
					PAUSE = true;
				}
				else {
					theta += 15;
					if (theta >= 360) {
						theta = -360;
					}
				}
                fill_background(B_BLUE);
                draw_letters_centered(12, "REVERSE", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            // ECHO
            else if ( (*SW_ptr & 0x3FF) == 0b10) {
                if (*(KEY_ptr+3) & 0x2) {
					*(KEY_ptr+3) = 0b10; // write 1 back into it
					PAUSE = true;
				}
				else if (PAUSE) {
					PAUSE = true;
				}
				else {
					theta -= 8;
					if (theta >= 360) {
						theta = -360;
					}
				}
                fill_background(TEAL);
                draw_letters_centered(12, "ECHO", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 1);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 1);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            // CROSSFADE
            /*changeeeeeeeeeee********************K*/
            else if ( (*SW_ptr & 0x3FF) == 0b100) {
                theta += 15;
                if (theta >= 360) {
                    theta = -360;
                }
                fill_background(ORANGE);
                draw_letters_centered(12, "CROSSFADE", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            // REVERSE + ECHO
            else if ((*SW_ptr & 0x3FF) == 0b11) {
                /*******CHANGEEEEEEEEE ++++++++THETA
                one disc keep reverse
                other disc apply echo*/
                theta -= 15;
                if (theta >= 360) {
                    theta = -360;
                }
                fill_background(P_PURPLE);
                draw_letters_centered(12, "REVERSE & ECHO", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            // REVERSE + CROSSFADE
            else if ((*SW_ptr & 0x3FF) == 0b101) {
                /*******CHANGEEEEEEEEE +++++++++++=THETA
                one disc apply reverse
                other disc apply crossfade*/
                theta -= 15;
                if (theta >= 360) {
                    theta = -360;
                }
                fill_background(B_TEAL);
                draw_letters_centered(12, "REVERSE & CROSSFADE", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            // ECHO + CROSSFADE
            else if ((*SW_ptr & 0x3FF) == 0b110) {
                /*******CHANGEEEEEEEEE ++++++++ THETA
                one disc apply echo
                other disc apply crossfade*/
                theta -= 15;
                if (theta >= 360) {
                    theta = -360;
                }
                fill_background(P_BLUE);
                draw_letters_centered(12, "ECHO & CROSSFADE", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            // ALL EFFECTS
            else if ((*SW_ptr & 0x3FF) == 0b111) {
                /*******CHANGEEEEEEEEE +++++++++++THETA */
                theta += 7;
                if (theta >= 360) {
                    theta = -360;
                }
                fill_background(COLOURS[IND++ % NUM_COL]);
                draw_letters_centered(12, "ALL EFFECTS", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
            
            else {
                theta -= 15;
                if (theta >= 360) {
                    theta = -360;
                }
                fill_background(BG_COLOUR_M);
                draw_letters_centered(12, "CRB", YELLOW, BLACK, 2);
                draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0);
                draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0);
                draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
            }
           
            key_data = *KEY_ptr;
            key_sum = 0;
            // draw 4 pushbuttons (black base, grey rect, black circle, four border circle)
            for (int i = 285; i >= 186; i -= 33) {
                draw_push_button(i, 198, 30, 30, 9, BLACK, L_GREY, 2, (key_data >> key_sum) & 1, GREEN);
                key_sum++;
            }
            process_ps2(PS2_ptr);
            draw_char(297, 208, 'R', WHITE, BLACK, 1.5);
            draw_char(265, 209, 'P', WHITE, BLACK, 1.5);
			
            draw_char(231, 208, 'B', WHITE, BLACK, 1.5);

			// EDGE CAPTURE REGISTERS
			if (*(KEY_ptr+3) & 0x8) {
				*(KEY_ptr+3) = 0b1000; // write 1 back into it
				PAUSE = false;
				if (TRACK_NUM == 5) {
					TRACK_NUM = 1;
				}
				else {
					TRACK_NUM++;
				}
			}
			
			if (*(KEY_ptr+3) & 0x4) {
				*(KEY_ptr+3) = 0x4;
				PAUSE = false;
				if (TRACK_NUM == 1) {
					TRACK_NUM = 5;
				}
				else {
					TRACK_NUM--;
				}
			}
			
			char track[20];
			sprintf(track, "Track %d", TRACK_NUM);
			draw_letters(222, 183, track, WHITE, BLACK, 1.5);
			
            draw_char(199, 209, 'F', WHITE, BLACK, 1.5);
           
			
            sw_data = *SW_ptr;
            sw_sum = 0;
            // draw switches
            for (int i = 167; i >= 5; i -= 18) {
                draw_switch(i, 195, 13, 35, L_GREY, GREY, BLACK, 2, 3, (sw_data >> sw_sum) & 1);
                sw_sum++;
            }
           
            // draw leds
            led_data = *LED_ptr;
            led_sum = 0;
            for (int i = 171; i >= 9; i -= 18) {
                *LED_ptr = 7;
                draw_led(i, 180, 6, 12, L_GREY, WHITE, 1, (led_data >> led_sum) & 1, RED);
                led_sum++;
            }
           
            // draw pause, play, increase decrease
            // TURN PAUSE, PLUS, MINUS INTO FUNCTION
            // PAUSE
            draw_rectangle(146 - 3, 42 - 3, 26 + (2*3), 26 + (2*3), WHITE);
            draw_rectangle(146, 42, 26, 26, GREY);
            draw_line(153, 47, 153, 63, YELLOW);
            draw_line(154, 47, 154, 63, YELLOW);
            draw_line(155, 47, 155, 63, YELLOW);
            draw_line(163, 47, 163, 63, YELLOW);
            draw_line(164, 47, 164, 63, YELLOW);
            draw_line(165, 47, 165, 63, YELLOW);
           
            //PLAY
            draw_rectangle(146 - 3, 75 - 3, 26 + (2*3), 26 + (2*3), WHITE);
            draw_rectangle(146, 75, 26, 26, GREY);
            draw_play_button(151, 89, 16, 18, YELLOW);
           
            // INCREASE
            draw_rectangle(146 - 3, 108 - 3, 26 + (2*3), 26 + (2*3), WHITE);
            draw_rectangle(146, 108, 26, 26, GREY);
            // vertical line
            draw_line(158, 113, 158, 129, YELLOW);
            draw_line(159, 113, 159, 129, YELLOW);
            draw_line(160, 113, 160, 129, YELLOW);
            // horizontal
            draw_line(151, 120, 167, 120, YELLOW);
            draw_line(151, 121, 167, 121, YELLOW);
            draw_line(151, 122, 167, 122, YELLOW);
           
            // DECREASE
            draw_rectangle(146 - 3, 141 - 3, 26 + (2*3), 26 + (2*3), WHITE);
            draw_rectangle(146, 141, 26, 26, GREY);
            draw_line(151, 153, 167, 153, YELLOW);
            draw_line(151, 154, 167, 154, YELLOW);
            draw_line(151, 155, 167, 155, YELLOW);
            
            process_ps2(PS2_ptr);
            
            draw_line (MOUSE.x - 3, MOUSE.y, MOUSE.x + 3, MOUSE.y, BLUE);
            draw_line(MOUSE.x, MOUSE.y - 3, MOUSE.x, MOUSE.y + 3, BLUE);
            //printf("xcoord : %d, ycoord: %d\n", MOUSE.x, MOUSE.y);
            
            if (((MOUSE.left && !MOUSE.prev_left) || (MOUSE.right && !MOUSE.prev_right)) &&
                (MOUSE.x >= EXIT_B.x_min && MOUSE.x <= EXIT_B.x_max) &&
                (MOUSE.y >= EXIT_B.y_min && MOUSE.y <= EXIT_B.y_max)) {
                LOADING = 1;
                MAIN = 0;

            }
        }
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = (volatile short int*) *(pixel_ctrl_ptr + 1); // gets current address of new back buffer for rendering
    }
}

void plot_pixel(int x, int y, short int colour) {
    if (y < 0 || y >= SCREEN_H || x < 0 || x >= SCREEN_W) {
        return;
    }
   
    volatile short int *one_pixel_address;
    one_pixel_address = pixel_buffer_start + (y << 9) + (x);
    *one_pixel_address = colour;
}


void fill_background(short int colour) {
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            plot_pixel(x, y, colour);
        }
    }
}

void plot_scaled_pixel(int x, int y, short int colour, float scale) {
    for (int dy = 0; dy < scale; dy++) {
        for (int dx = 0; dx < scale; dx++) {
            plot_pixel(x + dx, y + dy, colour);
        }
    }
}

void draw_char(int x, int y, char c, short int fill_colour, short int outline_colour, float scale) {
    // draw the black outline of the letter
    for (int row = 0; row < FONT_H; row++) {
        unsigned char bits = FONT[(unsigned char)c][row];
        // draw border of letters
        for (int col = 0; col < FONT_W; col++) {
            // check if the bit is a one by anding and bit shifting left
            if (bits & (1 << (FONT_W - col - 1))) {
                // (px, py) represent pixel
                int px = x + (col * scale);
                int py = y + (row * scale);
               
                plot_scaled_pixel(px - scale, py, outline_colour, scale); // left border
                plot_scaled_pixel(px + scale, py, outline_colour, scale); // right border
                plot_scaled_pixel(px, py + scale, outline_colour, scale); // bottom border
                plot_scaled_pixel(px, py - scale, outline_colour, scale); // top border

                plot_scaled_pixel(px - scale, py - scale, outline_colour, scale); // top left border
                plot_scaled_pixel(px + scale, py - scale, outline_colour, scale); // top right border
                plot_scaled_pixel(px + scale, py + scale, outline_colour, scale); // bottom right border
                plot_scaled_pixel(px - scale, py + scale, outline_colour, scale); // bottom left border
            }
        }
    }
   
    // draw the filled in colour for the letter
    for (int row = 0; row < FONT_H; row++) {
        unsigned char bits = FONT[(unsigned char)c][row];
       
        for (int col = 0; col < FONT_W; col++) {
            if (bits & (1 << (FONT_W - col - 1))) {
                int px = x + (col * scale);
                int py = y + (row * scale);
                plot_scaled_pixel(px, py, fill_colour, scale);
                plot_scaled_pixel(px - 1, py, fill_colour, scale);
                plot_scaled_pixel(px, py - 1, fill_colour, scale);
            }
        }
    }
}

void draw_letters(int x, int y, const char *letters, short int fill_colour, short int outline_colour, float scale) {
    while (*letters != '\0') { // don't want to hit the terminating character
        draw_char(x, y, *letters, fill_colour, outline_colour, scale);
        x += scale * (FONT_W + 1) ; // 1 PIXEL GAP
        letters++; // increment pointer to move 1 character to the right of the string
    }
}

void draw_letters_centered(int y, const char *letters, short int fill_colour, short int outline_colour, float scale) {
    if (letters != NULL && scale > 0) {
        int text_width = (int)strlen(letters) * scale * (FONT_W + 1); // add 1 to create space between letters
        int x = (SCREEN_W - text_width) / 2;
        draw_letters(x, y, letters, fill_colour, outline_colour, scale);
    }
}

void draw_circle(int x, int y, int r, short int colour) {
    // draw a filled in circle
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            // equation of circle centered at (0,0)
            if (dx*dx + dy*dy <= r*r) {
                plot_pixel(x + dx, y + dy, colour);
            }
        }
    }
}

void draw_ring(int x, int y, int r_max, int r_min, short int colour) {
    for (int dy = -r_max; dy <= r_max; dy++) {
        for (int dx = -r_max; dx <= r_max; dx++) {
            // compute radius square
            int r_sq = dx*dx + dy*dy;
           
            // draw pixel within minimum and maximum radius
            if (r_sq >=r_min*r_min && r_sq <= r_max*r_max) {
                plot_pixel(x + dx, y + dy, colour);
            }
        }
    }
}


void draw_disc(int x, int y, int r, bool show_button, float theta, volatile int* mouse_ptr, bool echo) {
    
    draw_circle(x, y, 65, 0x0000);
    draw_circle(x, y, 60, 0x2104);
    draw_circle(x, y, 52, 0x39E7);
    draw_circle(x, y, 44, 0x4208);
    draw_circle(x, y, 36, 0x2104);
    
    process_ps2(mouse_ptr);
   
    //draw_circle(x, y, r/3, GREY); // grey disk in center
    
    // HIGHLIGHTS
    // DEFINE COLOURS ABOVE
    if (show_button) {
        draw_arc_highlight(x, y, r-4, theta, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-10, theta, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-16, theta, 36, 3, 0x4208);

        draw_arc_highlight(x, y, r-4, theta + 120, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-10, theta + 120, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-16, theta + 120, 36, 3, 0x4208);

        draw_arc_highlight(x, y, r-4, theta + 240, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-10, theta + 240, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-16, theta + 240, 36, 3, 0x4208);
    }
	else if (echo) {
		draw_arc_highlight(x, y, r-1, theta, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7, theta, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, theta, 36, 3, 0x4208);
		//--
		draw_arc_highlight(x, y, r-1, theta + 120, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7, theta + 120, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, theta + 120, 36, 3, 0x4208);
		// --
		draw_arc_highlight(x, y, r-1, theta + 240, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7, theta + 240, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, theta + 240, 36, 3, 0x4208);
		
		
		// ECHO 1
        draw_arc_highlight(x, y, r-6,  theta-30,   32, 4, 0xAD55);
		draw_arc_highlight(x, y, r-14,  theta-30,   26, 3, 0x6B4D);
		draw_arc_highlight(x, y, r-22, theta-30,   20, 2, 0x3186);
		//--
		draw_arc_highlight(x, y, r-6,  theta-30 + 120,   32, 4, 0xAD55);
		draw_arc_highlight(x, y, r-14,  theta-30 + 120,   26, 3, 0x6B4D);
		draw_arc_highlight(x, y, r-22, theta-30 + 120,   20, 2, 0x3186);
		// --
		draw_arc_highlight(x, y, r-6,  theta-30 + 240,   32, 4, 0xAD55);
		draw_arc_highlight(x, y, r-14,  theta-30 + 240,   26, 3, 0x6B4D);
		draw_arc_highlight(x, y, r-22, theta-30 + 240,   20, 2, 0x3186);

		
		// ECHO 2
		draw_arc_highlight(x, y, r-12,  theta-60,  20, 3, 0x8410);
		draw_arc_highlight(x, y, r-20,  theta-60,  16, 2, 0x4208);
		draw_arc_highlight(x, y, r-28, theta-60,  12, 1, 0x2104);
		//--
		draw_arc_highlight(x, y, r-12,  theta-60 + 120,  20, 3, 0x8410);
		draw_arc_highlight(x, y, r-20,  theta-60 + 120,  16, 2, 0x4208);
		draw_arc_highlight(x, y, r-28, theta-60 + 120,  12, 1, 0x2104);
		// --
		draw_arc_highlight(x, y, r-12,  theta-60 + 240,  20, 3, 0x8410);
		draw_arc_highlight(x, y, r-20,  theta-60 + 240,  16, 2, 0x4208);
		draw_arc_highlight(x, y, r-28, theta-60 + 240,  12, 1, 0x2104);

	}
    else {
        draw_arc_highlight(x, y, r-1, theta, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7, theta, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, theta, 36, 3, 0x4208);

        draw_arc_highlight(x, y, r-1, theta + 120, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7, theta + 120, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, theta + 120, 36, 3, 0x4208);

        draw_arc_highlight(x, y, r-1, theta + 240, 48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7, theta + 240, 42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, theta + 240, 36, 3, 0x4208);
    }
    
       process_ps2(mouse_ptr);
    
    if (show_button) {
        draw_enter_button(x, y, r*1.2, r*0.38);
    }
}

void draw_enter_button (int x, int y, int w, int h) {
    int x0 = x - w/2;
    int y0 = y - h/2;
   
    draw_rectangle(x0, y0, w, h, 0x4208); // border of button
    draw_rectangle(x0 + 2, y0 + 2, w - 4, h - 4, WHITE); //white border
    draw_rectangle(x0 + 3, y0 + 3, w - 6, h - 6, GREY); // inside of button
       
    int padding = 3;
    ENTER_B.x_min = x0 - padding;
    ENTER_B.x_max = ENTER_B.x_min + w + padding;
    ENTER_B.y_min = y0 - padding;
    ENTER_B.y_max = ENTER_B.y_min + h + padding;
   
    draw_letters_box(SCREEN_W/2.12, SCREEN_H/2.305, w, h, "ENTER", WHITE, BLACK, 1.5);
}

void draw_rectangle(int x, int y, int w, int h, short int colour) {
    for (int dh = 0; dh <= h; dh++) {
        for (int dw = 0; dw <= w; dw++) {
            plot_pixel(x + dw, y + dh, colour);
        }
    }
}

void draw_letters_box(int x, int y, int w, int h, const char* letters, short int fill_colour, short int outline_colour, float scale) {
    int length = strlen(letters);
    int l_width = ((length - 1) * length * FONT_W); // number of spaces * number of letters
    int l_height = (scale * FONT_H); // need to scale the font height
    int x_ctr = x + (w - l_width) / 2;
    int y_ctr = y + (y - l_height) / 2;
    draw_letters(x_ctr, y_ctr, letters, fill_colour, outline_colour, scale);
}

void draw_line (int x0, int y0, int x1, int y1, short int line_colour) {
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);
    // checking if you need to step horizontally (compute x) or vertically (compute y)
    if (is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
   
    int deltax = x1 - x0; // you know its already positive
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    int y = y0;
    int y_step;
    if (y0 < y1) {
        y_step = 1;
    }
    else {
        y_step = -1;
    }
   
    for (int x = x0; x <= x1; x++) {
        if (is_steep) {
            plot_pixel(y, x, line_colour); // compute y
        }
        else {
            plot_pixel(x, y, line_colour); // compute x
        }
        error = error + deltay; // calcualte error to decide whether or not to increment y
        if (error > 0) {
            y = y + y_step;
            error = error - deltax;
        }
    }
};

void draw_arc_highlight(int x, int y, int r, float c_deg, float s_deg, int m_thick, short int colour) {
    float st = c_deg - (s_deg / 2.0f);
    float end = c_deg + (s_deg / 2.0f);
    
    for (float deg = st; deg <= end; deg += 1.5f) {
        float angle = DEG2RAD(deg);
        float distance = fabs(deg - c_deg) / (s_deg / 2.0f);
        int thick = (int) (m_thick * (1.0f - distance));
        if (thick < 1) {
            thick = 1;
        }
        for (int i = 0; i < thick; i++) {
            int x0 = x + (int)((r + i) * cos(angle));
            int y0 = y - (int)((r + i) * sin(angle));
            plot_pixel(x0, y0, colour);
        }
    }
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
};

unsigned char read_ps2(volatile int* mouse_ptr) {
    // keep popping of head
    int data = *mouse_ptr;
    while (!(data & 0x8000)) {
        data = *mouse_ptr;
    }
    return (unsigned char)(data & 0xFF);
}

int ps2_avail(volatile int* mouse_ptr) {
    return (*mouse_ptr & 0x8000) != 0;
}

void config_ps2(volatile int* mouse_ptr) {
    // reset then check for 0xFA, 0xAA, 0x00
    *mouse_ptr = 0xFF; // reset
    read_ps2(mouse_ptr);
    read_ps2(mouse_ptr);
    read_ps2(mouse_ptr);

    *mouse_ptr = 0xF4; // enable data to be sent
}

void process_ps2(volatile int* mouse_ptr) {
    while (ps2_avail(mouse_ptr)) {
        unsigned char byte1 = read_ps2(mouse_ptr);
        if (!(byte1 & 0x08)) {
            continue;
        }
        if (!ps2_avail(mouse_ptr)) {
            break;
        }
        unsigned char byte2 = read_ps2(mouse_ptr);
        
        if (!ps2_avail(mouse_ptr)) {
            break;
        }
        unsigned char byte3 = read_ps2(mouse_ptr);

        int dx, dy;
        if (byte1 & 0x10) {
            dx = (int)byte2 - 256;
        }
        else {
            dx = (int) byte2;
        }
        if (byte1 & 0x20) {
            dy = (int)byte3 - 256;
        }
        else {
            dy = (int)byte3;
        }
        if (dx > 1 || dx < -1) MOUSE.x += (dx / PS2_SPEED);
        if (dy > 1 || dy < -1) MOUSE.y -= (dy / PS2_SPEED);
        
        MOUSE.prev_left = MOUSE.left;
        MOUSE.left = (byte1 & 0x01) != 0;
        MOUSE.prev_right = MOUSE.right;
        MOUSE.right = (byte1 & 0x02) != 0;
        
        if (MOUSE.x < 5) MOUSE.x = 5;
        if (MOUSE.x > SCREEN_W - 5) MOUSE.x = SCREEN_W - 5;
        if (MOUSE.y < 5) MOUSE.y = 5;
        if (MOUSE.y > SCREEN_H - 5) MOUSE.y = SCREEN_H - 5;
    }

}

void draw_exit_button(int x0, int y0, int w, int h, short int line_colour, short int outer_colour, short int inner_colour, int scale) {
    // draw x base
    draw_rectangle(x0 - scale, y0 - scale, w + (2*scale), h + (2*scale), outer_colour);
    draw_rectangle(x0, y0, w, h, inner_colour);
    
    int padding = 3;
    EXIT_B.x_min = x0 - padding - scale;
    EXIT_B.x_max = EXIT_B.x_min + w + (2*scale) + padding;
    EXIT_B.y_min = y0 - padding - scale;
    EXIT_B.y_max = EXIT_B.y_min + h  + (2*scale) + padding;

    // draw actual x
    // diagonal 1
    draw_line (x0, y0 - 2, x0 + w + 2, y0 + w, line_colour);
    draw_line (x0, y0 - 1, x0 + w + 1, y0 + w, line_colour);
    draw_line (x0, y0, x0 + w, y0 + w, line_colour); // centre
    draw_line (x0, y0 + 1, x0 + w - 1, y0 + w, line_colour);
    draw_line (x0, y0 + 2, x0 + w - 2, y0 + w, line_colour);

    // diagonal 2
    draw_line (x0 + w, y0 - 2, x0 - 2, y0 + w, line_colour);
    draw_line (x0 + w, y0 - 1, x0 - 1, y0 + w, line_colour);
    draw_line (x0 + w, y0, x0, y0 + w, line_colour); // centre
    draw_line (x0 + w, y0 + 1, x0 + 1, y0 + w, line_colour);
    draw_line (x0 + w, y0 + 2, x0 + 2, y0 + w, line_colour);
}

void draw_push_button(int x, int y, int w, int h, int r, short int outer_colour, short int inner_colour, int scale, int mode, short int on) {
    if (mode == 0) {
        draw_rectangle(x, y, w, h, outer_colour);
        draw_rectangle(x + scale, y + scale, w - (2*scale), h - (2*scale), inner_colour);
        draw_circle(x + (w/2), y + (w/2), r, outer_colour);
        // four circles
        draw_circle(x + (w/6), y + (h/6), w/15, outer_colour);
        draw_circle(x + w - (w/6), y + (h/6), w/15, outer_colour);
        draw_circle(x + (w/6), y + h - (h/6), w/15, outer_colour);
        draw_circle(x + w - (w/6), y + h - (h/6), w/15, outer_colour);
    }
    else {
        draw_rectangle(x, y, w, h, outer_colour);
        draw_rectangle(x + scale, y + scale, w - (2*scale), h - (2*scale), inner_colour);
        draw_circle(x + (w/2), y + (w/2), r, on);
        // four circles
        draw_circle(x + (w/6), y + (h/6), w/15, outer_colour);
        draw_circle(x + w - (w/6), y + (h/6), w/15, outer_colour);
        draw_circle(x + (w/6), y + h - (h/6), w/15, outer_colour);
        draw_circle(x + w - (w/6), y + h - (h/6), w/15, outer_colour);
    }
}

void draw_switch(int x, int y, int w, int h, short int base, short int off, short int on, int scalex, int scaley, int mode) {
    if (mode == 0) { // off
        draw_rectangle(x, y, w, h, base);
        draw_rectangle(x + scalex, y + (scaley * 2), w - (scalex*2), h - (scaley*4), off);
        draw_rectangle(x + scalex, y + (h/2) , w - (scalex*2), ((h - (scaley*4)) / 2) + 1, on);
    }
    else { // on
        draw_rectangle(x, y, w, h, base);
        draw_rectangle(x + scalex, y + (scaley * 2), w - (scalex*2), h - (scaley*4), off);
        draw_rectangle(x + scalex, y + (scaley * 2) , w - (scalex*2), ((h - (scaley*4)) / 2) + 1, on);
    }
}

void draw_play_button(int x, int y_ctr, int h, int w, short int colour) {
    for (int y = 0; y < h; y++) {
        int y_off = y - h/2;
        int bw = w * ((h/2) - abs(y_off)) / (h/2);
        for (int x0 = 0; x0 < bw; x0++) {
            plot_pixel(x + x0, y_ctr + y_off, colour);
        }
    }
}

void draw_led(int x, int y, int w, int h, short int outer, short int inner, int scale, int mode, short int on){
    if (mode == 0) {
        draw_rectangle(x, y, w, h, outer);
        draw_rectangle(x + scale, y + scale, w - (scale * 2), h - (scale * 2), inner);
    }
    else {
        draw_rectangle(x, y, w, h, outer);
        draw_rectangle(x + scale, y + scale, w - (scale * 2), h - (scale * 2), on);
    }
}

void draw_control_bar(int x, int y, int w, int h, int scale, short int outer, short int inner, short int control) {

    // PAUSE
    draw_rectangle(x - scale, y - scale, w + (2*scale), h + (2*scale), outer);
    draw_rectangle(x, y, w, h, inner);

}

void wait_for_vsync() {
    volatile int* pixel_ctrl_ptr = (int *) FB_ADDR;
    *pixel_ctrl_ptr = 1; // start synchronization process by writing 1 into front buffer status register
    int status = *(pixel_ctrl_ptr + 3);
   
    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
}
