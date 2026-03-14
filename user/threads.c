#include "kernel/types.h"
#include "user/setjmp.h"
#include "user/threads.h"
#include "user/user.h"
#define NULL 0

static struct thread* current_thread = NULL;
static int id = 1;

//the below 2 jmp buffer will be used for main function and thread context switching
static jmp_buf env_st;
// static jmp_buf env_tmp;

struct thread *get_current_thread() {
    return current_thread;
}

struct thread *thread_create(void (*f)(void *), void *arg){
    struct thread *t = (struct thread*) malloc(sizeof(struct thread));
    unsigned long new_stack_p;
    unsigned long new_stack;
    
    new_stack = (unsigned long) malloc(sizeof(unsigned long)*0x100);
    new_stack_p = new_stack + 0x100*8 - 0x2*8;
    
    t->fp = f;
    t->arg = arg;
    t->ID  = id;
    t->buf_set = 0;
    t->stack = (void*) new_stack;  //points to the beginning of allocated stack memory for the thread.
    t->stack_p = (void*) new_stack_p;  //points to the current execution part of the thread.
    id++;
    // part 2
    new_stack = (unsigned long) malloc(sizeof(unsigned long)*0x100);
    new_stack_p = new_stack + 0x100*8 - 0x2*8;
    t->sig_stack = (void*) new_stack; //points to the beginning of allocated stack memory for the thread.
    t->sig_stack_p = (void*) new_stack_p; //points to the current execution part of the thread.
    t->sig_handler[0] = NULL_FUNC;
    t->sig_handler[1] = NULL_FUNC;
    t->signo = -1;
    t->handler_buf_set = 0;
    
    t->suspended = 0;      
    t->join_blocked = 0;   
    
    t->exited = 0;      
    t->waiter_list = NULL; 
    t->next_waiter = NULL; 
    return t;
}

void thread_add_runqueue(struct thread *t){
    if(current_thread == NULL){
        //TODO
        current_thread = t;
        current_thread->next = current_thread;
        current_thread->previous = current_thread;
    }
    else{
        //TODO
        current_thread->previous->next = t;
        t->previous = current_thread->previous;
        t->next = current_thread;
        current_thread->previous = t;
        // part 2 TODO
        t->sig_handler[0]=current_thread->sig_handler[0];
        t->sig_handler[1]=current_thread->sig_handler[1];
    }
}

void thread_yield(void){
    // Part 2 TODO
    if(current_thread->signo != -1){
        if(setjmp(current_thread->handler_env) == 0){
            schedule();
            dispatch();
        }
        return;
    }

    //TODO
    if(setjmp(current_thread->env) == 0){
        schedule();
        dispatch();
    }
}

void dispatch(void){
    // Part 2 TODO 
    if(current_thread->signo!=-1){
        if(current_thread->sig_handler[current_thread->signo]==NULL_FUNC){
            thread_exit();
        }
        if(current_thread->handler_buf_set==0){
            current_thread->handler_buf_set=1;
            if(setjmp(current_thread->handler_env)==0){
                current_thread->handler_env[0].sp=(unsigned long)current_thread->sig_stack_p;
                longjmp(current_thread->handler_env,1);
            }
            current_thread->sig_handler[current_thread->signo](current_thread->signo);
            current_thread->handler_buf_set=0;
            current_thread->signo=-1;
        }else{
            longjmp(current_thread->handler_env,1);
        }
    }

    //TO DO   
    if(current_thread->buf_set==0){
        current_thread->buf_set=1;
        // So that the function current_thread->fp won't be call on the stack. Instead, it will be call on current_thread->stack_p.
        if(setjmp(current_thread->env)==0){
            current_thread->env[0].sp=(unsigned long)current_thread->stack_p;
            longjmp(current_thread->env,1);
        }
        current_thread->fp(current_thread->arg);
        thread_exit();
    }else{
        longjmp(current_thread->env,1);
    }
}

//schedule will follow the rule of FIFO
void schedule(void){
    current_thread = current_thread->next;
    
    //TODO
    while(current_thread->suspended || current_thread->join_blocked) {
        current_thread = current_thread->next;
    };
}

void thread_exit(void) {
    struct thread *w = current_thread->waiter_list;
    // struct thread *ptr = current_thread;
    //TODO (Part2)
    struct thread *nxt;
    while(w != NULL){
        nxt = w->next_waiter;
        w->join_blocked = 0;
        w = nxt;
    }
    current_thread->exited = 1;

    if (current_thread->next != current_thread) {
       //TODO
        struct thread *nxt=current_thread->next;
        current_thread->previous->next=nxt;
        nxt->previous=current_thread->previous;
        free(current_thread->stack);
        free(current_thread->sig_stack);
        free(current_thread);
        current_thread=nxt;
        dispatch();
    } else {
        //TODO
        free(current_thread->stack);
        free(current_thread->sig_stack);
        free(current_thread);
        longjmp(env_st, 1);
    }
}

void thread_start_threading(void){
    //TODO
    if(setjmp(env_st)==0){
        dispatch();
    }
}

void thread_register_handler(int signo, void (*handler)(int)){
    current_thread->sig_handler[signo] = handler;
}

void thread_kill(struct thread *t, int signo){
    //TODO
    t->signo = signo;
}

void thread_suspend(struct thread *t) {
    //TODO
    t->suspended = 1;
    if(t == current_thread){
        thread_yield();
    }
}

void thread_resume(struct thread *t) {
    //TODO
    t->suspended = 0;
}

void thread_join(struct thread *t) {
    //TODO
    if(t->exited == 1){
        return;
    }
    current_thread->next_waiter = t->waiter_list;
    t->waiter_list = current_thread;
    current_thread->join_blocked = 1;
    thread_yield();
}
