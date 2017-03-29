#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <Winsock2.h>

#include "Data_Controller.h"

/* 
þu þekilde çaðrýlabilir: 
	tsim_client.exe -segment_name=test_segment -server_ip=10.11.41.1 -db_index=9 
*/

int main(int argc, char *argv[])
{

#ifdef _WIN32

	//! Windows netword DLL init
	WORD version = MAKEWORD(2, 2);
	WSADATA data;

	if (WSAStartup(version, &data) != 0)
	{
		std::cerr << "WSAStartup() failure" << std::endl;
		return -1;
	}

#endif /* _WIN32 */

	std::string _segment_name = "default_segment";
	std::string _server_ip = "10.11.41.1";
	int _db_index = 1;

	std::string config_file_name = "config.ini";
	
	struct stat buffer;
	bool file_exits = (stat(config_file_name.data(), &buffer) == 0);

	if (file_exits)
	{
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini(config_file_name, pt);

		try
		{
		_segment_name = pt.get<std::string>("shm.segment_name");
		}
		catch (const std::exception&)
		{
			std::cout << "segment_name does not exits in file!" << std::endl;
		}

		try
		{
			_server_ip = pt.get<std::string>("redis.server_ip");
		}
		catch (const std::exception&)
		{
			std::cout << "server_ip does not exits in file!" << std::endl;

		}

		try
		{
			_db_index = pt.get<int>("redis.db_index");
		}
		catch (const std::exception&)
		{
			std::cout << "db_index does not exits in file!" << std::endl;
		}
	}
	else
	{
		std::cout << "config.ini is empty or does not exits!" << std::endl;
	}

	while (--argc > 0)
	{
		int parameter_index;
		std::string parameter_name;

		std::string parameter = argv[argc];

		parameter_name = "-segment_name=";
		parameter_index = parameter.find(parameter_name);

		if (parameter_index < parameter.size())
		{
			_segment_name = parameter.substr(parameter_name.size(), parameter.size() - parameter_name.size());
			continue;
		}

		parameter_name = "-server_ip=";
		parameter_index = parameter.find(parameter_name);

		if (parameter_index < parameter.size())
		{
			_server_ip = parameter.substr(parameter_name.size(), parameter.size() - parameter_name.size());
			continue;
		}

		parameter_name = "-db_index=";
		parameter_index = parameter.find(parameter_name);

		if (parameter_index < parameter.size())
		{
			_db_index = stoi(parameter.substr(parameter_name.size(), parameter.size() - parameter_name.size()));
			continue;
		}

	}

	std::cout << "segment_name : " << _segment_name << std::endl;
	std::cout << "server_ip : " << _server_ip << std::endl;
	std::cout << "db_index : " << _db_index << std::endl;

	Data_Controller data_controller(_server_ip, _db_index, _segment_name);
}
