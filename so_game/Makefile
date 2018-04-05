CCOPTS= -Wall -g  -Wstrict-prototypes
LIBS= -lglut -lGLU -lGL -lm -lpthread
CC=gcc -std=gnu99
AR=ar


BINS=libso_game.a\
     so_game\
     test_packets_serialization\
     test_linked_list\
     test_world_extended\
     so_game_client\
     so_game_server\

OBJS = vec3.o\
       client_list.o\
       linked_list.o\
       logger.o\
       surface.o\
       image.o\
       vehicle.o\
       world.o\
       world_extended.o\
       world_viewer.o\
       so_game_protocol.o\

HEADERS=utils.h\
	logger.h\
	image.h\
	client_list.h\
	linked_list.h\
	so_game_protocol.h\
	surface.h\
	vec3.h\
	vehicle.h\
	world.h\
	world_extended.h\
	world_viewer.h\

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS)

libso_game.a: $(OBJS)
	$(AR) -rcs $@ $^
	$(RM) $(OBJS)

so_game: so_game.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

test_packets_serialization: test_packets_serialization.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)

test_linked_list: test_linked_list.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)


test_world_extended: test_world_extended.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)

so_game_client: so_game_client.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

so_game_server: so_game_server.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)
