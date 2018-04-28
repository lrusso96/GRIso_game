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


//----------global variables------------------------------------------->

bool running = false;

/*
 * Socket descriptors (TCP & UDP)
 */
int socket_desc, socket_udp;
struct sockaddr_in udp_server;

/*
 * Threads stuff
 */
pthread_t UDP_sender, UDP_receiver;
pthread_attr_t attr1, attr2;
bool udp_threads_created = false;

/*
 * Wrapper of World (local copy)
 */
WorldExtended* we;
bool world_created = false;

/*
 * Textures
 */
Image* map_elevation;
Image* map_texture;
Image* my_texture;

/*
 * My global id
 */
int my_id;

/*
 * My own Vehicle
 */
Vehicle* vehicle;

/*
 * Window
 */
int window;




//----------TCP functions---------------------------------------------->


/*
 * Create TCP connection
 * @return value is assigned to socket_desc global variable
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
void getIdFromServer(void){
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

    logger_verbose(__func__, "id_packet with :\ntype\t%d\nsize\t%d\nid\t%d",id_packet->header.type, id_packet->header.size, id_packet->id);

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

    //return value (global side effect)
    my_id = id_packet_deserialized->id;

    logger_verbose(__func__, "id_packet with : \ntype\t%d\nsize\t%d\nid\t%d",
		   id_packet_deserialized->header.type, id_packet_deserialized->header.size, id_packet_deserialized->id);

    //Free id_packet
    Packet_free(&id_packet_deserialized->header);
}

/*
 * get Elevation map from Server
 * @return the map (Image*)
 */
void getMapElevationFromServer(void){

  char buf_send[BUFFER_SIZE];
  char buf_rcv[BUFFER_SIZE];
  size_t msg_len;
  PacketHeader ph;
  ph.type = GetElevation;

  ImagePacket* request = (ImagePacket*) malloc(sizeof(ImagePacket));
  request->header = ph;
  request->id = -1;

  int size = Packet_serialize(buf_send, &request->header);
  if(size == -1) return;

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

    //return value (global side effect)
  map_elevation = deserialized_packet->image;

  free(deserialized_packet);
  //Image_save(im, "./images/client_surface.pgm");
}

/*
 * get Texture map from Server
 * @return the map (Image*)
 */
void getMapTextureFromServer(void){

    char buf_send[BUFFER_SIZE];
    char buf_rcv[BUFFER_SIZE];
    size_t msg_len;
    PacketHeader ph;

    ph.type = GetTexture;

    ImagePacket* request = (ImagePacket*) malloc(sizeof(ImagePacket));
    request->header = ph;
    request->id = -1;
    int size = Packet_serialize(buf_send, &(request->header));
    if(size == -1) return;

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

    logger_verbose(__func__, "Texture packet with :\ntype\t%d\nsize\t%d\n id\t%d\n",
		   deserialized_packet->header.type, deserialized_packet->header.size,
		   deserialized_packet->id);

    //free
    Packet_free(&request->header);

    //return value (global side effect)
    map_texture=  deserialized_packet->image;

    //Image_save(im, "./images/client_texture.ppm");
    free(deserialized_packet);
}

/*
 * post my vehicle texture to server
 */
int postVehicleTexture(Image* texture, int id){

    char buf_send[BUFFER_SIZE];
    size_t msg_len;

    logger_verbose(__func__, "Sending vehicle texture to server.\n");

    ImagePacket* request = (ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;

    ph.type = PostTexture;
    request->header = ph;
    request->id = id;
    request->image = texture;

    int size = Packet_serialize(buf_send, &(request->header));
    if(size == -1) return -1;

    //Send vehicle texture to server
    msg_len = griso_send(socket_desc, buf_send, size);
    ERROR_HELPER(msg_len, "Can't send vehicle texture to server.\n");

    logger_verbose(__func__, "Bytes sent : %zu bytes.", msg_len);

    free(request);

    return 0;
}

/*
 * get vehicle texture of user "id" from server
 * @return the texture
 */
Image* getVehicleTexture(int id){

    char buf_send[BUFFER_SIZE];
    char buf_rcv[BUFFER_SIZE];

    ImagePacket* request = (ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    size_t msg_len;
    size_t ip_len = sizeof(ImagePacket);

    logger_verbose(__func__, "Sending player texture request to server.\n");

    ph.type = GetTexture;
    request->header = ph;
    request->id = id;

    int size = Packet_serialize(buf_send, &(request->header));
    if(size == -1)
        return NULL;

    msg_len = griso_send(socket_desc, buf_send, size);
    ERROR_HELPER(msg_len, "Can't send player texture request to server.\n");

    logger_verbose(__func__, "Bytes sent : %zu bytes.\n", msg_len);

    Packet_free(&(request->header));

    //Receiving player texture from server
    msg_len = griso_recv(socket_desc, buf_rcv, ip_len);
    ERROR_HELPER(msg_len, "Can't receive player texture header from server.\n");

    logger_verbose(__func__, "Bytes received : %zu.\n", msg_len);

    ImagePacket* incoming_packet = (ImagePacket*) buf_rcv;
    size = incoming_packet->header.size - ip_len;

    msg_len = griso_recv(socket_desc, buf_rcv+ip_len, size);
    ERROR_HELPER(msg_len, "Can't receive player texture package from server.\n");

    logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

    ImagePacket* deserialized_packet = (ImagePacket*) Packet_deserialize(buf_rcv, msg_len+ip_len);

    logger_verbose(__func__, "deserialized_packet with : \ntype\t%d\nsize\t%d\nid\t%d\n",
		   deserialized_packet->header.type, deserialized_packet->header.size,
		   deserialized_packet->id);

    Image* im = deserialized_packet->image;

    //Image_save(im, "./images/player_texture.ppm");
    free(deserialized_packet);

    return im;
}


/*
 * notify server that the client "id" (me!) is quitting
 */
void postQuitPacket(void){
    char buf_send[BUFFER_SIZE];
    IdPacket* idpckt=(IdPacket*)malloc(sizeof(IdPacket));
    PacketHeader ph;
    ph.type=Quit;
    idpckt->id = my_id;
    idpckt->header = ph;

    int size = Packet_serialize(buf_send,&(idpckt->header));
    logger_verbose(__func__, "Sending quit packet of %d bytes",size);
    int msg_len = griso_send(socket_desc, buf_send, size);
    ERROR_HELPER(msg_len, "Can't send id request to server.\n");

    logger_verbose(__func__, "Quit packet was successfully sent");
}


//----------UDP stuff-------------------------------------------------->


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

    //enable asynchronous pthread_cancel from cleanup
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //this is scalar (not global epoch sent by server)
    unsigned long req_number = 1;

    char buf_send[BUFFER_SIZE];
    VehicleUpdatePacket* vpckt=(VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
    PacketHeader ph;
    ph.type=VehicleUpdate;
    vpckt->id = my_id;
    vpckt->header = ph;

    while(running){

        //todo decide sleeping time
        usleep(1000*300);

        vpckt->req_number = req_number++;

        //done in mutual exclusion
        WorldExtended_vehicleUpdatePacket_init(we, vpckt, vehicle);

        int size = Packet_serialize(buf_send,&(vpckt->header));
        logger_verbose(__func__, "Sending updating packet of %d bytes\n\tid = %d\n\tx = %fn\ty = %fn\ttheta = %fn\trf = %fn\ttf = %f",size,
        vpckt->id, vpckt->x, vpckt->y, vpckt->theta, vpckt->rotational_force, vpckt->translational_force);

        int sent = sendto(socket_udp, buf_send, size, 0, (struct sockaddr *) &udp_server, sizeof(udp_server));
        if(sent<0)
            logger_error(__func__, "Update packet NOT sent successfully");
        else{
            logger_verbose(__func__, "Update packet successfully sent");
        }
    }

    return NULL;
}


void applyUpdates(WorldUpdatePacket* wup){

    logger_verbose(__func__, "vehicles = %d", wup->num_vehicles);

    for(int i= 0; i<wup->num_vehicles; i++){
        ClientUpdate cu = wup->updates[i];

        logger_verbose(__func__, "vehicle %d with %f, %f, %f\t%f, %f", cu.id, cu.x, cu.y, cu.theta, cu.rotational_force, cu.translational_force);

        int id = cu.id;
        //skip my id !

        if(id == my_id)
            continue;


        float x = cu.x;
        float y = cu.y;
        float theta = cu.theta;
        float rf = cu.rotational_force;
        float tf = cu.translational_force;



        //check if it is already in the world
        //if yes, update its poisiton
        //if not, request his texture and add to world

        int val = WorldExtended_HasIdAndTexture(we, id);
        if(val==-1){
            logger_verbose(__func__, "vehicle with id %d joined the game", id);
            Image * t = getVehicleTexture(id);

            //init my vehicle with images got from server
            Vehicle* new_v = (Vehicle*) malloc(sizeof(Vehicle));
            Vehicle_init(new_v, we->w, id, t);

            //and adding it to my world
            WorldExtended_addVehicle(we, new_v);
        }
        else if(val == 0){
            logger_verbose(__func__, "updating vehicle with id %d", id);
        }

        //update its position
        WorldExtended_setVehicleXYTPlus(we, id, x, y, theta, rf, tf);

    }

	World_update(we->w);

    //free packet
    Packet_free(&(wup->header));
}


/*
 * This thread receives only vehicleUpdatePackets from server
 * until running is true (set it to false to stop it)
 */
void* UDPReceiverThread(void* args){

    //enable asynchronous pthread_cancel from cleanup
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);



    char buf_recv[BUFFER_SIZE];

    while(running){

        usleep(1000*300);

        int nBytes = recvfrom(socket_udp, buf_recv, BUFFER_SIZE, 0, NULL, NULL);
        logger_verbose(__func__, "received %d bytes", nBytes);

        PacketHeader* ph=(PacketHeader*)buf_recv;

        if(ph->size!=nBytes){
            logger_error(__func__, "Partial UDP packet arrived, I'm ignoring it");
            continue;
        }

        if(ph->type == WorldUpdate){
            WorldUpdatePacket* wup = (WorldUpdatePacket*) Packet_deserialize(buf_recv, nBytes);
            logger_verbose(__func__, "WorldUpdate epoch %lu", wup->epoch);
            applyUpdates(wup);
            continue;
        }
        if(ph->type == Quit){
            IdPacket* ip = (IdPacket*) Packet_deserialize(buf_recv, nBytes);
            int id = ip->id;

            logger_verbose(__func__, "Vehicle %d disconnected. I detach it from my world", id);
            int id_ret = WorldExtended_detachVehicle(we, id);
            logger_verbose(__func__, "Detached vehicle %d", id_ret);
            continue;
        }
    }

    return NULL;
}

//----------CLEANUP && SIGNALS----------------------------------------->


void cleanup(void){
    int ret;

    //set global bool to false
    running = false;

    //join the threads if created!
    if(udp_threads_created){
        logger_verbose(__func__, "Joining UDP threads");
        ret = pthread_cancel(UDP_sender);
        ERROR_HELPER(ret, "pthread_cancel on UDP_sender failed");
        ret = pthread_cancel(UDP_receiver);
        ERROR_HELPER(ret, "pthread_cancel on UDP_receiver failed");
        ret = pthread_join(UDP_sender, NULL);
        ERROR_HELPER(ret, "pthread_join on UDP_sender failed");
        ret = pthread_join(UDP_receiver, NULL);
        ERROR_HELPER(ret, "pthread_join on UDP_receiver failed");
        pthread_attr_destroy(&attr1);
        pthread_attr_destroy(&attr2);
        logger_verbose(__func__, "successful join on udp threads");
    }

    //post quitPacket to Server
    postQuitPacket();

    //close both sockets
    ret=close(socket_desc);
    ERROR_HELPER(ret,"Something went wrong closing TCP socket");
    ret=close(socket_udp);
    ERROR_HELPER(ret,"Something went wrong closing UDP socket");

    logger_verbose(__func__, "sockets were closed successfully");

    // final cleanup
    //destroy World if created!
    if(world_created){
        WorldExtended_destroy(we);
        logger_verbose(__func__, "worldextended deleted");
    }

    //and free images
    Image_free(map_elevation);
    Image_free(map_texture);

    logger_verbose(__func__, "cleanup completed");
}

/*
 * Signal Handling function
 *
 * if SIGINT is recevied, we call cleanup and close
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
            logger_error(__func__, "Uncaught signo: %d\n", signal);
            return;
    }
}


//----------MAIN------------------------------------------------------->

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
    getIdFromServer();
    getMapElevationFromServer();
    getMapTextureFromServer();
    ret = postVehicleTexture(my_texture_for_server, my_id);
    ERROR_HELPER(ret,"Cannot post my vehicle");
    my_texture = getVehicleTexture(my_id);

    // construct the world...
    we = WorldExtended_init(map_elevation, map_texture, 0.5, 0.5, 0.5);
    //...setting global bool true
    world_created = true;

    //init my vehicle with images got from server
    vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, we->w, my_id, my_texture);
    //and adding it to my world
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

    running = true;
    ret = pthread_create(&UDP_sender, &attr1, UDPSenderThread, NULL);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_sender");
    ret = pthread_create(&UDP_receiver, NULL, UDPReceiverThread, NULL);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_receiver");

    //handling global bool for threads creation (needed to cleanup later)
    udp_threads_created = true;

    //here we can run the world and finally play
    WorldViewer_runGlobal(we->w, vehicle, &argc, argv, cleanup);

    //after exit we cleanup and close
    cleanup();
    return 0;
}
