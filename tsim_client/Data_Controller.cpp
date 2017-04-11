#include "Data_Controller.h"


Data_Controller::Data_Controller(std::string const & server_ip, int const & db_index, std::string const & segment_name) :
	shm_handler(new Shared_Memory_Extension(segment_name)),
	redis_handler(new Redis_Handler_Extension(server_ip, db_index, shm_handler)),
	heartbeat(TIMER_INTERVAL, std::bind(&Data_Controller::process_shm_changes, this)),
	testbeat(1000, std::bind(&Data_Controller::test_print, this))
{

	std::vector <double> frontCouplingForces;
	std::map<std::string, double> test_map;

	//auto fn = std::bind(&Shared_Memory_Handler::get_notification_queue_size, shm_handler);

	//redis_handler->subscribe("val_d", value, shm_handler);
	//redis_handler->subscribe("val_i", value, shm_handler);
	//redis_handler->subscribe("vector");
	//redis_handler->subscriber_commit();
	//////redis_handler->set_value("val_d", 444.1);
	//////redis_handler->set_value("val_i", 555);
	//////redis_handler->set_value("val_b", false);

	////redis_handler->set_value("val_d", 444.1);
	////redis_handler->set_value("val_i", 555);
	////redis_handler->set_value("val_b", true);
	//redis_handler->client_commit();

	//for (int i = 0; i < 5; ++i)  //Insert data in the vector
	//{
	//	frontCouplingForces.push_back(i);
	//	test_map["key_" + std::to_string(i)] = i + 10000000;
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

		//int parameter_1 = 1000;
		//shm_handler->set_value("my_test_method.parameter_1", parameter_1, true);

		//int parameter_2 = 666;
		//shm_handler->set_value("my_test_method.parameter_2", parameter_2, true);

		//auto reply_ = [&]() {

		//	int return_val;
		//	std::cout << "first test of method call " << std::endl;
		//	shm_handler->get_value("my_test_method.return_val", return_val);
		//	std::cout << "return_val :  " << return_val << std::endl;

		//};

		//shm_handler->synced_remote_call("my_test_method", reply_);

		//std::cout << "synced remote call finished " << std::endl;


		//redis_handler->subscribe("test_map_1");
		//redis_handler->subscribe("test_vector_1");
		//redis_handler->subscribe("test_int_1");

	}
	else
	{

		//redis_handler->set_value("test_time_1", test_map);
		//std::string uuid = redis_handler->set_lock("test_time_1", 100000);

		//std::cout << "uuid : " << uuid << std::endl;


		//std::cout << "release_lock : " << redis_handler->release_lock("test_time_1", uuid) << std::endl;


		//	//Sleep(3000);
		//	//redis_handler->set_value("test_time", test_map);
		//	//redis_handler->set_value("test_map_2", test_map);
	}
}

Data_Controller::~Data_Controller()
{
}

void Data_Controller::process_shm_changes()
{
	process_data_notifications();
	process_rpc_notifications();
}

void Data_Controller::process_rpc_notifications()
{
	int process_count = shm_handler->get_rpc_notification_queue_size(); /* her iþlem aralýðýnda vector içindeki
																	bütün elemanlar iþlenir.*/

	while (process_count--)
	{
		std::cout << "process_count : " << process_count + 1 << std::endl;


		Shared::Rpc_Notification_Struct _notification;
		_notification = shm_handler->pop_rpc_notification();

		std::string _method_id = _notification._method_id;
		Shared::rpc_notification_type _type = _notification._type;
		std::string _uuid = _notification._uuid;

		std::cout << "process_shm_changes" << _method_id << " | " << _uuid << std::endl;

		if (_type == Shared::rpc_notification_type::reply)
		{
			shm_handler->handle_remote_call_reply(_uuid);
			continue;
		}

		if (_method_id == "my_test_method" && _type == Shared::rpc_notification_type::method_call)
		{
			int parameter_1;
			shm_handler->get_value("my_test_method.parameter_1", parameter_1);

			int parameter_2;
			shm_handler->get_value("my_test_method.parameter_2", parameter_2);

			int return_val;
			return_val = parameter_1 + parameter_2; // Burada çaðrýlmak istenen method ne ise o çaðrýlacak
			shm_handler->set_value("my_test_method.return_val", return_val, true);

			shm_handler->add_rpc_notification(_uuid, _method_id, Shared::rpc_notification_type::reply); // Dönüþ olarak rpc_notification_type reply olarak yollanmalý!!!
		}

	}
}

void Data_Controller::process_data_notifications()
{
	int process_count = shm_handler->get_data_notification_queue_size(); /* her iþlem aralýðýnda vector içindeki
																	bütün elemanlar iþlenir.*/

	while (process_count--)
	{
		//std::cout << "process_count : " << process_count + 1 << std::endl;

		Shared::Data_Notification_Struct _notification;
		_notification = shm_handler->pop_data_notification();

		std::string _typename = _notification._type;

		//std::cout << "process_shm_changes" << _notification._key << std::endl;

		//std::cout << " key : " << _notification._key << std::endl;
		//std::cout << " type : " << _notification._type << std::endl;


		try
		{
			/* TODO: Bunu böyle "if else" ile yapmak acý veriyor.
			fakat "get_value" methodunu dinamik type ile çaðýrmayý bulana kadar böyle kalacak */


			if (_notification._key == "##SUBSCRIPTIONS##")
			{
				std::vector<std::string> subscriptions;
				shm_handler->get_value(_notification._key, subscriptions);

				for (auto it = subscriptions.begin(); it != subscriptions.end(); it++)
				{
					redis_handler->subscribe(*it);
				}
			}
			else if (_notification._type == typeid(bool).name())
			{
				bool _value;
				shm_handler->get_value<bool>(_notification._key, _value);

				//std::cout << "bool value : " << _value << std::endl;
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(int).name())
			{
				int _value;
				shm_handler->get_value<int>(_notification._key, _value);
				//std::cout << "int value : " << _value << std::endl;
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(double).name())
			{
				double _value;
				shm_handler->get_value<double>(_notification._key, _value);
				//std::cout << "double value : " << _value << std::endl;
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::string).name())
			{
				std::string _value;
				shm_handler->get_value(_notification._key, _value);
				//std::cout << "string value : " << _value << std::endl;
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::vector<int>).name())
			{
				std::vector<int> _value;
				shm_handler->get_value<std::vector<int>>(_notification._key, _value);
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::vector<double>).name())
			{
				std::vector<double> _value;
				shm_handler->get_value(_notification._key, _value);
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::vector<std::string>).name())
			{
				std::vector<std::string> _value;
				shm_handler->get_value(_notification._key, _value);

				//for (auto i = _value.begin(); i != _value.end(); i++)  //Insert data in the vector
				//{
				//	std::cout << " val_s : " << (*i).c_str() << std::endl;
				//}

				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::map<double, double>).name())
			{
				std::map<double, double> _value;
				shm_handler->get_value(_notification._key, _value);
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::map<std::string, double>).name())
			{
				std::map<std::string, double> _value;
				shm_handler->get_value(_notification._key, _value);
				redis_handler->set_value(_notification._key, _value);
			}
			else if (_notification._type == typeid(std::map<std::string, std::string>).name())
			{
				std::map<std::string, std::string> _value;
				shm_handler->get_value(_notification._key, _value);
				redis_handler->set_value(_notification._key, _value);
			}
		}
		catch (const std::invalid_argument& ia)
		{
			std::cerr << "get_value : Invalid key '" << ia.what() << "'" << std::endl;
		}
	}
}