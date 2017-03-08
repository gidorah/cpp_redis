#pragma once
#include "Redis_Handler.h"


namespace Redis_Extension
{
	/*------------------------------------------Level_2_data_packet-------------------------------------------------*/
	typedef struct
	{
		std::string svr_balise_name;    // Ge�ilen Balis Ad�
		int svr_balise_type;            // Ge�ilen Balis Tipi
		double svr_balise_position;     // Ge�ilen Balis Pozisyonu
		int svr_has_relatedsignal;      // Balisin �li�kili Oldu�u Sinyal Varl���
		int svr_relatedsignal_type;     // �li�kili Sinyal Varsa Tipi
		int svr_relatedsignal_aspect;   // �li�kili Sinyal Varsa Bildirimi
		int svr_relatedsignal_speedindicator; // �li�kili Sinyal Varsa H�z G�stergesi

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

	Redis_Handler_Extension(std::string const & server_ip, Shared_Memory_Extension *shm);

	~Redis_Handler_Extension();

	void set_Level_2_data_packet(std::string const & key, Redis_Extension::Level_2_data_packet const & packet)
	{
		std::string alias = "Level_2_data_packet.";

		Redis_Handler::set_value(alias + "svr_eoa_position", packet.svr_eoa_position, false);

		alias += "Level_2_balise_data.";

		for (auto it = packet.incoming_balise_vector.begin(); it != packet.incoming_balise_vector.end(); it++)
		{

		}

		//Redis_Handler::set_vector(alias + "svr_balise_name", packet.)

	}

	Redis_Extension::Level_2_data_packet get_value(std::string const & key)
	{

	}

	//void subscribe(std::string const & key)
	//{

	//}


};

