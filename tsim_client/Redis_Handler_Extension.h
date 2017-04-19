#pragma once
#include "Redis_Handler.h"


namespace Redis_Extension
{
	/*------------------------------------------Level_2_data_packet-------------------------------------------------*/
	typedef struct
	{
		std::string svr_balise_name;    // Geçilen Balis Adý
		int svr_balise_type;            // Geçilen Balis Tipi
		double svr_balise_position;     // Geçilen Balis Pozisyonu
		int svr_has_relatedsignal;      // Balisin Ýliþkili Olduðu Sinyal Varlýðý
		int svr_relatedsignal_type;     // Ýliþkili Sinyal Varsa Tipi
		int svr_relatedsignal_aspect;   // Ýliþkili Sinyal Varsa Bildirimi
		int svr_relatedsignal_speedindicator; // Ýliþkili Sinyal Varsa Hýz Göstergesi

	}Level_2_balise_data;

	typedef struct                      /*Level 2 Bilgi Paketi*/
	{
		double svr_eoa_position;
		std::vector <Level_2_balise_data> incoming_balise_vector;
	}Level_2_data_packet;
	/*------------------------------------------Level_2_data_packet-------------------------------------------------*/

}


class Redis_Handler_Extension :
	public Redis_Handler
{
	
public:

	Redis_Handler_Extension(std::string const & server_ip, int const & db_index, Shared_Memory_Extension *shm);

	~Redis_Handler_Extension();

	void set_Level_2_data_packet(Redis_Extension::Level_2_data_packet const & packet)
	{
		std::string alias = "Level_2_data_packet.";
		
		std::string uuid = Redis_Handler::set_lock("Level_2_data_packet");

		Redis_Handler::set_value(alias + "svr_eoa_position", packet.svr_eoa_position, false);

		alias += "Level_2_balise_data.";

		std::vector<std::string> svr_balise_name;
		std::vector<int> svr_balise_type;
		std::vector<double> svr_balise_position;
		std::vector<int> svr_has_relatedsignal;
		std::vector<int> svr_relatedsignal_type;
		std::vector<int> svr_relatedsignal_aspect;
		std::vector<int> svr_relatedsignal_speedindicator;

		for (auto it = packet.incoming_balise_vector.begin(); it != packet.incoming_balise_vector.end(); it++)
		{
			svr_balise_name.push_back((*it).svr_balise_name);
			svr_balise_type.push_back((*it).svr_balise_type);
			svr_balise_position.push_back((*it).svr_balise_position);
			svr_has_relatedsignal.push_back((*it).svr_has_relatedsignal);
			svr_relatedsignal_type.push_back((*it).svr_relatedsignal_type);
			svr_relatedsignal_aspect.push_back((*it).svr_relatedsignal_aspect);
		}


		Redis_Handler::set_value(alias + "svr_balise_name", svr_balise_name);
		Redis_Handler::set_value(alias + "svr_balise_name", svr_balise_name);
		Redis_Handler::set_value(alias + "svr_balise_name", svr_balise_name);
		Redis_Handler::set_value(alias + "svr_balise_name", svr_balise_name);
		Redis_Handler::set_value(alias + "svr_balise_name", svr_balise_name);
		Redis_Handler::set_value(alias + "svr_balise_name", svr_balise_name);

		Redis_Handler::publish_notificaton("Level_2_data_packet", "struct");


		Redis_Handler::release_lock("Level_2_data_packet", uuid);

	}

	Redis_Extension::Level_2_data_packet get_Level_2_data_packet()
	{
		std::string alias = "Level_2_data_packet.";

		Redis_Extension::Level_2_data_packet  packet;

		std::string uuid = Redis_Handler::set_lock("Level_2_data_packet");


		Redis_Handler::get_value(alias + "svr_eoa_position", packet.svr_eoa_position);

		alias += "Level_2_balise_data.";

		Redis_Handler::get_value(alias + "svr_eoa_position", packet.svr_eoa_position);

		std::vector<std::string> svr_balise_name;
		std::vector<int> svr_balise_type;
		std::vector<double> svr_balise_position;
		std::vector<int> svr_has_relatedsignal;
		std::vector<int> svr_relatedsignal_type;
		std::vector<int> svr_relatedsignal_aspect;
		std::vector<int> svr_relatedsignal_speedindicator;

		Redis_Handler::get_value(alias + "svr_balise_name", svr_balise_name);
		Redis_Handler::get_value(alias + "svr_balise_type", svr_balise_type);
		Redis_Handler::get_value(alias + "svr_balise_position", svr_balise_position);
		Redis_Handler::get_value(alias + "svr_has_relatedsignal", svr_has_relatedsignal);
		Redis_Handler::get_value(alias + "svr_relatedsignal_type", svr_relatedsignal_type);
		Redis_Handler::get_value(alias + "svr_relatedsignal_aspect", svr_relatedsignal_aspect);
		Redis_Handler::get_value(alias + "svr_relatedsignal_speedindicator", svr_relatedsignal_speedindicator);

		Redis_Handler::release_lock("Level_2_data_packet", uuid);

		for (UINT it = 0; it < svr_balise_name.size(); it++)
		{
			Redis_Extension::Level_2_balise_data data;

			data.svr_balise_name = svr_balise_name[it];
			data.svr_balise_type = svr_balise_type[it];
			data.svr_balise_position = svr_balise_position[it];
			data.svr_has_relatedsignal = svr_has_relatedsignal[it];

			packet.incoming_balise_vector.push_back(data);
		}

		return packet;

	}

	void subscribe_Level_2_data_packet()
	{
		std::string const & key = "Level_2_data_packet";

		auto func_subscribe_reply = [=](const std::string& chan, const std::string& msg) {

			std::string redis_key = chan.substr(chan.find(key)); /* client'ýn deðeri çekebilmesi için
																 key oluþturuluyor */


																 //std::cout << "subscribe_reply : " << key << " || " << msg << std::endl;

			Redis_Extension::Level_2_data_packet packet = get_Level_2_data_packet();

		};

		subscriber.subscribe(sub_prefix + key, func_subscribe_reply).commit();
	}


};

