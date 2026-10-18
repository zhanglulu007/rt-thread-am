#include <am.h>
#include <klib.h>
#include <rtthread.h>

// Global variables to pass parameters to event handler
static rt_ubase_t *switch_from = NULL;
static rt_ubase_t *switch_to = NULL;
static bool switch_to_only = false;

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
      if (switch_from != NULL && switch_to != NULL) {
        *switch_from = (rt_ubase_t)c;
        return (Context *)(*switch_to);
      } else if (switch_to_only && switch_to != NULL) {
        return (Context *)(*switch_to);
      }
      break;
    default: 
      printf("Unhandled event ID = %d\n", e.event); 
      assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  switch_to_only = true;
  switch_to = (rt_ubase_t *)to;
  switch_from = NULL;
  
  yield();
  
  switch_to_only = false;
  switch_to = NULL;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  switch_to_only = false;
  switch_from = (rt_ubase_t *)from;
  switch_to = (rt_ubase_t *)to;
  
  yield();
  
  switch_from = NULL;
  switch_to = NULL;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  rt_hw_context_switch(from, to);
}


static void thread_wrapper(void *arg) {
  void **params = (void **)arg;
  void (*tentry)(void *) = (void (*)(void *))params[0];
  void *parameter = params[1];
  void (*texit)(void) = (void (*)(void))params[2];
  
  tentry(parameter);
  
  texit();
  
  while(1);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  uintptr_t stack_top = (uintptr_t)stack_addr;
  stack_top &= ~(sizeof(uintptr_t) - 1);
  
  stack_top -= 3 * sizeof(void *);
  void **params = (void **)stack_top;
  params[0] = tentry;
  params[1] = parameter;
  params[2] = texit;
  
  Area kstack = {
    .start = (void *)((uintptr_t)stack_addr - 0x8000), 
    .end = (void *)stack_top
  };
  
  Context *ctx = kcontext(kstack, thread_wrapper, (void *)params);

  return (rt_uint8_t *)ctx;
}
