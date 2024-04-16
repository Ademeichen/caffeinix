#ifndef __CAFFEINIX_KERNEL_THREAD_H
#define __CAFFEINIX_KERNEL_THREAD_H

#include <typedefs.h>
#include <list.h>
#include <spinlock.h>

typedef struct context {
        uint64 ra;
        uint64 sp;

        /* Callee saved */
        uint64 s0;
        uint64 s1;
        uint64 s2;
        uint64 s3;
        uint64 s4;
        uint64 s5;
        uint64 s6;
        uint64 s7;
        uint64 s8;
        uint64 s9;
        uint64 s10;
        uint64 s11;
}*context_t;

typedef enum thread_state {
        READY,
        ACTIVE,
}thread_state_t;

typedef struct thread {
        const char* name;
        struct spinlock lock;
        thread_state_t state;
        struct context context;
        struct list all_tag;
}*thread_t;

typedef struct cpu {
        struct context context;
        thread_t thread;
        /* Nesting Depth */
        uint8 lock_nest_depth;
        /* Is the interrupt enabled before locking */
        uint8 before_lock;
}*cpu_t;

typedef void (*thread_func_t)(void*);

void thread_init(void);
uint8 cpuid(void);
cpu_t cur_cpu(void);
thread_t cur_thread();

void scheduler(void);
void yield(void);

void thread_test(void);
#endif