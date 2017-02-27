#include "Data_Controller.h"


Data_Controller::Data_Controller() :
	shm_handler(new Shared_Memory_Extension("bizim_memory")),
	//redis_handler(new Redis_Handler("10.11.41.1", shm_handler)),
	redis_handler(new Redis_Handler),
	heartbeat(TIMER_INTERVAL, std::bind(&Data_Controller::process_shm_changes, this)),
	testbeat(1000, std::bind(&Data_Controller::test_print, this))
{

	std::vector <double> frontCouplingForces;
	std::map<std::string, double> test_map;

	//auto fn = std::bind(&Shared_Memory_Handler::get_notification_queue_size, shm_handler);

	//redis_handler->subscribe("val_d", value, shm_handler);
	//redis_handler->subscribe("val_i", value, shm_handler);
	redis_handler->subscribe("val_b", value, shm_handler);
	//redis_handler->subscriber_commit();
	//////redis_handler->set_value("val_d", 444.1);
	//////redis_handler->set_value("val_i", 555);
	//////redis_handler->set_value("val_b", false);

	////redis_handler->set_value("val_d", 444.1);
	////redis_handler->set_value("val_i", 555);
	////redis_handler->set_value("val_b", true);
	//redis_handler->client_commit();

	//for (int i = 0; i < 50000; ++i)  //Insert data in the vector
	//{
	//	frontCouplingForces.push_back(i);
	//	test_map["key_" + std::to_string(i)] = milliseconds_since_epoch;
	//}

	//unsigned long milliseconds_since_epoch =
	//	std::chrono::system_clock::now().time_since_epoch() /
	//	std::chrono::milliseconds(1);

	//for (int i = 0; i < 50000; ++i)  //Insert data in the vector
	//{
	//	frontCouplingForces.push_back(i);
	//	test_map["key_" + std::to_string(i)] = milliseconds_since_epoch;
	//}

	if (shm_handler->master)
	{
		//redis_handler->subscribe("test_map_1", subscribe_type::map, shm_handler);
		//redis_handler->subscribe("test_map_2", subscribe_type::map, shm_handler);
		//redis_handler->subscribe("test_time", subscribe_type::map, shm_handler);
		//Sleep(3000);
		//redis_handler->set_map("test_time_1", test_map);
		//redis_handler->subscriber_commit();

	}
	else
	{
		//redis_handler->subscribe("test_time", subscribe_type::value, shm_handler);
		//redis_handler->subscribe("test_time_1", subscribe_type::value, shm_handler);

		//redis_handler->subscribe("test_time_1", subscribe_type::map, shm_handler);
		//Sleep(3000);
		//redis_handler->set_map("test_time", test_map);
		//redis_handler->set_map("test_map_1", test_map);
		//redis_handler->set_map("test_map_2", test_map);
	}
}

Data_Controller::~Data_Controller()
{
}

void Data_Controller::process_shm_changes()
{
	int process_count = shm_handler->get_notification_queue_size(); /* her iþlem aralýðýnda vector içindeki
																	bütün elemanlar iþlenir.*/

	//std::cout << "process_count : " << process_count << std::endl;

	while (process_count--)
	{
		//std::cout << "process_count : " << process_count + 1 << std::endl;

		Shared::Notification_Struct _notification;
		_notification = shm_handler->pop_notification();

		std::cout << "process_shm_changes" << _notification._key <<std::endl;

		//std::cout << " key : " << _notification._key << std::endl;
		//std::cout << " type : " << _notification._type << std::endl;
		std::string _typename = _notification._type;

		try
		{
			//if (_notification._key == "test_time")
			//{
			//	unsigned long milliseconds_since_epoch =
			//		std::chrono::system_clock::now().time_since_epoch() /
			//		std::chrono::milliseconds(1);

			//	double test_value;
			//	shm_handler->get_value(_notification._key, test_value);
			//	std::cout << "shm_handler->get_value " << test_value << std::endl;


			//	std::cout << "time passed : " << milliseconds_since_epoch - test_value << std::endl;

			//	redis_handler->set_value(_notification._key,(double) milliseconds_since_epoch);
			//	std::cout << "redis_handler->set_value" << test_value << std::endl;
			//	//std::cout << "." << std::endl;
			//}
			//else if (_notification._key == "test_time_1")
			//{
			//	unsigned long milliseconds_since_epoch =
			//		std::chrono::system_clock::now().time_since_epoch() /
			//		std::chrono::milliseconds(1);

			//	double test_value;
			//	shm_handler->get_value(_notification._key, test_value);
			//	std::cout << "shm_handler->get_value " << test_value << std::endl;

			//	std::cout << "time passed_1 : " << milliseconds_since_epoch - test_value << std::endl;

			//	redis_handler->set_value(_notification._key, (double)milliseconds_since_epoch);
			//	std::cout << "redis_handler->set_value" << test_value << std::endl;
			//	//std::cout << "." << std::endl;
			//}
			//else 
			//if (_notification._key == "test_time")
			//{
			//	std::map<std::string, std::string> test_map;
			//	shm_handler->get_value(_notification._key, test_map);
			//	redis_handler->set_map(_notification._key, test_map);
			//	std::cout << "." << std::endl;
			//}
			//else if (_notification._key == "test_time_1")
			//{
			//	std::map<std::string, std::string> test_map;
			//	shm_handler->get_value(_notification._key, test_map);
			//	redis_handler->set_map(_notification._key, test_map);
			//	std::cout << "." << std::endl;
			//}
		}
		catch (const std::invalid_argument& ia)
		{
			std::cerr << "get_value : Invalid key '" << ia.what() << "'" << std::endl;
		}


		//try
		//{
		//	/* TODO: Bunu böyle "if else" ile yapmak acý veriyor.
		//	fakat "get_value" methodunu dinamik type ile çaðýrmayý bulana kadar böyle kalacak */
		//	if (_notification._type == typeid(bool).name())
		//	{
		//		bool _value; 
		//		shm_handler->get_value<bool>(_notification._key, _value);

		//		std::cout << "bool value : " << _value << std::endl;
		//		//redis_handler->set_value(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(int).name())
		//	{
		//		int _value;
		//		shm_handler->get_value<int>(_notification._key, _value);
		//		std::cout << "int value : " << _value << std::endl;
		//		////redis_handler->set_value(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(double).name())
		//	{
		//		double _value;
		//		shm_handler->get_value<double>(_notification._key, _value);
		//		std::cout << "double value : " << _value << std::endl;
		//		////redis_handler->set_value(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::string).name())
		//	{
		//		std::string _value;
		//		shm_handler->get_value(_notification._key, _value);
		//		std::cout << "string value : " << _value << std::endl;
		//		////redis_handler->set_value(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::vector<int>).name())
		//	{
		//		std::vector<int> _value;
		//		shm_handler->get_value<std::vector<int>>(_notification._key, _value);
		//		//redis_handler->set_vector(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::vector<double>).name())
		//	{
		//		std::vector<double> _value;
		//		shm_handler->get_value(_notification._key, _value);
		//		//redis_handler->set_vector(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::vector<std::string>).name())
		//	{
		//		std::vector<std::string> _value;
		//		shm_handler->get_value(_notification._key, _value);

		//		for (auto i = _value.begin() ; i != _value.end(); i++)  //Insert data in the vector
		//		{
		//			std::cout << " val_s : " << (*i).c_str() << std::endl;
		//		}

		//		//redis_handler->set_vector(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::map<double, double>).name())
		//	{
		//		std::map<double, double> _value;
		//		shm_handler->get_value(_notification._key, _value);
		//		//redis_handler->set_map(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::map<std::string, double>).name())
		//	{
		//		std::map<std::string, double> _value;
		//		shm_handler->get_value(_notification._key, _value);
		//		//redis_handler->set_map(_notification._key, _value);
		//	}
		//	else if (_notification._type == typeid(std::map<std::string, std::string>).name())
		//	{
		//		std::map<std::string, std::string> _value;
		//		shm_handler->get_value(_notification._key, _value);
		//		//redis_handler->set_map(_notification._key, _value);
		//	}
		//}
		//catch (const std::invalid_argument& ia)
		//{
		//	std::cerr << "get_value : Invalid key '" << ia.what() << "'" << std::endl;
		//}
	}

	//redis_handler->client_commit();

}