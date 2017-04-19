#include "Redis_Handler.h"



Redis_Handler::Redis_Handler(std::string const & server_ip, int const & db_index, Shared_Memory_Extension *shm) :
	shm_handler{ shm },
	db_index{ db_index },
	sub_prefix{ "#DB" + std::to_string(db_index) + "#:" }
{
	//cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	future_client.connect(server_ip, 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	subscriber.connect(server_ip, 6379, [](cpp_redis::redis_subscriber&) {
		std::cout << "subscriber disconnected (disconnection handler)" << std::endl;
	});

	future_client.select(db_index).get();

	//auto func_subscribe_reply = [=](const std::string& chan, const std::string& msg) {

	//	std::cout << "subscribe_reply : " << chan << " || " << msg << "\n";

	//	bool result;
	//	int _value = 100;
	//	std::string _key = "test_int_1";

	//	std::thread _thread([&] 
	//	{
	//		// same as client.send({ "GET", "hello" }, ...)
	//		client.get("test_int_1", [](cpp_redis::reply& reply) {
	//			std::cout << "get test_int_1: " << reply << std::endl;
	//			 if (reply.is_string())
	//				 std::cout << "get_value: " << reply.as_string() << "\n";				 
	//		}).sync_commit();
	//	});

	//	_thread.detach();

	//	//std::cout << "test_int_1 : " << _value << "\n";
	//	std::cout << "some text after client set \n";
	//};

	//subscriber.subscribe("test_channel", func_subscribe_reply).commit();
}


Redis_Handler::~Redis_Handler()
{
}

std::string Redis_Handler::set_lock(std::string const & key, int time_out)
{

	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	std::string str_uuid = boost::lexical_cast<std::string>(uuid);
	int interval = 100; // msec
	std::string _key = "##LOCK##:" + key;

	while ((time_out) > 0)
	{
		time_out -= interval;

		cpp_redis::reply reply = future_client.setnx(_key, str_uuid).get();

		//std::cout << "reply : " << reply << std::endl;

		if (reply.is_integer() && reply.as_integer() == 1)
		{
			//std::cout << "lock set!" << std::endl;
			future_client.expire(_key, 1).get(); /* TODO: normalde hiçbir kilit expire olmamalı.
										 Belki ileride bir diagnostic tool'u yazılırsa keyevent'ler
										 ile expire olan kilitler tespit edilebilir. */
			return str_uuid;
		}
		else
		{
			//std::cout << "lock failed trying again..." << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
		}
	}

	std::cout << "Redis_Handler lock failed! : " << key << std::endl;
	future_client.expire(_key, 1).get(); /* Bir şekilde kilit expire olmuyorsa gelecekteki hataları engellemek için */
	return std::string("-666");


}

bool Redis_Handler::release_lock(std::string const & key, std::string const & uuid)
{
	std::string _key = "##LOCK##:" + key;
	bool result = false;
	std::vector <std::string> watchlist;
	watchlist.push_back(_key);

	future_client.watch(watchlist).get();
	cpp_redis::reply reply = future_client.get(_key).get();

	if (reply.is_string())
	{
		if (reply.as_string() == uuid)
		{
			future_client.multi().get();
			future_client.del(watchlist).get();
			cpp_redis::reply reply = future_client.exec().get();
			future_client.unwatch().get();

			if (reply.is_array() && reply.as_array().size() != 0)
			{
				auto it = reply.as_array().begin();
				if ((*it).is_integer())
				{
					result = (*it).as_integer();
				}
				else
				{
					std::cerr << " release_lock : exec array element is NOT an integer!" << std::endl;
					return false;
				}
			}
			else
			{
				std::cerr << " release_lock : exec reply is NOT an integer!" << std::endl;
			}
		}
	}

	return result;
}
