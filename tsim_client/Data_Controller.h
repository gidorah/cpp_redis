#pragma once

#include <iostream>
#include <ctime>

#include "Shared_Memory_Extension.h" 
#include "Redis_Handler_Extension.h"
#include "Call_Back_Timer.h"

#define TIMER_INTERVAL 100

class Data_Controller
{
public:
	Data_Controller();
	~Data_Controller();

private:
	Shared_Memory_Extension *shm_handler;
	Redis_Handler_Extension *redis_handler;
	Call_Back_Timer heartbeat;
	Call_Back_Timer testbeat;	

	void process_shm_changes(); /* shared memory'de karþý tarafýn yaptýðý deðiþiklikleri hissedip
								redis sunucusuna iþler. */

	void test_print()
	{
		std::cout << "processed data count : " << shm_handler->test_count << std::endl;
		shm_handler->test_count = 0;
	}
};


