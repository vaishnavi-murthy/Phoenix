/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define SHIP_NAME 0
#define BIRD_NAME 2
#define PLAYER_BULLET_NAME 5
#define EXPLOSION_NAME 6

#define TOP_ROW 32
#define DISPLAY 1
#define DONT_DISPLAY 0

int vga_ball_fd;

/* Read and print the coordinates */
void print_coordinates() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("New_x: %d New_y: %d\n",
	 vla.sprite_args.new_x, vla.sprite_args.new_y);
}

/* Set the new position of the sprite */
void set_new_args(const sprite_change_t *c)
{
  vga_ball_arg_t vla;
  vla.sprite_args = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}

void move_ship_left(coordinates_t *ship_coordinates)
{
	sprite_change_t new_sprite = {1, SHIP_NAME, 0, 0, SHIP_NAME, DISPLAY};
	ship_coordinates->x = (ship_coordinates->x) - 10;
	new_sprite.new_x = ship_coordinates->x;
	new_sprite.new_y = ship_coordinates->y;

	set_new_args(&new_sprite);
}

void move_ship_right(coordinates_t *ship_coordinates)
{
        sprite_change_t new_sprite = {1, SHIP_NAME, 0, 0, SHIP_NAME, DISPLAY};
        ship_coordinates->x = (ship_coordinates->x) + 10;
        new_sprite.new_x = ship_coordinates->x;
        new_sprite.new_y = ship_coordinates->y;

        set_new_args(&new_sprite);
}

void player_shoot_bullet(coordinates_t *ship_coor, sprite_change_t *ship_sprite)
{
	ship_coor->y -= 32;
	sprite_change_t new_sprite = {1, PLAYER_BULLET_NAME, ship_coor->x, ship_coor->y, PLAYER_BULLET_NAME, DISPLAY};
	
	set_new_args(&new_sprite);
	set_new_args(ship_sprite);

	while((ship_coor->y) > TOP_ROW)
	{
		ship_coor->y -= 10;
		new_sprite.new_y =  ship_coor->y;
		set_new_args(&new_sprite);
		set_new_args(ship_sprite);
		usleep(40000);
	}
}

void move_birds()
{
	sprite_change_t bird1 = {1, BIRD_NAME, 
}

int main()
{
  vga_ball_arg_t vla;
  int i;
  static const char filename[] = "/dev/vga_ball";


  printf("VGA ball Userspace program started\n");


  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  printf("initial state: ");
  //print_coordinates();

  coordinates_t ship_coordinates = {255, 255};

  sprite_change_t ship_sprite = {1, SHIP_NAME, ship_coordinates.x, ship_coordinates.y, SHIP_NAME, DISPLAY};
  set_new_args(&ship_sprite);


  player_shoot_bullet(&ship_coordinates, &ship_sprite);
/*
  int x = 0;
  while(x < 10){
    move_ship_left(&ship_coordinates);
    usleep(40000);
x++;
  }

player_shoot_bullet(&ship_coordinates);
*/
  printf("User Program Terminated\n");
  return 0;
}

