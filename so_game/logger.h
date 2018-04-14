#pragma once

/*
 * These functions wrap fprintf (and variants).
 * Added to make easier to debug main program.
 *
 * Note: By changing VERBOSE and ERROR values, we can print what we need
 */

void logger_verbose(const char* who, const char *fmt, ...);
void logger_error(const char* who, const char *fmt, ...);

