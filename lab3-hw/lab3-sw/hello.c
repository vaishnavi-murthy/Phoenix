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
#include <pthread.h>

#define SHIP_NAME 0
#define BIRD1_NAME 2
#define BIRD2_NAME 3
#define BIRD3_NAME 4
#define BIRD1_BULLET_NAME 5
#define BIRD2_BULLET_NAME 6
#define BIRD3_BULLET_NAME 7
#define EXPLOSION_NAME 6

#define BIRD_STANDSTILL_NAME 2
#define BIRD_RIGHT_NAME 4
#define BIRD_LEFT_NAME 3

#define TOP_ROW 32
#define LAST_COLUMN 112
#define DISPLAY 1
#define DONT_DISPLAY 0
#define ROW_BIRD 40
#define ROW_SHIP 224

int vga_ball_fd;

// new_y goes from 0 to 224
// new_x goes from 0 to 112

coordinates_t bird1_coor = {0,ROW_BIRD};
coordinates_t bird2_coor = {0,ROW_BIRD};
coordinates_t bird3_coor = {0,ROW_BIRD};
coordinates_t ship_coor = {0,ROW_SHIP};

pthread_t bird1_thread_shoot;
pthread_t bird2_thread_shoot;
pthread_t bird3_thread_shoot;
pthread_t move_birds_thread;

sprite_change_t ship = {1, SHIP_NAME, 0, ROW_SHIP, SHIP_NAME, DISPLAY};

sprite_change_t bird1 = {1, BIRD1_NAME, 0, ROW_BIRD, BIRD1_NAME, DISPLAY};

sprite_change_t bird2 = {1, BIRD2_NAME, 0, ROW_BIRD, BIRD1_NAME, DISPLAY};

sprite_change_t bird3 = {1, BIRD3_NAME, 0, ROW_BIRD, BIRD1_NAME, DISPLAY};


/* Read and print the coordinates */
/*void print_coordinates() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("New_x: %d New_y: %d\n",
	 vla.sprite_args.new_x, vla.sprite_args.new_y);
}*/

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

void move_ship_left()
{
	ship_coor.x -= 1;
	ship.new_x = ship_coor.x;

	set_new_args(&ship);
	usleep(4000);
}

void move_ship_right()
{
	ship_coor.x += 1;
        ship.new_x = ship_coor.x;

        set_new_args(&ship);
	usleep(4000);
}
/*
void player_shoot_bullet(coordinates_t *ship_coor, sprite_change_t *ship_sprite)
{
	ship_coor->y -= 32;
	sprite_change_t new_sprite = {1, PLAYER_BULLET_NAME, ship_coor->x, ship_coor->y, PLAYER_BULLET_NAME, DISPLAY};
	
	set_new_args(&new_sprite);
	set_new_args(ship_sprite);

	while((ship_coor->y) > TOP_ROW)
	{
		ship_coor->y -= 10;
		new_sprite.new_y_row1 =  ship_coor->y_row1;
		set_new_args(&new_sprite);
		set_new_args(ship_sprite);
		usleep(40000);
	}
}

void bird1_shoot_bullet()
{
	sprite_change_t bullet_sprite = {1, BIRD1_BULLET_NAME, bird1_coor.x, bird1_coor.y, BIRD1_BULLET_NAME, DISPLAY};
	coordinates_t bullet_coor = {bird1_coor.x, bird1_coor.y};
	
	while((bullet_coor.y) > 255)
	{
		bullet_coor.y -= 10;
		bullet_sprite.
}
*/

void *move_birds()
{
    while(1){
	for (int i = 0; i < LAST_COLUMN - 16; i++)
	{
		bird1_coor.x = i;
		bird1.new_x= bird1_coor.x;
		bird2_coor.x = i + 8;
                bird2.new_x= bird2_coor.x;
		bird3_coor.x = i + 16;
                bird3.new_x= bird3_coor.x;		
		bird1.new_name = BIRD_RIGHT_NAME;
		bird2.new_name = BIRD_RIGHT_NAME;
		bird3.new_name = BIRD_RIGHT_NAME;
		set_new_args(&bird1);
                set_new_args(&bird2);
                set_new_args(&bird3);
/*
		if (bird1_coor.x >= ship_coor.x && bird1_coor.x <= (ship_coor.x + 32))
		{
			pthread_create(&bird1_thread_shoot, NULL, bird1_shoot_bullet, NULL);
		}
		else if (bird2_coor.x == ship_coor.x)
		{
			pthread_create(&bird2_thread_shoot, NULL, bird2_shoot_bullet, NULL);
		}
		else if (bird3_coor.x == ship_coor.x)
		{
			pthread_create(&bird3_thread_shoot, NULL, bird3_shoot_bullet, NULL);
		}
*/
		usleep(40000);
	}

	bird1.new_name = BIRD_STANDSTILL_NAME;
        bird2.new_name = BIRD_STANDSTILL_NAME;
        bird3.new_name = BIRD_STANDSTILL_NAME;
	set_new_args(&bird1);
        set_new_args(&bird2);
        set_new_args(&bird3);
	usleep(400000);

	for (int i = LAST_COLUMN - 16; i > 0; i--)
        {
                bird1_coor.x = i;
                bird1.new_x= bird1_coor.x;
                bird2_coor.x = i + 8;
                bird2.new_x= bird2_coor.x;
                bird3_coor.x = i + 16;
                bird3.new_x= bird3_coor.x;
		bird1.new_name = BIRD_LEFT_NAME;
                bird2.new_name = BIRD_LEFT_NAME;
                bird3.new_name = BIRD_LEFT_NAME;
                set_new_args(&bird1);
                set_new_args(&bird2);
                set_new_args(&bird3);
		usleep(40000);
	}
        bird1.new_name = BIRD_STANDSTILL_NAME;
        bird2.new_name = BIRD_STANDSTILL_NAME;
        bird3.new_name = BIRD_STANDSTILL_NAME;
        set_new_args(&bird1);
        set_new_args(&bird2);
        set_new_args(&bird3);        
        usleep(400000);
    }
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

  pthread_create(&move_birds_thread, NULL, move_birds, NULL);
while(1){
  while(ship_coor.x < LAST_COLUMN){
    move_ship_right();
  }
  while(ship_coor.x > 0){
    move_ship_left();
  }
}
  pthread_join(move_birds_thread, NULL);
  //player_shoot_bullet(&ship_coordinates, &ship_sprite);
  // move_birds();
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

