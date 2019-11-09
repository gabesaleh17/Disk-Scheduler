#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <map>
#include <queue>
#include <deque>
#include <ucontext.h>
#include <iterator>
#include "thread.h"
#include "interrupt.h"

using namespace std;

struct Thread
{
	int thread_id;
	char* stk;
	ucontext_t*  ucontext_pointer;
	bool done;

};

struct Lock
{
	Thread* lock_owner;
	int lock_value;
	queue<Thread*> locked_threads;
};

struct Mesa_monitor 
{
	int lock;
	int condition;
	queue<Thread*> monitor_threads; 

};

queue<Thread*> ready_queue;
deque<Lock*> lock_queue;
deque<Mesa_monitor*> monitor_queue;

Thread* thread_curr;
ucontext_t* context_switcher;
bool thread_libinit_called;
int thread_num = 0;

std::deque<Lock*>::iterator lock_itr;
std::deque<Mesa_monitor*>::iterator monitor_itr;

void removeThread(Thread* current_thread)
{
	
	current_thread->ucontext_pointer->uc_stack.ss_sp = NULL;
	current_thread->ucontext_pointer->uc_stack.ss_size = 0;
	current_thread->ucontext_pointer->uc_stack.ss_flags = 0;
	current_thread->ucontext_pointer->uc_link = NULL;

	delete current_thread->stk;
	delete current_thread->ucontext_pointer;
	delete current_thread;
}

void start(thread_startfunc_t func, void* arg)
{
	func(arg);
	thread_curr->done = true;
	swapcontext(thread_curr->ucontext_pointer, context_switcher);

}


int thread_libinit(thread_startfunc_t func, void *arg)
{

	if(thread_libinit_called == true)
	{
		return -1;
	}

	thread_libinit_called = true;

	thread_create(func, arg);
	context_switcher = new ucontext_t;

	getcontext(context_switcher);
	swapcontext(context_switcher, thread_curr->ucontext_pointer);

	while(!ready_queue.empty())
	{
		if(thread_curr->done == true)
		{
			removeThread(thread_curr);
		}
		
		
		Thread* next_thread = ready_queue.front();
		ready_queue.pop();

		thread_curr = next_thread;
		swapcontext(context_switcher, thread_curr->ucontext_pointer);
		
	}

	if(thread_curr != NULL)
	{
	   removeThread(thread_curr);
	}

	cout << "Thread library exiting.\n";
	exit(0);

	return 0;

}

int thread_create(thread_startfunc_t func, void *arg)
{
	
	if(thread_libinit_called == false )
	{
		return -1;
	}

	Thread* thread = new Thread;
	
	ucontext_t* ucontext_pointer = new ucontext_t;
	
	getcontext(ucontext_pointer);
	thread->ucontext_pointer = ucontext_pointer;
	thread->thread_id = thread_num;
	thread_num++;

	char* stack = new char[STACK_SIZE];


	thread->ucontext_pointer->uc_stack.ss_sp = stack;
	thread->ucontext_pointer->uc_stack.ss_size = STACK_SIZE;
	thread->ucontext_pointer->uc_stack.ss_flags = 0;
	thread->ucontext_pointer->uc_link = NULL;

	thread->stk = stack;
	thread->done = false;

	makecontext(thread->ucontext_pointer, (void (*)()) start, 2, func, arg);

	//check to see if new thrad is first thread
	//if not first thread -- add to ready queue
	//else first thread will be NULL
	
	if(thread_curr != NULL)
	{
	   ready_queue.push(thread);
	}
	else
	{
	   thread_curr = thread;
	}

	return 0;

}

int thread_yield(void)
{
	
	if(thread_libinit_called == true)
	{

		ready_queue.push(thread_curr);
		swapcontext(thread_curr->ucontext_pointer, context_switcher);
		return 0;
	}
	else
	{
		return -1;
	}

}


int thread_lock(unsigned int lock_num)
{
	
	Lock* lock = new Lock;
	int count = 0;

	if(thread_libinit_called == false)
	{
		return -1;
	}

	//check to see if lock exists
	for(lock_itr = lock_queue.begin(); lock_itr != lock_queue.end(); ++lock_itr)
	{
		if((*lock_itr)->lock_value == lock_num)
		{
			lock = (*lock_itr);
			count++;
			if(lock->lock_owner == thread_curr)
			{
				return -1;
			}
		}
	}

	//new lock
	if(count == 0)
	{
		queue<Thread*> thread_queue;
		lock->lock_value = lock_num;
		lock->lock_owner = NULL;
		lock->locked_threads = thread_queue;
		lock_queue.push_back(lock);
	}
	 //lock exists
	
	if(lock->lock_owner == NULL) //lock is free
	{

		lock->lock_owner = thread_curr;
	}
	else //lock not free
	{

		lock->locked_threads.push(thread_curr);
		swapcontext(thread_curr->ucontext_pointer, context_switcher);
			
	}
	
	return 0;

}

int thread_unlock(unsigned int lock_num)
{
	
	Lock* lock = new Lock;
	int count = 0;

	if(thread_libinit_called == false)
	{
		return -1;
	}


	//check to see if lock exists
	for(lock_itr = lock_queue.begin(); lock_itr != lock_queue.end(); ++lock_itr)
	{
		if((*lock_itr)->lock_value == lock_num)
		{
			lock = (*lock_itr);
			count++;
			if(lock->lock_owner != thread_curr)
			{
				return -1;
			}
		}
	}

	if(count == 0) //lock doesnt exist
	{

		lock->lock_value = lock_num;
		lock_queue.push_back(lock);
	}

	// lock exists
	lock->lock_owner = NULL; 
		
	if(lock->locked_threads.size() > 0)
	{
		Thread* new_thread = new Thread;
		new_thread = lock->locked_threads.front();
		lock->lock_owner = new_thread;
		ready_queue.push(new_thread);
		lock->locked_threads.pop();
	}
		
		
	return 0;

}


int thread_wait(unsigned int lock, unsigned int cond)
{
	
	if(thread_libinit_called == false)
	{
		return -1;
	}

	Mesa_monitor* new_monitor = new Mesa_monitor;
	int count = 0;

	thread_unlock(lock);

	for(monitor_itr = monitor_queue.begin(); monitor_itr != monitor_queue.end(); ++monitor_itr)
	{
		if((*monitor_itr)->condition == cond)
		{
			new_monitor = (*monitor_itr);
			count++;
		}
	}

	if(count == 0) //condition not found
	{
		new_monitor->lock = lock;
		new_monitor->condition = cond;
		monitor_queue.push_back(new_monitor);
		
		
	}

	//condition found 
	new_monitor->monitor_threads.push(thread_curr);
	swapcontext(thread_curr->ucontext_pointer, context_switcher);
	thread_lock(lock);
	return 0;


}

int thread_signal(unsigned int lock, unsigned int cond)
{
	
	if(thread_libinit_called == false)
	{
		return -1;
	}

	Mesa_monitor* new_monitor = new Mesa_monitor;
	int count = 0;

	for(monitor_itr = monitor_queue.begin(); monitor_itr != monitor_queue.end(); ++monitor_itr)
	{
		if((*monitor_itr)->condition == cond)
		{
			new_monitor = (*monitor_itr);
			count++;
		}
	}

	if(count == 0) //condition not found
	{
		new_monitor->lock = lock;
		new_monitor->condition = cond;
		monitor_queue.push_back(new_monitor);
		

	}

	Thread* new_thread = new Thread;
	if(!new_monitor->monitor_threads.empty())
	{
		new_thread = new_monitor->monitor_threads.front();
		new_monitor->monitor_threads.pop();
		ready_queue.push(new_thread);
	}

	return 0;

}


int thread_broadcast(unsigned int lock, unsigned int cond)
{
	
	if(thread_libinit_called == false)
	{
		return -1;
	}

	Mesa_monitor* new_monitor = new Mesa_monitor;
	int count = 0;

	for(monitor_itr = monitor_queue.begin(); monitor_itr != monitor_queue.end(); ++monitor_itr)
	{
		if((*monitor_itr)->condition == cond)
		{
			new_monitor = (*monitor_itr);
			count++;
		}
	}

	if(count == 0) //condition not found
	{
		new_monitor->lock = lock;
		new_monitor->condition = cond;
		monitor_queue.push_back(new_monitor);
		
	}

	Thread* new_thread = new Thread;
	while(!new_monitor->monitor_threads.empty())
	{
		new_thread = new_monitor->monitor_threads.front();
		new_monitor->monitor_threads.pop();
		ready_queue.push(new_thread);
	}

	
	return 0;

}





