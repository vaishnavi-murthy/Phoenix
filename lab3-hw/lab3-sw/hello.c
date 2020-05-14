/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include "vga_ball.h"
#include "usbkeyboard.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#define SHIP_NAME 0
#define BIRD1_NAME 2
#define BIRD2_NAME 3
#define BIRD3_NAME 4
#define BIRD1_BULLET_NAME 5
#define BIRD2_BULLET_NAME 6
#define BIRD3_BULLET_NAME 7
#define EXPLOSION_NAME 6

#define TOP_ROW 32
#define DISPLAY 1
#define DONT_DISPLAY 0
#define ROW_BIRD1 0
#define ROW_BIRD2 10
#define ROW_BIRD3 20

int vga_ball_fd;
struct libusb_device_handle *keyboard;
uint8_t endpoint_address;


/* Translate the joystick input into moves */
int input_moves(const char *ks) {
	int move;

	if (ks[6] == 'f' && ks[12] == '1') move = 2;  // Moving left and button is pressed
	else if (ks[6] == 'f' && ks[12] == '0') move = 1;
	else if (ks[6] == '0' && ks[12] == '1') move = 0;
	else if (ks[6] == '0' && ks[12] == '0') move = -1;
	else if (ks[6] == '7' && ks[12] == '1') move = 3;  // Just button is pressed
	else move = -2;

        return move;
}

coordinates_t bird1_coor = {0,0,0,0};
coordinates_t bird2_coor = {0,0,0,224};
coordinates_t bird3_coor = {0,0,1,56};
coordinates_t ship_coor = {0,0,1,224};

pthread_t bird1_thread_shoot;
pthread_t bird2_thread_shoot;
pthread_t bird3_thread_shoot;
pthread_t move_birds_thread;

sprite_change_t ship = {1, SHIP_NAME, ship_coor.x_column1, ship_coor.x_column2, ship_coor.y_row1, ship_coor.y_row2, SHIP_NAME, DISPLAY};

sprite_change_t bird1 = {1, BIRD1_NAME, bird1_coor.x_column1, bird1_coor.x_column2, bird1_coor.y_row1, bird1_coor.y_row2, BIRD1_NAME, DISPLAY};

sprite_change_t bird2 = {1, BIRD2_NAME, bird2_coor.x_column1, bird2_coor.x_column2, bird2_coor.y_row1, bird2_coor.y_row2, BIRD2_NAME, DISPLAY};

sprite_change_t bird3 = {1, BIRD3_NAME, bird3_coor.x_column1, bird3_coor.x_column2, bird3_coor.y_row1, bird3_coor.y_row2, BIRD3_NAME, DISPLAY};

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

void move_birds()
{
	for (int i = 0; i < 480; i++)
	{
		bird1_coor.x_column1 = i > 255? 1 : 0;
		bird1_coor.x_column2 = i > 255? i-255 : i;
		bird1.new_x_column1 = bird1_coor.x_column1;
		bird1.new_x_column2 = bird1_coor.x_column2;
		bird2_coor.x_column1 = (i+64) > 255? 1 : 0;
                bird2_coor.x_column2 = (i+64) > 255? i+64-255 : i;
                bird2.new_x_column1 = bird2_coor.x_column1;
                bird2.new_x_column2 = bird2_coor.x_column2;
		bird3_coor.x_column1 = (i+128) > 255? 1 : 0;
                bird3_coor.x_column2 = (i+128) > 255? i+128-255 : i;
                bird3.new_x_column1 = bird3_coor.x_column1;
                bird3.new_x_column2 = bird3_coor.x_column2;
		set_new_args(&bird1);
                set_new_args(&bird2);
                set_new_args(&bird3);
		if (bird1_coor.x  == ship_coor.x)
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

		usleep(40000);
	}
}

int main()
{
  vga_ball_arg_t vla;
  int i;
  static const char filename[] = "/dev/vga_ball";
  
  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[15];
  int m;

  printf ("VGA Ball Userspace program started\n");
  
  /* Open the Joystick */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL) {
	  fprintf(stderr, "Did not find a keyboard\n");
	  exit(1);
  }
  
  for (;;) {  // Expand for loop to end when the number of lives is done or game ends??
	  libusb_interrupt_transfer(keyboard, endpoint_address, (unsigned char *) &packet, sizeof(packet), &transferred, 0);
	  if (transferred == sizeof(packet)) {
		  sprintf(keystate, "%02x %02x %02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1], packet.keycode[2], packet.keycode[3]);
		  //m = input_moves(keystate)
		  printf("%s\n", keystate);  // Check if this if block is working now that theres no server part
	  }
  }


  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  set_new_args(&ship_sprite);

  set_new_args(&bird1);

  set_new_args(&bird2);

  set_new_args(&bird3);

  pthread_create(&move_birds_thread, NULL, move_birds, NULL);

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

