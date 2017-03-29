#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "Data_Controller.h"

int main(void)
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

	std::string _segment_name = "";
	std::string _server_ip = "";
	int _selected_db = -1;
	
	struct stat buffer;
	bool file_exits =  (stat("config.ini", &buffer) == 0);

	if (file_exits)
	{
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini("config.ini", pt);
		_segment_name = pt.get<std::string>("shm.segment_name");
		_server_ip = pt.get<std::string>("redis.server_ip");
		_selected_db = pt.get<int>("redis.selected_db");

		std::cout << "_segment_name : " << _segment_name << std::endl;
		std::cout << "_server_ip : " << _server_ip << std::endl;
		std::cout << "_selected_db : " << _selected_db << std::endl;
	}
	else
	{
		std::cout << "config.ini is empty or does not exits!" << std::endl;
	}

	Data_Controller data_controller;
}
