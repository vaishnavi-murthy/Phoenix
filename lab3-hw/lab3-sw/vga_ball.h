#ifndef _VGA_BALL_H
#define _VGA_BALL_H

#include <linux/ioctl.h>


typedef struct {
	unsigned char sprite_change, sprite_num, new_x, new_y, new_name, new_tag;
} sprite_change_t;
  
typedef struct {
	unsigned char x, y;
} coordinates_t;

typedef struct {
  sprite_change_t sprite_args;
} vga_ball_arg_t;

#define VGA_BALL_MAGIC 'q'

/* ioctls and their arguments */
#define VGA_BALL_WRITE_BACKGROUND _IOW(VGA_BALL_MAGIC, 1, vga_ball_arg_t *)
#define VGA_BALL_READ_BACKGROUND  _IOR(VGA_BALL_MAGIC, 2, vga_ball_arg_t *)

#endif
