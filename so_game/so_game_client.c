#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <GL/glut.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "grisonet.h"
#include "image.h"
#include "logger.h"
#include "so_game_protocol.h"
#include "surface.h"
#include "utils.h"
#include "vehicle.h"
#include "world_extended.h"
#include "world_viewer.h"
#include "logger.h"


/*
 * global variables
 */

int window;
Vehicle* vehicle; // The vehicle

int socket_desc;
int socket_udp;
struct sockaddr_in udp_server;
//wrapper of World
WorldExtended* we;
bool world_created = false;

//images
Image* map_elevation;
Image* map_texture;
Image* my_texture;

int my_id;

bool running = false;

bool udp_threads_created = false;
pthread_t UDP_sender, UDP_receiver;
pthread_attr_t attr1, attr2;

//--------------------------------------->

/*
 * TCP functions
 */


/*
 * Create TCP connection
 * the return value is assigned to socket_desc global variable
 */
void createTCPConnection(int server_port){
    int ret;
    struct sockaddr_in server_addr = {0};

    //create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Could not create socket.\n");

    //setting up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    //initialize a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Could not connect to the server.\n");

    logger_verbose(__func__, "Connection established..\n");
}


/*
 * get ID from Server
 * @return the id (int)
 */
int getIdFromServer(void){
    char id_packet_buffer[BUFFER_SIZE];
    size_t pi_len = sizeof(IdPacket);
    size_t msg_len;

    //Client ask an id to server
    PacketHeader id_header;
    id_header.type = GetId;
    IdPacket* id_packet = (IdPacket*)malloc(sizeof(IdPacket));
    id_packet->header = id_header;
    id_packet->id = -1;  //id = -1 to ask an id



    int bytes_to_send = Packet_serialize(id_packet_buffer, &id_packet->header);

    logger_verbose(__func__, "id_packet with :\n type\t%d\n size\t%d\n id\t%d",id_packet->header.type, id_packet->header.size, id_packet->id);

    logger_verbose(__func__, "Bytes to send : %d bytes.", bytes_to_send);

    //Send id request to server
    msg_len = griso_send(socket_desc, id_packet_buffer, bytes_to_send);
    ERROR_HELPER(msg_len, "Can't send id request to server.\n");

    logger_verbose(__func__, "Bytes sent : %d bytes.\n", msg_len);

    //Free the id_packet
    Packet_free(&id_packet->header);

    //Receive an id from server
    msg_len = griso_recv(socket_desc, id_packet_buffer, pi_len);
    ERROR_HELPER(msg_len, "Can't receive an id from server.\n");

    logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

    IdPacket* id_packet_deserialized = (IdPacket*)Packet_deserialize(id_packet_buffer, msg_len);
    int my_id = id_packet_deserialized->id;

    logger_verbose(__func__, "id_packet with : \n type\t%d\n size\t%d\n id\t%d",
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
  size_t msg_len;
  PacketHeader ph;

  ph.type = GetElevation;

  ImagePacket* request = (ImagePacket*) malloc(sizeof(ImagePacket));
  request->header = ph;
  request->id = -1;

  int size = Packet_serialize(buf_send, &request->header);
  if(size == -1) return NULL;

  logger_verbose(__func__,"Sending elevation surface request to server.\n");

  //Send map elevation request to server
  msg_len = griso_send(socket_desc, buf_send, size);
  ERROR_HELPER(msg_len, "Can't send map elevation request to server.\n");

  logger_verbose(__func__ , "Bytes sent : %d bytes.\n",msg_len);
  logger_verbose(__func__, "Receiving elevation surface from server.\n");

  //Receive header of map elevation post from server
  int ph_len = sizeof(PacketHeader);

  msg_len = griso_recv(socket_desc, buf_rcv, ph_len);
  ERROR_HELPER(msg_len, "Can't receive header of map elevation post from server.\n");

  logger_verbose(__func__, "Bytes received : %d bytes.\n", msg_len);

  PacketHeader* incoming_pckt=(PacketHeader*) buf_rcv;
  size = incoming_pckt->size-ph_len;

  //Receive leftovers of map elevation package from server
  msg_len = griso_recv(socket_desc, buf_rcv+ph_len, size);
  ERROR_HELPER(msg_len, "Can't receive leftovers of map elevation package from server.\n");

  ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);

  logger_verbose(__func__, "Bytes received : %d bytes.\n",msg_len);
  logger_verbose(__func__, "Elevation package with :\ntype\t%d\nsize\t%d\nid\t%d\n",
		 deserialized_packet->header.type, deserialized_packet->header.size,
		 deserialized_packet->id);

  //free && returns image
  Packet_free(&(request->header));
  Image* im=deserialized_packet->image;
  free(deserialized_packet);
  //Image_save(im, "./images/client_surface.pgm");
  return im;
}

/*
 * get Texture map from Server
 * @return the map (Image*)
 */
Image* getMapTextureFromServer(void){

    char buf_send[BUFFER_SIZE];
    char buf_rcv[BUFFER_SIZE];
    size_t msg_len;
    PacketHeader ph;

    ph.type = GetTexture;

    ImagePacket* request = (ImagePacket*) malloc(sizeof(ImagePacket));
    request->header = ph;
    request->id = -1;
    int size = Packet_serialize(buf_send, &(request->header));
    if(size == -1) return NULL;

    logger_verbose(__func__, "Sending map texture request to server.\n");

    //Sending map texture request to server
    msg_len = griso_send(socket_desc, buf_send, size);
    ERROR_HELPER(msg_len, "Can't send map texture request to server.\n");

    logger_verbose(__func__, "Bytes sent : %d bytes.\n", msg_len);
    logger_verbose(__func__, "Receving map texture from server.\n");

    //Receiving map texture header response from server
    int ph_len = sizeof(PacketHeader);

    msg_len = griso_recv(socket_desc, buf_rcv, ph_len);
    ERROR_HELPER(msg_len, "Can't receive map texture header response from server.\n");

    logger_verbose(__func__, "Bytes received : %d bytes.\n", msg_len);

    PacketHeader* incoming_pckt = (PacketHeader*)buf_rcv;
    size = incoming_pckt->size-ph_len;

    //Receiving leftovers of map texture package from server
    msg_len = griso_recv(socket_desc, buf_rcv+ph_len, size);
    ERROR_HELPER(msg_len, "Can't receiving leftovers of map texture package from server.\n");

    logger_verbose(__func__, "Bytes received : %d bytes.",msg_len);

    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);

    logger_verbose(__func__, "Texture packet with :\n type\t%d\n size\t%d\n id\t%d\n",
		   deserialized_packet->header.type, deserialized_packet->header.size,
		   deserialized_packet->id);

    //free
    Packet_free(&request->header);
    Image* im = deserialized_packet->image;
    //Image_save(im, "./images/client_texture.ppm");
    free(deserialized_packet);
    return im;
}

/*
 * post my vehicle texture to server
 */
int postVehicleTexture(Image* texture){
    char buf_send[BUFFER_SIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=PostTexture;
    request->header=ph;
    request->id = my_id;
    request->image=texture;

    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return -1;
    int bytes_sent=0;
    int ret=0;
    while(bytes_sent<size){
        ret=send(socket_desc,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Can't send my vehicle texture");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    logger_verbose(__func__, "Sent %d bytes",bytes_sent);
    return 0;
}

/*
 * get vehicle texture of user "id" from server
 * @return the texture
 */
Image* getVehicleTexture(int id){
    char buf_send[BUFFER_SIZE];
    char buf_rcv[BUFFER_SIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetTexture;
    request->header=ph;
    request->id=id;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return NULL;
    int bytes_sent=0;
    int ret=0;
    while(bytes_sent<size){
        ret=send(socket_desc,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Can't request a texture of a vehicle");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    Packet_free(&(request->header));

    int ph_len=sizeof(PacketHeader);
    int msg_len=0;
    while(msg_len<ph_len){
        ret=recv(socket_desc, buf_rcv+msg_len, ph_len-msg_len, 0);
        if (ret==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        msg_len+=ret;
        }
    PacketHeader* header=(PacketHeader*)buf_rcv;
    size=header->size-ph_len;

    msg_len=0;
    while(msg_len<size){
        ret=recv(socket_desc, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        msg_len+=ret;
        }

    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
	logger_verbose(__func__, "Received %d bytes",msg_len+ph_len);
    Image* im=deserialized_packet->image;

    free(deserialized_packet);
    return im;
}

/*
 * notify server that the client "id" (me!) is quitting
 */
int postQuitPacket(void){
    char buf_send[BUFFER_SIZE];
    IdPacket* idpckt=(IdPacket*)malloc(sizeof(IdPacket));
    PacketHeader ph;
    ph.type=Quit;
    idpckt->id = my_id;
    idpckt->header=ph;
    int size=Packet_serialize(buf_send,&(idpckt->header));
    logger_verbose(__func__, "Sending quit packet of %d bytes",size);
    int msg_len=0;
    while(msg_len<size){
        int ret=send(socket_desc,buf_send+msg_len,size-msg_len,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Can't send quit packet");
        if (ret==0) break;
        msg_len+=ret;
    }
    logger_verbose(__func__, "Quit packet was successfully sent");
    return 0;

}


//------------- UDP stuff -------------------------->


/*
 * Create UDP connection
 * the "return value" is assigned to socket_udp global variable
 * side effect on udp_server field too
 */
void createUDPConnection(void){
    uint16_t udp_port_number = htons((uint16_t)SERVER_UDP_PORT); // we use network byte order
	socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(socket_udp, "Can't create an UDP socket");
    //to handle global variable initialization
    struct sockaddr_in alias = {0};
	udp_server = alias; // some fields are required to be filled with 0
    udp_server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    udp_server.sin_family = AF_INET;
    udp_server.sin_port = udp_port_number;
    logger_verbose(__func__, "UDP socket successfully created");
}

/*
 * This thread sends only vehicleUpdatePackets to server
 * until running is true (set it to false to stop it)
 */
void* UDPSenderThread(void* args){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(running){
        sleep(10);
        printf("...");
    }

    printf("fine 1");

    return NULL;
}

/*
 * This thread receives only vehicleUpdatePackets from server
 * until running is true (set it to false to stop it)
 */
void* UDPReceiverThread(void* args){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while(running){
        sleep(10);
    }
    return NULL;
}

//--------------------------------------->


void cleanup(void){
    //FILLME join the threads (running = false;)

    logger_verbose(__func__, "Joining UDP threads");
    running = false;

    int ret;

    //TODO add error helper...
    if(udp_threads_created){
        ret = pthread_cancel(UDP_sender);
        ret = pthread_cancel(UDP_receiver);
        ret = pthread_join(UDP_sender, NULL);
        ret = pthread_join(UDP_receiver, NULL);
        pthread_attr_destroy(&attr1);
        pthread_attr_destroy(&attr2);
    }

    //FILLME post quitPacket to Server
    postQuitPacket();

    //close both sockets
    ret=close(socket_desc);
    ERROR_HELPER(ret,"Something went wrong closing TCP socket");
    ret=close(socket_udp);
    ERROR_HELPER(ret,"Something went wrong closing UDP socket");


    //FILLME clean other stuff

    // final cleanup
    //destroy World
    if(world_created){
        WorldExtended_destroy(we);
        //not needed anymore World_destroy(&world);
        logger_verbose(__func__, "worldextended deleted");
    }

    Image_free(map_elevation);
    Image_free(map_texture);
    Image_free(my_texture);
}

/*
 * Signal Handling
 */
void signalHandler(int signal){
    switch (signal) {
        case SIGHUP:
            break;
        case SIGINT:
            running=false;
            cleanup();
            break;
        default:
            fprintf(stderr, "Uncaught signal: %d\n", signal);
            return;
    }
}

//--------------------------------------->

int main(int argc, char **argv) {
    if (argc<3) {
        printf("usage: %s <player texture> <server port>\n", argv[0]);
        exit(-1);
    }

    printf("loading texture image from %s ... ", argv[1]);
    my_texture = Image_load(argv[1]);
    if (my_texture) {
        printf("Done! \n");
    } else {
        printf("Fail! \n");
    }

    int server_port = atoi(argv[2]);
    assert(server_port == SERVER_TCP_PORT);


    //setting signal handler
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    // Restart the system call
    sa.sa_flags = SA_RESTART;
    // Block every signal during the handler
    sigfillset(&sa.sa_mask);
    int ret = sigaction(SIGHUP, &sa, NULL);
    ERROR_HELPER(ret,"Cannot handle SIGHUP");
    ret=sigaction(SIGINT, &sa, NULL);
    ERROR_HELPER(ret,"Cannot handle SIGINT");



    //fixme is this needed?
    Image* my_texture_for_server= my_texture;

    // todo: connect to the server
    //   -get ad id
    //   -send your texture to the server (so that all can see you)
    //   -get an elevation map
    //   -get the texture of the surface

    // these come from the server


    //let's create a TCP socket
    createTCPConnection(server_port);

    //and here we get from server all needed stuff to play
    my_id = getIdFromServer();
    map_elevation= getMapElevationFromServer();
    map_texture= getMapTextureFromServer();
    ret = postVehicleTexture(my_texture_for_server);
    ERROR_HELPER(ret,"Cannot post my vehicle");
    Image* my_texture_from_server= getVehicleTexture(my_id);

    // construct the world
    //not needed anymore World_init(&world, map_elevation, map_texture, 0.5, 0.5, 0.5);

    we = WorldExtended_init(map_elevation, map_texture, 0.5, 0.5, 0.5);

    world_created = true;


    vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, we->w, my_id, my_texture_from_server);
    WorldExtended_addVehicle(we, vehicle);

    // spawn a thread that will listen the update messages from
    // the server, and sends back the controls
    // the update for yourself are written in the desired_*_force
    // fields of the vehicle variable
    // when the server notifies a new player has joined the game
    // request the texture and add the player to the pool
    /*FILLME*/


    //FILLME init UDP
    createUDPConnection();

    //FILLME spawn both UDP sender/receiver threads.

    //handle cancellation threads

    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);


    ret = pthread_create(&UDP_sender, &attr1, UDPSenderThread, NULL);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_sender");
    ret = pthread_create(&UDP_receiver, NULL, UDPReceiverThread, NULL);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_receiver");

    udp_threads_created = true;


    //here we can run the world and finally play
    WorldViewer_runGlobal(we->w, vehicle, &argc, argv);

    cleanup();
    return 0;
}
