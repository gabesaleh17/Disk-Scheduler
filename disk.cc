#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <ctype.h>
#include <sstream>
#include <map>
#include "thread.h"
#include <deque>
#include <algorithm>
#include <math.h>

using namespace std;

void servicer(void *arg);
void requester(void * request_tag);
void thread_starter(void *arg);

int MAX_QUEUE_SIZE;
int current_track_num;
int file_num;
char** disk_names;
int queue_lock = 1;
int queue_not_full = 2;
int queue_full = 3;
int possible_threads;
int actual_threads = 0;
int deque_lock = 4;
int map_lock = 5;
 

struct requester_data_type
{
   int tag;
   int track_num;
};

 
deque<requester_data_type> request_queue;
deque<string> files; 
map<int,int> requests_to_finish;
deque<deque<int> > list_of_tracks;

bool serviced(int request_tag)
{
	for(int i = 0; i <request_queue.size(); i++)
	{
		if(request_queue[i].tag == request_tag)
		{
			
			return true;
		}
	}

	return false;
}

bool shortest_path(requester_data_type start, requester_data_type end)
{
	bool shortest;
	shortest = abs(start.track_num - current_track_num) < abs(end.track_num - current_track_num);
	return shortest;
}

int main(int argc, char* argv[])
{

	MAX_QUEUE_SIZE = atoi(argv[1]);
	file_num = argc;
	disk_names = argv + 2;
	possible_threads = argc - 2;


	for(int i = 0; i < possible_threads; i++)
	{
		files.push_back(argv[i+2]);
	}

	if (thread_libinit((thread_startfunc_t) thread_starter, (void *) 0))
	   {
		cout << "thread_libinit failed\n";
		exit(1);
	   }
	
	return 0;


}


void requester(void* request_tag)
{

	thread_lock(deque_lock);
	int requester_tag = (intptr_t)request_tag;

	deque<int> list_of_track = list_of_tracks.front();

	thread_lock(map_lock);
	requests_to_finish[requester_tag] = list_of_track.size();
	thread_unlock(map_lock);
	
	list_of_tracks.pop_front();
	thread_unlock(deque_lock);

	
	while(requests_to_finish[requester_tag] > 0 && MAX_QUEUE_SIZE > 0)
	{
		

		thread_lock(queue_lock);
		while(request_queue.size() == MAX_QUEUE_SIZE || serviced(requester_tag) || (list_of_track.empty() && requests_to_finish[requester_tag] >= 0))
		{
			
			thread_wait(queue_lock, queue_not_full);
		}
		
		
		
		if(!list_of_track.empty())
		{
			requester_data_type request;
			//cout << list_of_track.front() << endl;
			request.track_num = list_of_track.front();
			list_of_track.pop_front();
			request.tag = requester_tag;
			request_queue.push_back(request);

			cout << "requester " << request.tag << " track " << request.track_num << endl;
			

			thread_signal(queue_lock, queue_full);
			thread_unlock(queue_lock);

		
		}

	}
		

}

void servicer(void* arg)
{
     

  while(possible_threads > 0 || request_queue.size() > 0 && (MAX_QUEUE_SIZE >= 0))
  {
  	thread_lock(queue_lock);

 
  	if(MAX_QUEUE_SIZE > possible_threads)
  	{
  		MAX_QUEUE_SIZE = possible_threads;
  	}
	

  	while(request_queue.size() < MAX_QUEUE_SIZE)
  	{
  	 
     thread_wait(queue_lock, queue_full);
  	}  
	
  	sort(request_queue.begin(), request_queue.end(), shortest_path);
  	requester_data_type request = request_queue.front();
  	request_queue.pop_front();
  	current_track_num = request.track_num;

  	thread_lock(map_lock);
  	requests_to_finish[request.tag] = requests_to_finish[request.tag]-1;
  	thread_unlock(map_lock);
  	
  	if(requests_to_finish[request.tag] == 0)
  	{
  		possible_threads--;
  	}
	

  	cout << "service requester " << request.tag<< " track " << request.track_num << endl;
  	


  	thread_broadcast(queue_lock, queue_not_full);
  	thread_unlock(queue_lock);

  		
  }
   

}



void thread_starter(void* arg)
{
	if (MAX_QUEUE_SIZE > 0)
	{
   	  for(int i = 0; i < possible_threads; i++)
   	 {
   		string disk_name = "";
   		string track_number = "";

   		disk_name =  files.front().c_str();
   		//cout << disk_name << endl;
   		ifstream disk_file(files.front().c_str());
   		files.pop_front();
   		deque<int> list_of_track;
   		//cout << "thread_starter" << endl;
   		if(disk_file.is_open())
   		{

   				while(getline(disk_file, track_number))
   				{
   					
   					list_of_track.push_back(atoi(track_number.c_str()));

   				}
   		}

   		
   		list_of_tracks.push_back(list_of_track);
  
  	
   		while(!list_of_track.empty())
   		{
   			
   			list_of_track.pop_front();

   		}
   		
   		
   		
   		thread_create((thread_startfunc_t) requester, (void *) i);
   		

   	  }
   	  	
     	thread_create((thread_startfunc_t) servicer, 0);
    }
}








