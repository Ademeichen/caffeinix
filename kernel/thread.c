/*
 * @Author: TroyMitchell
 * @Date: 2024-05-11
 * @LastEditors: TroyMitchell
 * @LastEditTime: 2024-05-16
 * @FilePath: /caffeinix/kernel/thread.c
 * @Description: 
 * Words are cheap so I do.
 * Copyright (c) 2024 by TroyMitchell, All Rights Reserved. 
 */
#include <thread.h>
#include <palloc.h>
#include <mystring.h>
#include <uart.h>
#include <printf.h>

struct list all_thread;

struct thread thread[NTHREAD];

static int next_tid = 1;
static struct spinlock tid_lock;

static int tid_alloc(void)
{
        int tid;
        spinlock_acquire(&tid_lock);
        tid = next_tid++;
        spinlock_release(&tid_lock);
        return tid;
}

void thread_setup(void)
{
        thread_t t;
        spinlock_init(&tid_lock, "tid_lock");
        for(t = thread; t <= &thread[NTHREAD - 1]; t++) {
                spinlock_init(&t->lock, "thread");
                t->kstack = KSTACK((int)(t - thread));;
                t->state = NUSED;
        }
}

thread_t thread_alloc(process_t p)
{
        thread_t t;
        for(t = thread; t <= &thread[NTHREAD - 1]; t++) {
                spinlock_acquire(&t->lock);
                if(t->state == NUSED) {
                        t->home = p;
                        goto found;
                }
                spinlock_release(&t->lock);
        }
        return 0;
found:
        t->state = NREADY;

        t->trapframe = (trapframe_t)palloc();
        if(!t->trapframe) {
                goto r1;
        }

        memset(t->trapframe, 0, PGSIZE);

        t->tid = tid_alloc();

        return t;
        
r1:
        spinlock_release(&t->lock);
        return 0;
}

void thread_free(thread_t t)
{
        t->state = NUSED;
        if(t->trapframe)
                pfree(t->trapframe);
        t->trapframe = 0;
}
