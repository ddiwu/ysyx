#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  bool unuse;
  char expr[40];
  int new_value;
  int old_value;
} WP;

extern WP wp_pool[NR_WP];//声明全局变量

void init_wp_pool();
WP* new_wp();
void free_wp(WP *wp);
#endif