# GRIso_game

This repo contains source code of the VideoGame project for "Operative Systems" course by [G. Grisetti](https://gitlab.com/grisetti)


## Project

Implement a distributed videogame.

### Server Side

the server operates in both TCP and UDP

#### TCP Part

- registerning a new client when it comes online
- deregistering a client when it goes offline
- sending the map, when the client requests it
- UDP part
- the server receives preiodic upates from the client in the form of
    <timestamp, translational acceleration, rotational acceleration>
  Each "epoch" it integrates the messages from the clients and sends back a state update
- the server sends to each connected client the position of the agents around him

### Client side

- the client does the following
- connects to the server (TCP)
- requests the map, and gets an ID from the server (TCP)
- receives udates on the state from the server

- periodically
- updates the viewer (provided)
- reads either keyboard or joystick

## Authors
- [Luigi Russo](https://gitlab.com/lrusso96)
- [Matteo Salvino](https://gitlab.com/MatteoSalvino)
