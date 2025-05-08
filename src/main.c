#include <stdlib.h>
#include <string.h>

#include "pixler.h"
#include "common.h"

#define BG_COLOR 0x31
static const u8 PALETTE[] = {
	BG_COLOR, 0x00, 0x10, 0x20,
	BG_COLOR, 0x06, 0x16, 0x26,
	BG_COLOR, 0x09, 0x19, 0x29,
	BG_COLOR, 0x01, 0x11, 0x21,
	
	BG_COLOR, 0x00, 0x10, 0x20,
	BG_COLOR, 0x06, 0x16, 0x26,
	BG_COLOR, 0x09, 0x19, 0x29,
	BG_COLOR, 0x01, 0x11, 0x21,
};

Gamepad pad1, pad2;

void read_gamepads(void){
	pad1.prev = pad1.value;
	pad1.value = joy_read(0);
	pad1.press = pad1.value & (pad1.value ^ pad1.prev);
	pad1.release = pad1.prev & (pad1.value ^ pad1.prev);
	
	pad2.prev = pad2.value;
	pad2.value = joy_read(1);
	pad2.press = pad2.value & (pad2.value ^ pad2.prev);
	pad2.release = pad2.prev & (pad2.value ^ pad2.prev);
}

#pragma bss-name (push, "ZEROPAGE")
static s8 DROP_X, DROP_Y;
static u8 DROP_ROT;

static s8 *DROP_OFF_X, *DROP_OFF_Y;
#pragma bss-name (pop)

typedef struct {
	s8 off_x[4], off_y[4];
} RotatedPiece;

static RotatedPiece piece[] = {
	{{+0, -1, +0, +1}, {+0, +0, +1, +0}},
	{{+0, +0, +1, +0}, {+0, -1, +0, +1}},
	{{+0, -1, +0, +1}, {+0, +0, -1, +0}},
	{{+0, +0, -1, +0}, {+0, -1, +0, +1}},
};

static void sprite_block(s8 x, s8 y){
	px_spr(16*x + 48, 216 - 16*y - 1, 0, 0x01);
	px_spr(16*x + 56, 216 - 16*y - 1, 0, 0x01);
	px_spr(16*x + 48, 208 - 16*y - 1, 0, 0x01);
	px_spr(16*x + 56, 208 - 16*y - 1, 0, 0x01);
}

static void sprite_piece(void){
	sprite_block(DROP_X + DROP_OFF_X[0], DROP_Y + DROP_OFF_Y[0]);
	sprite_block(DROP_X + DROP_OFF_X[1], DROP_Y + DROP_OFF_Y[1]);
	sprite_block(DROP_X + DROP_OFF_X[2], DROP_Y + DROP_OFF_Y[2]);
	sprite_block(DROP_X + DROP_OFF_X[3], DROP_Y + DROP_OFF_Y[3]);
}

static void set_block(s8 x, s8 y){
	u8 block[4] = {0x01, 0x01, 0x01, 0x01};
	px_buffer_blit(NT_ADDR(0, 2*x + 6, 26 - 2*y), block + 0, 2);
	px_buffer_blit(NT_ADDR(0, 2*x + 6, 27 - 2*y), block + 2, 2);
}

static void set_piece(void){
	set_block(DROP_X + DROP_OFF_X[0], DROP_Y + DROP_OFF_Y[0]);
	set_block(DROP_X + DROP_OFF_X[1], DROP_Y + DROP_OFF_Y[1]);
	set_block(DROP_X + DROP_OFF_X[2], DROP_Y + DROP_OFF_Y[2]);
	set_block(DROP_X + DROP_OFF_X[3], DROP_Y + DROP_OFF_Y[3]);
}

static void tetris_state(void){
	DROP_X = 5, DROP_Y = 10, DROP_ROT = 0;
	
	px_ppu_sync_disable();{
		px_addr(PAL_ADDR);
		px_blit(32, PALETTE);
		
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_SPLASH);
		// px_addr(NT_ADDR(0, 0, 1));
		// px_fill(32*30, 0x00);
	} px_ppu_sync_enable();
	
	while(true){
		read_gamepads();
		if(JOY_LEFT (pad1.press)) DROP_X -= 1;
		if(JOY_RIGHT(pad1.press)) DROP_X += 1;
		if(JOY_DOWN (pad1.press)) DROP_Y -= 1;
		if(JOY_UP   (pad1.press)) DROP_Y += 1;
		if(JOY_BTN_A(pad1.press)) DROP_ROT = (DROP_ROT + 1)&3;
		if(JOY_BTN_B(pad1.press)) DROP_ROT = (DROP_ROT - 1)&3;
		
		DROP_OFF_X = piece[DROP_ROT].off_x;
		DROP_OFF_Y = piece[DROP_ROT].off_y;
		
		if(JOY_START(pad1.press)){
			set_piece();
			DROP_X = 5, DROP_Y = 10, DROP_ROT = 0;
		} else {
			sprite_piece();
		}
		
		px_spr_end();
		px_wait_nmi();
	}
	
	tetris_state();
}

void main(void){
	px_uxrom_select(0);
	joy_install(nes_stdjoy_joy);
	
	px_lz4_to_vram(CHR_ADDR(0, 0), CHR0);
	px_bg_table(0);
	px_spr_table(0);
	
	
	music_init(&MUSIC);
	sound_init(&SOUNDS);
	music_play(0);
	
	tetris_state();
}
