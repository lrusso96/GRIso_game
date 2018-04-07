#pragma once

int griso_send(int desc, char* buf, int bytes_to_send);

int griso_recv(int desc, char* buf, int bytes_to_recv);
