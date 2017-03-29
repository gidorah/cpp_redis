#include "Redis_Handler_Extension.h"

Redis_Handler_Extension::Redis_Handler_Extension(std::string const & server_ip, int const & db_index, Shared_Memory_Extension *shm) :
	Redis_Handler(server_ip, db_index, shm)
{
}


Redis_Handler_Extension::~Redis_Handler_Extension()
{
}
