#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char exprname[32];
	int value;
	/* TODO: Add more members if necessary */


} WP;

WP* new_wp();
void free_wp(WP *wp);
void delete_wp(int no);
void print_wp();

#endif
