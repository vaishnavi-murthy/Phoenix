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
#include "usbkeyboard.h"
#include <stdlib.h>

#define SHIP_NAME 0
#define SHIP_BULLET_NAME 1
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

#define TOP_ROW 16
#define LAST_COLUMN 150
#define DISPLAY 1
#define DONT_DISPLAY 0
#define ROW_BIRD 40
#define ROW_SHIP 224

#define TIME_CONSTANT 10000
int vga_ball_fd;
int ship_lives = 2;
int bird1_lives = 1;
int bird2_lives = 1;
int bird3_lives = 1;
unsigned short score = 0;
hardware_p data;


struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

// new_y goes from 0 to 224
// new_x goes from 0 to 112

coordinates_t bird1_coor = {0,ROW_BIRD};
coordinates_t bird2_coor = {0,ROW_BIRD};
coordinates_t bird3_coor = {0,ROW_BIRD};
coordinates_t ship_coor = {60,ROW_SHIP};

pthread_t bird1_thread_shoot;
pthread_t bird2_thread_shoot;
pthread_t bird3_thread_shoot;
pthread_t ship_thread_shoot;
pthread_t move_birds_thread;

sprite_change_t ship = {1, SHIP_NAME, 60, ROW_SHIP, SHIP_NAME, DISPLAY};

sprite_change_t bird1 = {1, BIRD1_NAME, 0, ROW_BIRD, BIRD1_NAME, DISPLAY};

sprite_change_t bird2 = {1, BIRD2_NAME, 0, ROW_BIRD, BIRD1_NAME, DISPLAY};

sprite_change_t bird3 = {1, BIRD3_NAME, 0, ROW_BIRD, BIRD1_NAME, DISPLAY};

sprite_change_t bird1_bullet = {1, BIRD1_BULLET_NAME, 0, 0, BIRD1_BULLET_NAME,DONT_DISPLAY};

sprite_change_t bird2_bullet = {1, BIRD2_BULLET_NAME, 0, 0, BIRD1_BULLET_NAME,DONT_DISPLAY};

sprite_change_t bird3_bullet = {1, BIRD3_BULLET_NAME, 0, 0, BIRD1_BULLET_NAME,DONT_DISPLAY};

sprite_change_t ship_bullet = {1, SHIP_BULLET_NAME, 0, 0, SHIP_BULLET_NAME,DONT_DISPLAY};

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

void set_alldata(const hardware_p *c)
{
  vga_ball_arg_t vla;
  vla.alldata = *c;
  
  if( ioctl(vga_ball_fd, VGA_BALL_WRITE_ALLDATA, &vla))
  {
        perror("ioctl(VGA_BALL_SET_ALLDATA) failed");
        return;
  }
}

/* translate the joystick input into moves */
int input_moves(char *ks) {
	int move;

	if (ks[6] == 'f' && ks[12] == '1') move = 2;
	else if (ks[6] == 'f' && ks[12] == '0') move = 1;
	else if (ks[6] == '0' && ks[12] == '1') move = 0;
	else if (ks[6] == '0' && ks[12] == '0') move = -1;
	else if (ks[6] == '7' && ks[12] == '1') move = 3;
	else move = -2;

	return move;
}

void move_ship_left()
{
	if(ship_coor.x > 0) ship_coor.x -= 1;
	ship.new_x = ship_coor.x;

	set_new_args(&ship);
	usleep(TIME_CONSTANT);
}

void move_ship_right()
{
	if(ship_coor.x > LAST_COLUMN-5) ship_coor.x += 0;
	else ship_coor.x+=1;
        ship.new_x = ship_coor.x;

        set_new_args(&ship);
	usleep(TIME_CONSTANT);
}

int check_collision_with_birds(sprite_change_t *ship_bullet)
{
        if(ship_bullet->new_x >= bird1_coor.x - 2 && ship_bullet->new_x <= bird1_coor.x + 2 && ship_bullet->new_y >= bird1_coor.y && ship_bullet->new_y <= bird1_coor.y + 4)
                return 1;
	if(ship_bullet->new_x >= bird2_coor.x - 2 && ship_bullet->new_x <= bird2_coor.x + 2 && ship_bullet->new_y >= bird2_coor.y && ship_bullet->new_y <= bird2_coor.y + 4)
                return 2;
	if(ship_bullet->new_x >= bird3_coor.x - 2 && ship_bullet->new_x <= bird3_coor.x + 2 && ship_bullet->new_y >= bird3_coor.y && ship_bullet->new_y <= bird3_coor.y + 4)
                return 3;
        return 0;
}


void *ship_shoot_bullet()
{
	char prev_name;
        ship_bullet.new_tag = DISPLAY;
        ship_bullet.new_x = ship_coor.x;
        ship_bullet.new_y = ship_coor.y - 8;
        set_new_args(&ship_bullet);
	

        while((ship_bullet.new_y) > TOP_ROW)
        {
                int bird_hit = check_collision_with_birds(&ship_bullet);
        	switch (bird_hit)
                {
			case (1):
                		if (bird1_lives == 0)
                        	{
                                        bird1.new_name = EXPLOSION_NAME;
                                        set_new_args(&bird1);
					usleep(TIME_CONSTANT*2);
					bird1.new_tag = DONT_DISPLAY;
					set_new_args(&bird1);
					bird1_lives--;
					score++;
					data.score1 = score;
					set_alldata(&data);

                                }
                                else
                                        bird1_lives--;
					prev_name = bird1.new_name;
					bird1.new_name = EXPLOSION_NAME;
                                        set_new_args(&bird1);
                                        usleep(TIME_CONSTANT/2);
					bird1.new_name = prev_name;
                                        set_new_args(&bird1);
                                        
					score++;
                                        data.score1 = score;
                                        set_alldata(&data);
                                ship_bullet.new_tag = DONT_DISPLAY;
                                set_new_args(&ship_bullet);
                                break;

			case (2):
                                if (bird2_lives == 0)
                                {
                                        bird2.new_name = EXPLOSION_NAME;
                                        set_new_args(&bird2);
					usleep(TIME_CONSTANT*2);
					bird2.new_tag = DONT_DISPLAY;
                                        set_new_args(&bird2);
					bird2_lives--;
					score++;
                                        data.score1 = score;
                                        set_alldata(&data);
                                }
                                else
                                        bird2_lives--;
					prev_name = bird2.new_name;
                                        bird2.new_name = EXPLOSION_NAME;
                                        set_new_args(&bird2);
                                        usleep(TIME_CONSTANT/2);
                                        bird1.new_name = prev_name;
                                        set_new_args(&bird2);

					score++;
                                        data.score1 = score;
                                        set_alldata(&data);
                                ship_bullet.new_tag = DONT_DISPLAY;
                                set_new_args(&ship_bullet);
                                break;

			case (3):
                                if (bird3_lives == 0)
                                {
                                        bird3.new_name = EXPLOSION_NAME;
                                        set_new_args(&bird3);
					usleep(TIME_CONSTANT*2);
					bird3.new_tag = DONT_DISPLAY;
                                        set_new_args(&bird3);
					bird3_lives--;
					score++;
                                        data.score1 = score;
                                        set_alldata(&data);
                                }
                                else
                                        bird3_lives--;
					prev_name = bird3.new_name;
                                        bird3.new_name = EXPLOSION_NAME;
                                        set_new_args(&bird3);
                                        usleep(TIME_CONSTANT/2);
                                        bird3.new_name = prev_name;
                                        set_new_args(&bird3);

					score++;
                                        data.score1 = score;
                                        set_alldata(&data);
                                ship_bullet.new_tag = DONT_DISPLAY;
                                set_new_args(&ship_bullet);
                                break;
			case (0):
				ship_bullet.new_y -= 1;
				set_new_args(&ship_bullet);
                                break;
			default:
                                ship_bullet.new_y -= 1;
                                set_new_args(&ship_bullet);
                                break;
                }
                
                usleep(TIME_CONSTANT/2);
		if (ship_bullet.new_tag == DONT_DISPLAY)
			break;
        }
        ship_bullet.new_tag = DONT_DISPLAY;
        set_new_args(&ship_bullet);
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
*/

int check_collision_with_ship(sprite_change_t *bird_bullet)
{
	if(bird_bullet->new_x >= ship_coor.x - 2 && bird_bullet->new_x <= ship_coor.x + 2)
		return 1;
	return 0;
}

void *bird1_shoot_bullet()
{
	bird1_bullet.new_tag = DISPLAY;
	bird1_bullet.new_x = bird1_coor.x;
	bird1_bullet.new_y = bird1_coor.y + 8;
	set_new_args(&bird1_bullet);

	while((bird1_bullet.new_y) < 240)
	{
		if(bird1_bullet.new_y >= 212) {
			if (check_collision_with_ship(&bird1_bullet))
			{
				if (ship_lives == 0)
				{
					ship.new_name = EXPLOSION_NAME;
					set_new_args(&ship);
					usleep(TIME_CONSTANT*2);
                                        ship.new_tag = DONT_DISPLAY;
                                        set_new_args(&ship);
					ship_lives--;
				}
				else
					ship_lives--;
					char prev_name = ship.new_name;
                                        ship.new_name = EXPLOSION_NAME;
                                        set_new_args(&ship);
                                        usleep(TIME_CONSTANT/2);
                                        ship.new_name = prev_name;
                                        set_new_args(&ship);
				bird1_bullet.new_tag = DONT_DISPLAY;
				set_new_args(&bird1_bullet);
				break;
			}
		}
		bird1_bullet.new_y += 1;
		set_new_args(&bird1_bullet);
		usleep(TIME_CONSTANT/2);
        }
	bird1_bullet.new_tag = DONT_DISPLAY;
        set_new_args(&bird1_bullet);
}


void *bird2_shoot_bullet()
{
        bird2_bullet.new_tag = DISPLAY;
        bird2_bullet.new_x = bird2_coor.x;
        bird2_bullet.new_y = bird2_coor.y + 8;
        set_new_args(&bird2_bullet);

        while((bird2_bullet.new_y) < 240)
        {
                if(bird2_bullet.new_y >= 212) {
                        if (check_collision_with_ship(&bird2_bullet))
                        {
                                if (ship_lives == 0)
                                {
                                        ship.new_name = EXPLOSION_NAME;
                                        set_new_args(&ship);
					usleep(TIME_CONSTANT*2);
                                        ship.new_tag = DONT_DISPLAY;
                                        set_new_args(&ship);
					ship_lives--;
                                }
                                else
                                        ship_lives--;
					char prev_name = ship.new_name;
                                        ship.new_name = EXPLOSION_NAME;
                                        set_new_args(&ship);
                                        usleep(TIME_CONSTANT/2);
                                        ship.new_name = prev_name;
                                        set_new_args(&ship);
                                bird2_bullet.new_tag = DONT_DISPLAY;
                                set_new_args(&bird2_bullet);
				break;
                        }
                }
                bird2_bullet.new_y += 1;
                set_new_args(&bird2_bullet);
                usleep(TIME_CONSTANT/2);
        }
	bird2_bullet.new_tag = DONT_DISPLAY;
        set_new_args(&bird2_bullet);
}


void *bird3_shoot_bullet()
{
        bird3_bullet.new_tag = DISPLAY;
        bird3_bullet.new_x = bird3_coor.x;
        bird3_bullet.new_y = bird3_coor.y + 8;
        set_new_args(&bird3_bullet);

        while((bird3_bullet.new_y) < 240)
        {
                if(bird3_bullet.new_y >= 212) {
                        if (check_collision_with_ship(&bird3_bullet))
                        {
                                if (ship_lives == 0)
                                {
                                        ship.new_name = EXPLOSION_NAME;
                                        set_new_args(&ship);
					usleep(TIME_CONSTANT*2);
                                        ship.new_tag = DONT_DISPLAY;
                                        set_new_args(&ship);
					ship_lives--;
                                }
                                else
                                        ship_lives--;
					char prev_name = ship.new_name;
                                        ship.new_name = EXPLOSION_NAME;
                                        set_new_args(&ship);
                                        usleep(TIME_CONSTANT/2);
                                        ship.new_name = prev_name;
                                        set_new_args(&ship);
                                bird3_bullet.new_tag = DONT_DISPLAY;
                                set_new_args(&bird3_bullet);
				break;
                        }
                }
                bird3_bullet.new_y += 1;
                set_new_args(&bird3_bullet);
                usleep(TIME_CONSTANT/2);
        }
	bird3_bullet.new_tag = DONT_DISPLAY;
        set_new_args(&bird3_bullet);
}


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

		if (bird1_coor.x >= ship_coor.x - 2 && bird1_coor.x <= (ship_coor.x + 2) && bird1_bullet.new_tag == DONT_DISPLAY)
		{
			pthread_create(&bird1_thread_shoot, NULL, bird1_shoot_bullet, NULL);
		}
		if (bird2_coor.x >= ship_coor.x - 2 && bird2_coor.x <= (ship_coor.x + 2) && bird2_bullet.new_tag == DONT_DISPLAY)
                {
                        pthread_create(&bird2_thread_shoot, NULL, bird2_shoot_bullet, NULL);
                }
		if (bird3_coor.x >= ship_coor.x - 2 && bird3_coor.x <= (ship_coor.x + 2) && bird3_bullet.new_tag == DONT_DISPLAY)
                {
                        pthread_create(&bird3_thread_shoot, NULL, bird3_shoot_bullet, NULL);
                }
		usleep(TIME_CONSTANT*10);
	}

	bird1.new_name = BIRD_STANDSTILL_NAME;
        bird2.new_name = BIRD_STANDSTILL_NAME;
        bird3.new_name = BIRD_STANDSTILL_NAME;
	set_new_args(&bird1);
        set_new_args(&bird2);
        set_new_args(&bird3);
	usleep(TIME_CONSTANT*10);

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
		if (bird1_coor.x >= ship_coor.x - 2 && bird1_coor.x <= (ship_coor.x + 2) && bird1_bullet.new_tag == DONT_DISPLAY)
                {
                        pthread_create(&bird1_thread_shoot, NULL, bird1_shoot_bullet, NULL);
                }
		if (bird2_coor.x >= ship_coor.x - 2 && bird2_coor.x <= (ship_coor.x + 2) && bird2_bullet.new_tag == DONT_DISPLAY)
                {
                        pthread_create(&bird2_thread_shoot, NULL, bird2_shoot_bullet, NULL);
                }
                if (bird3_coor.x >= ship_coor.x - 2 && bird3_coor.x <= (ship_coor.x + 2) && bird3_bullet.new_tag == DONT_DISPLAY)
                {
                        pthread_create(&bird3_thread_shoot, NULL, bird3_shoot_bullet, NULL);
                }

		usleep(TIME_CONSTANT*10);
		
	}
        bird1.new_name = BIRD_STANDSTILL_NAME;
        bird2.new_name = BIRD_STANDSTILL_NAME;
        bird3.new_name = BIRD_STANDSTILL_NAME;
        set_new_args(&bird1);
        set_new_args(&bird2);
        set_new_args(&bird3);        
        usleep(TIME_CONSTANT*10);
    }
}

int main()
{
  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[15];
  int m;
  static const char filename[] = "/dev/vga_ball";
  score = 0;
  data.score1=score;
  set_alldata(&data);


  printf("VGA ball Userspace program started\n");


  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL) {
	  fprintf(stderr, "Did not find a joystick\n");
	  exit(1);
  }

  set_new_args(&ship);
  pthread_create(&move_birds_thread, NULL, move_birds, NULL);

  while(1){
    libusb_interrupt_transfer(keyboard, endpoint_address, (unsigned char *) &packet, sizeof(packet), &transferred, 0);
    if (transferred == sizeof(packet)) {
		sprintf(keystate, "%02x %02x %02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1], packet.keycode[2], packet.keycode[3]);
		
		m = input_moves(keystate);
		switch(m)
		{
			case(-1):
				move_ship_left();
				break;
			case(0):
				if(ship_bullet.new_tag == DONT_DISPLAY)
					pthread_create(&ship_thread_shoot, NULL, ship_shoot_bullet, NULL);
				move_ship_left();
				break;
			case(1):
				move_ship_right();
				break;
			case(2):
				if(ship_bullet.new_tag == DONT_DISPLAY)
                                        pthread_create(&ship_thread_shoot, NULL, ship_shoot_bullet, NULL);
                                move_ship_right();
                                break;
			case (3):
				if(ship_bullet.new_tag == DONT_DISPLAY)
                                        pthread_create(&ship_thread_shoot, NULL, ship_shoot_bullet, NULL);
                                break;
		}

  		if(bird1_lives < 0 && bird2_lives < 0 && bird3_lives < 0){
			printf("You won!");
			exit(1);
		}
		if(ship_lives <0){
			printf("You lost");
			exit(1);
		}

	}
	
	/*if(bird1_lives == 0)
                score1=1;
        else if(bird1_lives ==-1)
                score1=2;

        if(bird2_lives == 0)
                score2=1;
        else if(bird2_lives == -1)
                score2=2;

        if(bird3_lives == 0)
                score3=1;
        else if(bird3_lives == -1)
                score3=2;

        score=score1+score2+score3;
	printf("%d\n",score);        
        data.score1 = score;
        set_alldata(&data);*/

	


  }

	

  
  pthread_join(move_birds_thread, NULL);

  printf("User Program Terminated\n");
  return 0;
}

