#include "Shared_Memory_Extension.h"



Shared_Memory_Extension::Shared_Memory_Extension(std::string const & segment_name)
	: Shared_Memory_Handler(segment_name)
{
}


Shared_Memory_Extension::~Shared_Memory_Extension()
{
}
