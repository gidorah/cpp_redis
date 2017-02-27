#include "Redis_Handler_Extension.h"

Redis_Handler_Extension::Redis_Handler_Extension(std::string const & server_ip, Shared_Memory_Extension *shm) :
	Redis_Handler(server_ip, shm)
{
}


Redis_Handler_Extension::~Redis_Handler_Extension()
{
}
