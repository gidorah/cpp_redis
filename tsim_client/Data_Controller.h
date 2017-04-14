#pragma once

#include <iostream>
#include <ctime>

#include "Shared_Memory_Extension.h" 
#include "Redis_Handler_Extension.h"
#include "Call_Back_Timer.h"

#define TIMER_INTERVAL 10

class Data_Controller
{
public:
	Data_Controller(std::string const & server_ip, int const & db_index, std::string const & segment_name);
	~Data_Controller();

private:
	Shared_Memory_Extension *shm_handler;
	Redis_Handler_Extension *redis_handler;
	Call_Back_Timer heartbeat;
	Call_Back_Timer testbeat;	

	void process_shm_changes(); /* shared memory'de karþý tarafýn yaptýðý deðiþiklikleri iþler. */

	void process_rpc_notifications(); /* shared memory'de karþý tarafýn gönderdiði method call notification'larýný iþler. */

	void process_data_notifications(); /* shared memory'de karþý tarafýn yaptýðý deðiþiklikleri hissedip
								redis sunucusuna iþler. */


	void test_print()
	{
		std::cout << "processed data count : " << shm_handler->test_count << std::endl;
		shm_handler->test_count = 0;

		//if (shm_handler->master == false)
		//{
		//	std::vector <double> frontCouplingForces;
		//	std::map<std::string, double> test_map;

		//	for (int i = 0; i < 50; ++i)  //Insert data in the vector
		//	{
		//		frontCouplingForces.push_back(i + 10000);
		//		test_map["key_" + std::to_string(i)] = i + 10000000;
		//		//redis_handler->set_value("test_int_" + std::to_string(i), (int)2312);
		//	}

		//	for (int i = 0; i < 100; ++i)  //Insert data in the vector
		//	{
		//		redis_handler->set_value("test_vector_" + std::to_string(i), frontCouplingForces);
		//		redis_handler->set_value("test_map_" + std::to_string(i), test_map);
		//	}

		//	//bool set_return;

		//	//set_return = redis_handler->set_value("test_map_1", test_map);

		//	//if (set_return == false)
		//	//	std::cout << "test_map_1 set failed!!!!\n";

		//	//set_return = redis_handler->set_value("test_vector_1", frontCouplingForces);

		//	//if (set_return == false)
		//	//	std::cout << "test_vector_1 set failed!!!!\n";

		//	//set_return = redis_handler->set_value("test_int_1", (int)2312);
		//	//set_return = redis_handler->set_value("test_int_2", (int)12);
		//	//set_return = redis_handler->set_value("test_int_3", (int)22);

		//	//if (set_return == false)
		//	//	std::cout << "test_int_1 set failed!!!!\n";
		//}
	}
};


