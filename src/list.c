#include "list.h"

#define LIST_ASSERT(x) ASSERT(x)

void list_init(list_t* l) {
	l->first_element = NULL;
	l->last_element = NULL;
	l->length = 0;
}

void list_add(list_t* l, void* ve) {
  LIST_ASSERT(ve != NULL);
  element_t* e = (element_t*) ve;
  if (l->first_element == NULL) {
    l->first_element = e;
    e->prev = NULL;
  } else {
    l->last_element->next = e;
    e->prev = l->last_element;
  }
  l->last_element = e;
  e->next = NULL;
  l->length++;
  LIST_ASSERT(l->first_element != NULL);
}


void list_add_first(list_t* l, void* ve) {
  LIST_ASSERT(ve != NULL);
  element_t* e = (element_t*) ve;
  if (l->first_element == NULL) {
    l->last_element = e;
    e->next = NULL;
  } else {
    l->first_element->prev = e;
    e->next = l->first_element;
  }
  l->first_element = e;
  e->prev = NULL;
  l->length++;
  LIST_ASSERT(l->first_element != NULL);
}


void list_insert_before(list_t* l, void* ve, void* ve_before) {
  LIST_ASSERT(ve != NULL);
  LIST_ASSERT(ve_before != NULL);
  element_t* e = (element_t*) ve;
  element_t* e_before = (element_t*) ve_before;

  e->next = e_before;
  e->prev = e_before->prev;
  if (e_before->prev != NULL) {
    e_before->prev->next = e;
  } else {
    l->first_element = e;
  }
  e_before->prev = e;
  l->length++;
  LIST_ASSERT(l->first_element != NULL);
}

void list_delete(list_t* l, void* ve) {
  LIST_ASSERT(ve != NULL);
  element_t* e = (element_t*) ve;
  if (e == l->first_element) {
    // removing first element in list
    l->first_element = l->first_element->next;
  } else {
    e->prev->next = e->next;
  }

  if (e->next == NULL) { // no element after this
    l->last_element = e->prev;
  } else {
    e->next->prev = e->prev;
  }
  e->prev = NULL;
  e->next = NULL;
  l->length--;
  if (l->length == 0) {
    LIST_ASSERT(l->first_element == NULL);
  } else {
    LIST_ASSERT(l->first_element != NULL);
  }
}


void list_sort_insert(list_t* l, void* ve) {
  element_t* e = (element_t*) ve;
  if (l->first_element == NULL) {
    // First and only element
    list_add(l, e);
  } else {
    element_t* cur_e = l->first_element;
    list_sort_order e_val = list_get_order(e);
    while (cur_e != NULL) {
      if (e_val <= list_get_order(cur_e)) {
        list_insert_before(l, e, cur_e);
        break;
      }
      cur_e = cur_e->next;
    }
    if (cur_e == NULL) {
      // Found no one to put this before, so must be a lazy one
      list_add(l, e);
    }
  }
}

void list_move_last(list_t* l, void* ve) {
  LIST_ASSERT(ve != NULL);
  element_t* e = (element_t*) ve;

  if (e == l->last_element) {
    return;
  }

  // delete element, knowing it cannot be the only or the last one in list
  if (e == l->first_element) {
    // removing first element in list
    l->first_element = l->first_element->next;
  } else {
    e->prev->next = e->next;
  }
  e->next->prev = e->prev;

  // add element to end, knowing list is not empty
  l->last_element->next = e;
  e->prev = l->last_element;

  l->last_element = e;
  e->next = NULL;
}


void list_move_all(list_t* l_dst, list_t* l_src) {
  if (l_src->first_element == NULL) {
    // moving empty list, nop
    return;
  }
  if (l_dst->last_element == NULL) {
    // moving list to empty list, copy
    l_dst->first_element = l_src->first_element;
    l_dst->last_element = l_src->last_element;
    l_dst->length = l_src->length;
  } else {
    // hook dst's last together with src's first element
    l_dst->last_element->next = l_src->first_element;
    l_src->first_element->prev = l_dst->last_element;
    l_dst->last_element = l_src->last_element;
    l_dst->length += l_src->length;
  }
  l_src->length = 0;
  l_src->first_element = NULL;
  l_src->last_element = NULL;
}

void list_move_all_first(list_t* l_dst, list_t* l_src) {
  if (l_src->first_element == NULL) {
    // moving empty list, nop
    return;
  }
  if (l_dst->first_element == NULL) {
    // moving list to empty list, copy
    l_dst->first_element = l_src->first_element;
    l_dst->last_element = l_src->last_element;
    l_dst->length = l_src->length;
  } else {
    // hook src's last together with dst's first element
    l_dst->first_element->prev = l_src->last_element;
    l_src->last_element->next = l_dst->first_element;
    l_dst->first_element = l_src->first_element;
    l_dst->length += l_src->length;
  }
  l_src->length = 0;
  l_src->first_element = NULL;
  l_src->last_element = NULL;
}

