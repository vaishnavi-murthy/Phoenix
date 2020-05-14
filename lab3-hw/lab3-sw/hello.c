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

int vga_ball_fd;
struct libusb_device_handle *keyboard;


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

/* Read and print the coordinates */
void print_coordinates() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("New_x: %d New_y: %d\n",
	 vla.background.new_x, vla.background.new_y);
}

/* Set the new position of the sprite */
void set_new_args(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
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

  printf("initial state: ");
  //print_coordinates();

  vga_ball_color_t color = {1, 0, 16, 52};
  set_new_args(&color);

  printf("User Program Terminated\n");
  return 0;
}
