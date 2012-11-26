CC = gcc
CFLAGS = 
SENDER_OBJS = sender.o
RECEIVER_OBJS = receiver.o

all: sender receiver

sender: $(SENDER_OBJS)
	$(CC) $(CFLAGS) $(SENDER_OBJS) -o sender
sender.o: sender.c
	$(CC) $(CFLAGS) -c sender.c -o sender.o

receiver: $(RECEIVER_OBJS)
	$(CC) $(CFLAGS) $(RECEIVER_OBJS) -o receiver
receiver.o: receiver.c
	$(CC) $(CFLAGS) -c receiver.c -o receiver.o


clean:
	rm -f $(SENDER_OBJS) $(RECEIVER_OBJS) sender receiver
