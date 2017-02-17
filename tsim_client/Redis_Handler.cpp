#include "Redis_Handler.h"



Redis_Handler::Redis_Handler()
{
	cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	client.connect("10.11.41.1", 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	future_client.connect("10.11.41.1", 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	subscriber.connect("10.11.41.1", 6379, [](cpp_redis::redis_subscriber&) {
		std::cout << "subscriber disconnected (disconnection handler)" << std::endl;
	});

	client.select(11);
	client.commit();
}


Redis_Handler::~Redis_Handler()
{
}

void Redis_Handler::client_commit()
{
	client.commit();
}

void Redis_Handler::subscriber_commit()
{
	subscriber.commit();
}



