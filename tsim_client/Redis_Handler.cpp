#include "Redis_Handler.h"



Redis_Handler::Redis_Handler(std::string const & server_ip, int const & db_index, Shared_Memory_Extension *shm) :
	shm_handler{shm},
	db_index{db_index},
	sub_prefix{ "#DB" + std::to_string(db_index) + "#:"}
{
	//cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	client.connect(server_ip, 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	sync_client.connect(server_ip, 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	future_client.connect(server_ip, 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	subscriber.connect(server_ip, 6379, [](cpp_redis::redis_subscriber&) {
		std::cout << "subscriber disconnected (disconnection handler)" << std::endl;
	});

	future_client.select(db_index);
	sync_client.select(db_index);
	client.select(db_index);
	client.commit();
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

		cpp_redis::reply reply = sync_client.setnx(_key, str_uuid);

		//std::cout << "reply : " << reply << std::endl;

		if (reply.as_integer() == 1)
		{
			//std::cout << "lock set!" << std::endl;
			sync_client.expire(_key, 1); /* TODO: normalde hiçbir kilit expire olmamalı.
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
	return std::string("-666");


}

bool Redis_Handler::release_lock(std::string const & key, std::string const & uuid)
{
	std::string _key = "##LOCK##:" + key;
	bool result = false;
	std::vector <std::string> watchlist;
	watchlist.push_back(_key);

	sync_client.watch(watchlist);
	cpp_redis::reply reply = sync_client.get(_key);

	if (reply.is_string())
	{
		if (reply.as_string() == uuid)
		{
			sync_client.multi();
			sync_client.del(watchlist);
			cpp_redis::reply reply = sync_client.exec();
			future_client.unwatch();

			if (reply.as_array().size() != 0)
			{
				auto it = reply.as_array().begin();
				result = (*it).as_integer();
			}
		}
	}

	return result;
}
