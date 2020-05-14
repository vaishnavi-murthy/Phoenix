CFLAGS = -Wall

OBJECTS = hello.o usbkeyboard.o

hello : $(OBJECTS)
	cc $(CFLAGS) -o hello $(OBJECTS) -lusb-1.0

hello.o: hello.c vga_ball.h usbkeyboard.h
usbkeyboard.o: usbkeyboard.c usbkeyboard.h

.PHONY : clean
clean :
	rm -rf *.o hello
