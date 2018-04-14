#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>

#include "grisonet.h"
#include "image.h"
#include "logger.h"
#include "random_id.h"
#include "so_game_protocol.h"
#include "surface.h"
#include "utils.h"
#include "vehicle.h"
#include "world.h"
#include "world_viewer.h"
#include "world_server.h"


//----------GLOBAL vars------------------------------------------------>

bool is_up = false;

//server descriptors (tcp & ud)
int tcp_server_desc, udp_server_desc;
struct sockaddr_in tcp_server_addr, udp_server_addr;

int tcp_sockaddr_len, tcp_server_port;

pthread_t UDP_receiver_thread, UDP_sender_thread;
pthread_attr_t attr1, attr2;


//the world
WorldServer* ws;

//images of the world
Image* surface_elevation;
Image* surface_texture;

//progressive id getter
RandomId* randomId;


typedef struct{
  struct sockaddr_in* client_addr;      //field of client item
  int client_socket;
} TCPArgs;




int postMapElevationToClient(ClientItem* ci){

  size_t msg_len;
  char img_packet_buffer[BUFFER_SIZE];
  char buf_rcv[BUFFER_SIZE];
  int ip_len = sizeof(IdPacket);

  logger_verbose(__func__, "[Server] Waiting elevation surface request from client #%d.\n", ci->id);

  //Receive map elevation request from client
  msg_len = griso_recv(ci->socket, buf_rcv, ip_len);
  ERROR_HELPER(msg_len, "Can't receive map elevation request from client.\n");

  logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

  ImagePacket* incoming_packet = (ImagePacket*) buf_rcv;

  logger_verbose(__func__, "incoming_packet with :\ntype\t%d\nsize\t%d\n", incoming_packet->header.type, incoming_packet->header.size);

  if(incoming_packet->header.type == GetElevation)
    logger_verbose(__func__, "Header Check passed.\n");
  else{
    logger_verbose(__func__, "Header Check failed.\n");
    return 0;
  }

  //Packet_free(incoming_packet);

  PacketHeader header_elevation_surface;
  ImagePacket* img_packet_elevation_surface = (ImagePacket*) malloc(sizeof(ImagePacket));

  logger_verbose(__func__, "Sending elevation surface to client #%d.\n", ci->id);

  header_elevation_surface.type = PostElevation;

  img_packet_elevation_surface->header = header_elevation_surface;
  img_packet_elevation_surface->id = 0;
  img_packet_elevation_surface->image = surface_elevation;

  logger_verbose(__func__, "img_packet_elevation_surface with :\ntype\t%d\nsize\t%d\n", img_packet_elevation_surface->header.type, img_packet_elevation_surface->header.size);
  logger_verbose(__func__, "Serialize img_packet_elevation_surface.\n");

  int img_packet_buffer_size = Packet_serialize(img_packet_buffer, &img_packet_elevation_surface->header);

  logger_verbose(__func__, "Bytes written in the buffer : %d.\n", img_packet_buffer_size);

  //Send serialize img_elevation_surface to client
  msg_len = griso_send(ci->socket, img_packet_buffer, img_packet_buffer_size);
  ERROR_HELPER(msg_len, "Can't send serialize elevation surface package to client.\n");

  logger_verbose(__func__, "Bytes sent : %zu bytes.\n", msg_len);

  free(img_packet_elevation_surface);

  return 1;
}

int postMapTextureToClient(ClientItem* ci){

  size_t msg_len;
  char img_packet_buffer[BUFFER_SIZE];
  char buf_rcv[BUFFER_SIZE];
  size_t ip_len = sizeof(IdPacket);

  logger_verbose(__func__, "Waiting texture surface request from client #%d.\n", ci->id);

  //Receive texture surface request from client
  msg_len = griso_recv(ci->socket, buf_rcv, ip_len);
  ERROR_HELPER(msg_len, "Can't receive texture surface request from client.\n");

  logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

  ImagePacket* incoming_packet = (ImagePacket*) buf_rcv;
  logger_verbose(__func__, "incoming_packet with :\ntype\t%d\nsize\t%d\n.",incoming_packet->header.type, incoming_packet->header.size);

  if(incoming_packet->header.type == GetTexture)
    logger_verbose(__func__, "Header Check passed.\n");
  else{
    logger_verbose(__func__, "Header Check failed.\n");
    return 0;
  }

  PacketHeader header_texture_surface;
  ImagePacket* img_packet_texture_surface = (ImagePacket*) malloc(sizeof(ImagePacket));
  //size_t ph_len = sizeof(PacketHeader);

  logger_verbose(__func__, "Sending texture surface to client #%d.\n", ci->id);

  header_texture_surface.type = PostTexture;

  img_packet_texture_surface->header = header_texture_surface;
  img_packet_texture_surface->id = 0;
  img_packet_texture_surface->image = surface_texture;

  logger_verbose(__func__, "img_packet_texture_surface with :\ntype\t%d\nsize\t%d\n", img_packet_texture_surface->header.type, img_packet_texture_surface->header.size);

  logger_verbose(__func__, "Serialize img_packet_texture_surface.\n");

  int img_packet_buffer_size = Packet_serialize(img_packet_buffer, &img_packet_texture_surface->header);

  logger_verbose(__func__, "Bytes written in the buffer : %d.\n", img_packet_buffer_size);

  //Send serialize img_texture_surface to client
  msg_len = griso_send(ci->socket, img_packet_buffer, img_packet_buffer_size);
  ERROR_HELPER(msg_len, "Can't send serialized texture surface package to client.\n");

  logger_verbose(__func__, "[Server - Data] Bytes sent : %zu bytes.\n", msg_len);

  free(img_packet_texture_surface);

  return 1;
}


Image* getVehicleTextureFromClient(ClientItem* ci){

  size_t msg_len;
  char buf_rcv[BUFFER_SIZE];
  int ip_len = sizeof(ImagePacket);
  int size;

  logger_verbose(__func__, "Receiving player texture from client #%d.\n", ci->id);

  //Receive player texture header from client
  msg_len = griso_recv(ci->socket, buf_rcv, ip_len);
  ERROR_HELPER(msg_len, "Can't receive player texture from client.\n");

  logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

  ImagePacket* incoming_packet = (ImagePacket*) buf_rcv;

  logger_verbose(__func__, "incoming_packet with : \ntype\t%d\nsize\t%d\n.",incoming_packet->header.type, incoming_packet->header.size);

  size = incoming_packet->header.size - ip_len;


  if(incoming_packet->header.type == PostTexture && incoming_packet->id > 0){

    logger_verbose(__func__, "Header Check Passed.\n");

    //Receive player texture data from client
    msg_len = griso_recv(ci->socket, buf_rcv+ip_len, size);
    ERROR_HELPER(msg_len, "Can't receive player texture date from client.\n");

    logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

    ImagePacket* deserialized_packet = (ImagePacket*) Packet_deserialize(buf_rcv, msg_len+ip_len);

    logger_verbose(__func__, "deserialized_packet with : \ntype\t%d\nsize\t%d\nid\t%d\n",
		   deserialized_packet->header.type, deserialized_packet->header.size,
		   deserialized_packet->id);

    Image* player_texture = deserialized_packet->image;

    //Build client vehicle and add to world server
    Vehicle* vehicle = (Vehicle*) malloc(sizeof(Vehicle));

    Vehicle_init(vehicle, ws->w, ci->id, player_texture);
    WorldServer_addClient(ws, vehicle, ci);

    logger_verbose(__func__, "Vehicle added to world server.\n");

    //Packet_free(&incoming_packet->header);
    free(deserialized_packet);

    return player_texture;

  }else{
    logger_verbose(__func__, "Header Check Failed.\n");
    return NULL;
  }

}


int postVehicleTextureToClient(ClientItem* ci, int id, Image* player_texture){

  int size;
  size_t msg_len;
  char img_packet_player_texture[BUFFER_SIZE];
  char buf_rcv[BUFFER_SIZE];
  int ip_len = sizeof(IdPacket);

  logger_verbose(__func__, "Waiting player texture request from client #%d.\n", ci->id);

  //Receive player texture request from client
  msg_len = griso_recv(ci->socket, buf_rcv, ip_len);
  ERROR_HELPER(msg_len, "Can't receive player texture request from client.\n");

  logger_verbose(__func__, "Bytes received : %zu bytes.\n", msg_len);

  ImagePacket* incoming_packet = (ImagePacket*) buf_rcv;

  logger_verbose(__func__, "incoming_packet with : \ntype\t%d\nsize\t%d\n.", incoming_packet->header.type, incoming_packet->header.size);

  if(incoming_packet->header.type == GetTexture)
    logger_verbose(__func__, "Header Check Passed.\n");
  else{
    logger_verbose(__func__, "Header Check Failed.\n");
    return 0;
  }

  //Send player texture packet to client
  PacketHeader player_texture_header;

  player_texture_header.type = PostTexture;

  ImagePacket* img_packet_pt = (ImagePacket*) malloc(sizeof(ImagePacket));

  img_packet_pt->header = player_texture_header;
  img_packet_pt->id = id;
  img_packet_pt->image = player_texture;

  size = Packet_serialize(img_packet_player_texture, &(img_packet_pt->header));

  msg_len = griso_send(ci->socket, img_packet_player_texture, size);
  ERROR_HELPER(msg_len, "Can't send player texture packet to client.\n");

  logger_verbose(__func__, "Bytes sent : %zu bytes.\n", msg_len);

  free(img_packet_pt);

  return 1;
}


int getIdFromClient(ClientItem* ci){

  char id_packet_buffer[BUFFER_SIZE];
  size_t pi_len = sizeof(IdPacket);
  size_t msg_len;

  logger_verbose(__func__, "Receiving an id request from client.\n");

  //Receive an id request from client
  msg_len = griso_recv(ci->socket, id_packet_buffer, pi_len);
  ERROR_HELPER(msg_len, "griso_recv failed.\n");

  logger_verbose(__func__, "Bytes received : %d bytes.\n", msg_len);

  IdPacket* id_packet = (IdPacket*) Packet_deserialize(id_packet_buffer, msg_len);

  logger_verbose(__func__, "id_packet with : \ntype\t%d\nsize\t%d\nid\t%d\n",
		 id_packet->header.type, id_packet->header.size, id_packet->id);

    int id = 0;

  //Check if client satisfied game protocol
  if(id_packet->header.type == GetId && id_packet->id == -1){
    logger_verbose(__func__, "Header Check Passed.\n");
    logger_verbose(__func__, "A client required an id.\n");

    id =  RandomId_getNext(randomId);
  }

    else{
        logger_verbose(__func__,"Header Check Failed.\n");
    }

    free(id_packet);

  return id;
}

void postIdToClient(ClientItem* ci){

  char id_packet_buffer[BUFFER_SIZE];
  size_t msg_len;

  PacketHeader ph;
  ph.type = GetId;

  IdPacket* id_packet = (IdPacket*) malloc(sizeof(IdPacket));
  id_packet->header = ph;
  id_packet->id = ci->id;

  int bytes_to_send = Packet_serialize(id_packet_buffer, &id_packet->header);

  logger_verbose(__func__, "[Server] Bytes written in the buffer %d.\n", bytes_to_send);

  //Send id packet to client
  msg_len = griso_send(ci->socket, id_packet_buffer, bytes_to_send);
  ERROR_HELPER(msg_len, "griso_send failed.\n");

  logger_verbose(__func__, "[Server - Data] Bytes sent : %d bytes.\n", msg_len);
  logger_verbose(__func__, "[Server] Client #%d has joined the game.\n",id_packet->id);

  free(id_packet);
}


void* TCPWork(void* params){

    int client_id, elevation_surface_flag, texture_surface_flag, post_pt_flag;
    Image* player_texture;

    ClientItem* ci = (ClientItem*) params;

    client_id = getIdFromClient(ci);

    //Client satisfied game protocol
    if(client_id){
        ci->id = client_id;
        postIdToClient(ci);

        elevation_surface_flag = postMapElevationToClient(ci);

        if(elevation_surface_flag){
            texture_surface_flag = postMapTextureToClient(ci);
            if(texture_surface_flag){

                player_texture = getVehicleTextureFromClient(ci);
                if(player_texture->rows && player_texture->cols){
                    post_pt_flag = postVehicleTextureToClient(ci, client_id, player_texture);
                    if(post_pt_flag){
                        //Client is ready to play
                        logger_verbose(__func__, "Listening update packets from client.\n");

                        /*fillme recv a packet
                         *
                         * if quit packet, detach client and close
                         * if gettexture packet, post texture
                         *
                         */

                        char buf_rcv[BUFFER_SIZE];
                        int ph_len = sizeof(PacketHeader);

                        while(true){

                            size_t msg_len = griso_recv(ci->socket, buf_rcv, ph_len);
                            ERROR_HELPER(msg_len, "Can't receive header packeth\n");

                            logger_verbose(__func__, "Bytes received : %d bytes.\n", msg_len);

                            PacketHeader* incoming_pckt=(PacketHeader*) buf_rcv;
                            int size = incoming_pckt->size-ph_len;

                            //Receive rest of packet
                            if(incoming_pckt->type == Quit){
                                msg_len = griso_recv(ci->socket, buf_rcv+ph_len, size);
                                ERROR_HELPER(msg_len, "Can't receive end of packet.\n");

                                IdPacket* deserialized_packet = (IdPacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
                                int id = deserialized_packet->id;
                                logger_verbose(__func__, "Bytes received : %d bytes.\n",msg_len);
                                logger_verbose(__func__, "Quit packet with :\n\tid = %d\n", id);

                                //free not needed
                                //Packet_free(&(deserialized_packet->header));

                                pthread_mutex_lock(&(ws->mutex));
                                WorldServer_detachClient(ws, id);
                                pthread_mutex_unlock(&(ws->mutex));
                                free(deserialized_packet);


                                char buf_send[BUFFER_SIZE];

                                PacketHeader ph;
                                ph.type = Quit;

                                IdPacket* id_packet = (IdPacket*) malloc(sizeof(IdPacket));
                                id_packet->header = ph;
                                id_packet->id = id;

                                size = Packet_serialize(buf_send, &(id_packet->header));

                                ListItem* item = ws->clients.first;
                                while(item){
                                    ClientItem* ci=(ClientItem*)item;
                                    item = item->next;

                                    if(ci->id == id)
                                        continue;

                                    int sent = sendto(udp_server_desc, buf_send, size, 0, (struct sockaddr *) &(ci->clientStorage), ci->addr_size);
                                    logger_verbose(__func__, "sent %d bytes of quit of client %d to client %d", sent, id, ci->id);
                                }

                                free(id_packet);


                                return NULL;

                            }

                            else if(incoming_pckt->type == GetTexture){
                                msg_len = griso_recv(ci->socket, buf_rcv+ph_len, size);
                                ERROR_HELPER(msg_len, "Can't receive end of packet.\n");

                                ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
                                int id = deserialized_packet->id;
                                Image* im = NULL;

                                World* w = ws->w;
                                ListItem* item=w->vehicles.first;
                                while(item){
                                    Vehicle* v=(Vehicle*)item;
                                    if(v->id == id){
                                        im = v->texture;
                                        break;
                                    }
                                    item = item->next;
                                }

                                free(deserialized_packet);


    /////////////// wrap this

                                //Send player texture packet to client
                                char img_packet_player_texture[BUFFER_SIZE];
                                PacketHeader player_texture_header;

                                player_texture_header.type = PostTexture;

                                ImagePacket* img_packet_pt = (ImagePacket*) malloc(sizeof(ImagePacket));

                                img_packet_pt->header = player_texture_header;
                                img_packet_pt->id = id;
                                img_packet_pt->image = im;

                                size = Packet_serialize(img_packet_player_texture, &(img_packet_pt->header));

                                size_t msg_len = griso_send(ci->socket, img_packet_player_texture, size);
                                ERROR_HELPER(msg_len, "Can't send player texture packet to client.\n");

                                logger_verbose(__func__, "Bytes sent : %zu bytes.\n", msg_len);

                                free(img_packet_pt);

                            }

                        }

                    }
                }
            }
        }
    }
  printf("Done.\n");
  return NULL;
}

void createTCPConnection(void){

  int ret;

  tcp_sockaddr_len = sizeof(struct sockaddr_in);
  //tcpArgs->client_addr = calloc(1, sizeof(struct sockaddr_in));

  //Initialize server socket for listening
  tcp_server_desc = socket(AF_INET, SOCK_STREAM, 0);
  ERROR_HELPER(tcp_server_desc, "Can't create a server socket.\n");

  tcp_server_addr.sin_addr.s_addr = INADDR_ANY;
  tcp_server_addr.sin_family = AF_INET;
  tcp_server_addr.sin_port = htons(tcp_server_port);

  //We enable SO_REUSEADDR to restart server after a crash
  int reuseaddr_opt = 1;
  ret = setsockopt(tcp_server_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
  ERROR_HELPER(ret, "Can't set reuseaddr option.\n");

  //Now, we bind address to socket
  ret = bind(tcp_server_desc, (struct sockaddr*) &(tcp_server_addr), tcp_sockaddr_len);
  ERROR_HELPER(ret, "Can't bind address to socket.\n");

  //Start listening on server desc
  ret = listen(tcp_server_desc, MAX_CONNECTIONS);
  ERROR_HELPER(ret, "Can't listen on server desc.\n");

  is_up = true;

  logger_verbose(__func__, "[Server] TCP socket successfully created.\n");
}


void createUDPConnection(void){

  uint16_t udp_port_number = htons((uint16_t)SERVER_UDP_PORT); // we use network byte order
  udp_server_desc = socket(AF_INET, SOCK_DGRAM, 0);
  ERROR_HELPER(udp_server_desc, "Can't create an UDP socket");

  udp_server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
  udp_server_addr.sin_family = AF_INET;
  udp_server_addr.sin_port = udp_port_number;

  memset(udp_server_addr.sin_zero, '\0', sizeof udp_server_addr.sin_zero);

   /*Bind socket with address struct*/
  bind(udp_server_desc, (struct sockaddr *) &(udp_server_addr), sizeof(udp_server_addr));

  logger_verbose(__func__, "UDP socket successfully created");
}



void* UDPSenderThread(void* args){

    //enable asynchronous pthread_cancel from cleanup
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    unsigned long epoch = 1;

    char buf_send[BUFFER_SIZE];

    PacketHeader ph;
    ph.type = WorldUpdate;

    WorldUpdatePacket* wup = (WorldUpdatePacket*) malloc(sizeof(WorldUpdatePacket));
    wup->header = ph;

    while(is_up){
        usleep(500*1000);

        if(ws->clients.size==0){
            continue;
        }


        pthread_mutex_lock(&(ws->mutex));

        World_update(ws->w);

        //set num_vehicles (world size can't change now!)
        wup->num_vehicles = ws->clients.size;

        wup->epoch = epoch++;

        wup->updates = (ClientUpdate*) malloc(sizeof(ClientUpdate)*wup->num_vehicles);

        ListItem* item = ws->clients.first;
        int i = 0;
        logger_verbose(__func__,"start my loop");
        while(item){
            ClientItem* ci=(ClientItem*)item;
            item = item->next;

            ClientUpdate* cu = &(wup->updates[i++]);
            cu->id = ci->id;
            WorldServer_getClientInfo(ws, cu->id, &(cu->x), &(cu->y), &(cu->theta));
            logger_verbose(__func__,"info: %d, %f, %f, %f", cu->id, cu->x, cu->y, cu->theta);
        }

        logger_verbose(__func__,"end my loop");

        int size = Packet_serialize(buf_send, &wup->header);
        logger_verbose(__func__, "serialized packet wup of %d bytes", size);

        pthread_mutex_unlock(&(ws->mutex));

        //send update to everyone now
        item = ws->clients.first;
        while(item){
            ClientItem* ci=(ClientItem*)item;
            item = item->next;

            int sent = sendto(udp_server_desc, buf_send, size, 0, (struct sockaddr *) &(ci->clientStorage), ci->addr_size);
            logger_verbose(__func__, "sent %d bytes of world update to client %d", sent, ci->id);
        }


    }

    Packet_free(&(wup->header));

    return NULL;
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


    int udpSocket = udp_server_desc;

    struct sockaddr_storage serverStorage;
    socklen_t addr_size = sizeof(serverStorage);


    while(is_up){

        usleep(100*1000);

        int nBytes = recvfrom(udpSocket, buf_recv, BUFFER_SIZE, 0, (struct sockaddr *)&serverStorage, &addr_size);
        logger_verbose(__func__, "received %d bytes", nBytes);

        VehicleUpdatePacket* v_packet = (VehicleUpdatePacket*) Packet_deserialize(buf_recv, nBytes);

        ListItem* item = ws->clients.first;
         while(item){
            ClientItem* ci=(ClientItem*)item;
            item = item->next;
            if(ci->id == v_packet->id){
                ci->clientStorage = serverStorage;
                ci->addr_size = addr_size;
                break;
            }
        }


/*
        int nn = sendto(udpSocket, buf_recv, nBytes, 0, (struct sockaddr *)&serverStorage, addr_size);
        logger_verbose(__func__, "sent back %d bytes", nn);

*/


        pthread_mutex_lock(&(ws->mutex));
        WorldServer_updateClient(ws, v_packet->id, v_packet->x, v_packet->y, v_packet->theta);


        //now update clientitem if needed


        pthread_mutex_unlock(&(ws->mutex));
        logger_verbose(__func__, "packet received:\n\ttype = %d\n\tsize = %d\n\tid = %d\n\tx = %fn\ty = %fn\ttheta = %f",
        v_packet->header.type, v_packet->header.size, v_packet->id, v_packet->x, v_packet->y, v_packet->theta);

        free(v_packet);
    }
    return NULL;
}


//TODO handle multi clients
void mainLoop(void){

    int ret;

     //,  UDP_sender_thread, UDP_receiver_thread, TCP_connection_check;

    //init udpreceiver attr;
    pthread_attr_init(&attr1);

    ret = pthread_create(&UDP_receiver_thread, &attr1, UDPReceiverThread, NULL);
    ERROR_HELPER(ret, "Can't create UDP receiver thread.\n");

    //init udpreceiver attr;
    pthread_attr_init(&attr2);

    ret = pthread_create(&UDP_sender_thread, &attr1, UDPSenderThread, NULL);
    ERROR_HELPER(ret, "Can't create UDP receiver thread.\n");


    while(is_up){

        ClientItem* ci = (ClientItem*) malloc(sizeof(ClientItem));
        struct sockaddr_in alias = {1};
        ci->user_addr = alias;
        ci->socket = accept(tcp_server_desc, (struct sockaddr*) &(ci->user_addr), (socklen_t*)&(tcp_sockaddr_len));
        //ERROR_HELPER(ci->socket, "Can't accept incoming connection.\n");
        if(ci->socket < 0){
            free(ci);
            return;
        }

        logger_verbose(__func__, "Incoming connection accepted.\n");

        pthread_t TCP_thread;
        ret = pthread_create(&TCP_thread, NULL, TCPWork, (void*)ci);
        ERROR_HELPER(ret, "Can't create TCP_thread.\n");
    }
}

//TODO handle N users connected!
void cleanup(void){
    int ret;

    logger_verbose(__func__, "Joining UDP receiver thread");
    ret = pthread_cancel(UDP_receiver_thread);
    ERROR_HELPER(ret, "pthread_cancel to UDP_receiver failed");
    ret = pthread_join(UDP_receiver_thread, NULL);
    ret = pthread_cancel(UDP_sender_thread);
    ERROR_HELPER(ret, "pthread_cancel to UDP_sender failed");
    ret = pthread_join(UDP_sender_thread, NULL);

    pthread_attr_destroy(&attr1);
    pthread_attr_destroy(&attr2);

    logger_verbose(__func__, "successful join on threads");

    /*
     * todo:
     *
     * add join for all threads spawned (iterate over clientsitems)
     *
     * join udp receiver too
     */


    ret = close(tcp_server_desc);
    ERROR_HELPER(ret, "Can't close server tcp socket.\n");
    logger_verbose(__func__, "TCP server desc closed");
    ret = close(udp_server_desc);
    ERROR_HELPER(ret, "Can't close server udp socket.\n");
    logger_verbose(__func__, "UDP server desc closed");

    //clean up
    Image_free(surface_elevation);
    Image_free(surface_texture);
    logger_verbose(__func__, "Images free'd");

    WorldServer_destroy(ws);
    logger_verbose(__func__, "World Server destroyed");

    RandomId_destroy(randomId);
    logger_verbose(__func__, "all resources were free'd successfully");
}

void signalHandler(int signal){
  switch (signal) {
  case SIGHUP:
    break;
  case SIGINT:
    cleanup();
    break;
  default:
    fprintf(stderr, "Uncaught signal: %d\n", signal);
    return;
  }
}



int main(int argc, char **argv) {
  if (argc<3) {
    printf("usage: %s <elevation_image> <texture_image>\n", argv[0]);
    exit(-1);
  }

   randomId = RandomId_init();

  char* elevation_filename=argv[1];
  char* texture_filename=argv[2];

  /*
  char* vehicle_texture_filename="./images/arrow-right.ppm";
  printf("loading elevation image from %s ... ", elevation_filename);
  */

  // load the images
  surface_elevation = Image_load(elevation_filename);
  if (surface_elevation) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }

  printf("loading texture image from %s ... ", texture_filename);
  surface_texture = Image_load(texture_filename);
  if (surface_texture) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }



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

  struct sockaddr_in tcp_server = {0};
  struct sockaddr_in udp_server = {0};

  //Construct the world
  ws = WorldServer_init(surface_elevation, surface_texture, 0.5, 0.5, 0.5);
  logger_verbose(__func__, "World Server initialized.\n");


  tcp_server_addr = tcp_server;
  tcp_server_port = SERVER_TCP_PORT;



  udp_server_addr = udp_server;

  logger_verbose(__func__, "Creating TCP connection .\n");

  createTCPConnection();

  logger_verbose(__func__, "Creating UDP connection.\n");

  createUDPConnection();

  //spawn tcp and udp thread
  mainLoop();

  //can arrive here from cleanup only!
  logger_verbose(__func__, "See you");
  return 0;
}
