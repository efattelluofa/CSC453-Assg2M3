/*
 * File: global.h
 * Author: S. Debray
 */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define FALSE    0
#define TRUE     ~(FALSE)

#define WARNING(msg)  fprintf(stderr, "%s\n", msg);
#define ERROR(msg)  { fprintf(stderr, "%s\n", msg); exit(-1); }

#endif
