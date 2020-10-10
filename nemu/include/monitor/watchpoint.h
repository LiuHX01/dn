#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	char exprname[32];
	unsigned int value;
	struct watchpoint *next;
	/* TODO: Add more members if necessary */


} WP;

WP* new_wp();
void free_wp(WP *);
void delete_wp(int );
void print_wp();

#endif
