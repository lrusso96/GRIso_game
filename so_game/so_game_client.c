#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <GL/glut.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "image.h"
#include "logger.h"
#include "so_game_client.h"
#include "so_game_protocol.h"
#include "surface.h"
#include "utils.h"
#include "vehicle.h"
#include "world.h"
#include "world_viewer.h"


/*
 * global variables
 */

int window;
World world;
Vehicle* vehicle; // The vehicle

int socket_desc;

//todo can convert to bool? (stdbool.h)
int running = 0;

//--------------------------------------->

/*
 * TCP functions
 */


/*
 * Create TCP connection
 * the return value is assigned to socket_desc global variable
 */
void createConnection(void){
    int ret;
    struct sockaddr_in server_addr = {0};

    //create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Could not create socket.\n");

    //setting up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    //initialize a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Could not connect to the server.\n");

    logger_print(__func__, "Connection established..\n");
}


/*
 * get ID from Server
 * @return the id (int)
 */
int getIdFromServer(void){
    char id_packet_buffer[BUFFER_SIZE];
    size_t id_buffer_size = sizeof(id_packet_buffer);
    size_t msg_len;
    int ret;

    //Client ask an id to server
    PacketHeader id_header;
    id_header.type = GetId;
    IdPacket* id_packet = (IdPacket*)malloc(sizeof(IdPacket));
    id_packet->header = id_header;
    id_packet->id = -1;  //id = -1 to ask an id
    logger_print(__func__, "id_packet with :\n type\t%d\n size\t%d\n id\t%d",id_packet->header.type, id_packet->header.size, id_packet->id);

    int bytes_to_send = Packet_serialize(id_packet_buffer, &id_packet->header);
    logger_print(__func__, "Bytes to send : %d", bytes_to_send);

    while(1){
    ret = send(socket_desc, id_packet_buffer, bytes_to_send, 0);
    if(ret == -1){
      if(errno == EINTR)
    continue;
        ERROR_HELPER(ret, "Attempt to require an id to server failed..\n");
      }
      break;
    }
    logger_print(__func__, "Bytes sent : %d", ret);

    //Free the id_packet
    Packet_free(&id_packet->header);

    //Receive an id from server
    while(1){
    msg_len = recv(socket_desc, id_packet_buffer, id_buffer_size, 0);
    if(msg_len == -1){
      if(errno == EINTR)
    continue;
      ERROR_HELPER(msg_len, "Can't receive an id from server.\n");
    }
    break;
    }
    logger_print(__func__, "Bytes received : %zu", msg_len);

    IdPacket* id_packet_deserialized = (IdPacket*)Packet_deserialize(id_packet_buffer, msg_len);
    int my_id = id_packet_deserialized->id;
    logger_print(__func__, "id_packet with : \n type\t%d\n size\t%d\n id\t%d",
    id_packet_deserialized->header.type, id_packet_deserialized->header.size, id_packet_deserialized->id);

    //Free id_packet
    Packet_free(&id_packet_deserialized->header);
    return my_id;
}

/*
 * get Elevation map from Server
 * @return the map (Image*)
 */
Image* getMapElevationFromServer(void){
    char buf_send[BUFFER_SIZE];
    char buf_rcv[BUFFER_SIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetElevation;
    request->header=ph;
    request->id=-1;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return NULL;
    int bytes_sent=0;
    int ret=0;

    while(bytes_sent<size){
        ret=send(socket_desc,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Can't send MapElevation request");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    logger_print(__func__ ,"Sent %d bytes",bytes_sent);

    int msg_len=0;
    int ph_len=sizeof(PacketHeader);
    while(msg_len<ph_len){
        ret=recv(socket_desc, buf_rcv, ph_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }

    PacketHeader* incoming_pckt=(PacketHeader*)buf_rcv;
    size=incoming_pckt->size-ph_len;
    msg_len=0;
    while(msg_len<size){
        ret=recv(socket_desc, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }

    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
    logger_print(__func__, "Received %d bytes",msg_len+ph_len);

    //free && returns image
    Packet_free(&(request->header));
    Image* im=deserialized_packet->image;
    free(deserialized_packet);
    return im;
}

/*
 * get Texture map from Server
 * @return the map (Image*)
 */
Image* getMapTextureFromServer(void){
    char buf_send[BUFFER_SIZE];
    char buf_rcv[BUFFER_SIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetTexture;
    request->header=ph;
    request->id=-1;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return NULL;
    int bytes_sent=0;
    int ret=0;

    while(bytes_sent<size){
        ret=send(socket_desc,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Can't send MapTexture");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    logger_print(__func__, "Sent %d bytes",bytes_sent);
    int msg_len=0;
    int ph_len=sizeof(PacketHeader);

    while(msg_len<ph_len){
        ret=recv(socket_desc, buf_rcv, ph_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }
    PacketHeader* incoming_pckt=(PacketHeader*)buf_rcv;
    size=incoming_pckt->size-ph_len;
    logger_print(__func__, "Received %d bytes",size);

    msg_len=0;
    while(msg_len<size){
        ret=recv(socket_desc, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }
    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
    logger_print(__func__, "Received %d bytes %d",msg_len+ph_len);

    //free
    Packet_free(&(request->header));
    Image* im=deserialized_packet->image;
    free(deserialized_packet);
    return im;

}

Image* getMyTextureFromServer(Image* my_texture_for_server, int id){
  return NULL;
}

//--------------------------------------->

/*
 * UDP threads routines
 */

void* UDPSenderThread(void* args){
    return NULL;
}

void* UDPReceiverThread(void* args){
    return NULL;
}

//--------------------------------------->

/*
 * Signal Handling
 */
void signalHandler(int signal){
    switch (signal) {
        case SIGHUP:
            break;
        case SIGINT:
            running=0;
            break;
        default:
            fprintf(stderr, "Uncaught signal: %d\n", signal);
            return;
    }
}

//--------------------------------------->

int main(int argc, char **argv) {
  if (argc<2) {
    printf("usage: %s <player texture>\n", argv[1]);
    exit(-1);
  }

  printf("loading texture image from %s ... ", argv[2]);
  Image* my_texture = Image_load(argv[2]);
  if (my_texture) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }

  //fixme is this needed?
  Image* my_texture_for_server= my_texture;

  // todo: connect to the server
  //   -get ad id
  //   -send your texture to the server (so that all can see you)
  //   -get an elevation map
  //   -get the texture of the surface

  // these come from the server
  int my_id = getIdFromServer();
  Image* map_elevation= getMapElevationFromServer();
  Image* map_texture= getMapTextureFromServer();
  Image* my_texture_from_server= getMyTextureFromServer(my_texture_for_server, my_id);

  // construct the world
  World_init(&world, map_elevation, map_texture, 0.5, 0.5, 0.5);
  vehicle=(Vehicle*) malloc(sizeof(Vehicle));
  Vehicle_init(vehicle, &world, my_id, my_texture_from_server);
  World_addVehicle(&world, vehicle);

  // spawn a thread that will listen the update messages from
  // the server, and sends back the controls
  // the update for yourself are written in the desired_*_force
  // fields of the vehicle variable
  // when the server notifies a new player has joined the game
  // request the texture and add the player to the pool
  /*FILLME*/


    //FILLME spawn both UDP sender/receiver threads.


  WorldViewer_runGlobal(&world, vehicle, &argc, argv);

    //FILLME join the threads

    // cleanup
    World_destroy(&world);
    return 0;
}
