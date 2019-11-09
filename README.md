# Disk-Scheduler
Created a C++ disk scheduler that simulates how an operating system gets and schedules disk threads. Threads issue disk requests by queueing them on the disk scheduler. The number of disk requesters is specified in the arguments to run the code, and there is one thread to service all the requests. The disk queues are serviced in shortest seek time first.

disk.cc  -> Concurrent disk scheduler C++ program 

****************************************************************************************************************************

Created a C++ thread library designed to allow for writing multi-threaded programs on Linux machines.  thread.cc is my implementation of the thread libraries. This was then tested using disk.cc with various inputs to make sure it still worked as intended. 

thread.cc -> implemented thread library functions, uses (disk.cc) as driver for testing
