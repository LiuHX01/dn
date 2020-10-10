#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp()
{
	WP *fr,*p;
	fr = free_;
	free_ = free_->next;
	fr->next = NULL;
	p = head;
	if (p == NULL) {
		head = fr;
		p = head;
	}
	else {
		while (p->next!=NULL)
			p=p->next;
		p->next = fr;
	}
	return fr;
}

void free_wp(WP *wp) {
	WP *p, *q;
	p = free_;
	if (p == NULL) {
		free_ = wp;
		p = free_; // p == free_ == wp
	}
	else {
		while (p->next != NULL)
			p = p->next;
		p->next = wp;
	}

	q = head;
	if (q == NULL) 
		assert(0);
	if (head->NO == wp->NO)  // wp is the first node
		head = head->next;
	else {
		while (q->next != NULL && q->next->NO != wp->NO)
			q = q->next;
		if (q->next == NULL && q->NO == wp->NO)
			printf("wrong!\n");
		else if (q->next->NO == wp->NO)
			q->next = q->next->next;
		else 
			assert(0);
	}

	wp->next = NULL;
	wp->value = 0;
	wp->exprname[0] = '\0';
}

void delete_wp(int no) {
	WP *q;
	q = &wp_pool[no];
	free_wp(q);
	printf("delete watchpoint Done.\n");
}

void print_wp() {
	WP *p;
	p = head;
	while (p != NULL) {
		printf("watchpoint %d: %s = %d\n", p->NO, p->exprname, p->value);
		p = p->next;
	}
}
