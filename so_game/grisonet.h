#pragma once

/*
 * TCP primitives wrapped in these two griso functions.
 *
 * BLOCKING mode for both
 */

int griso_send(int desc, char* buf, int bytes_to_send);

int griso_recv(int desc, char* buf, int bytes_to_recv);
