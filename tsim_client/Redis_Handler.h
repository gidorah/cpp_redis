#pragma once

#include <cpp_redis/cpp_redis>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "sync_client.hpp"
#include "Shared_Memory_Extension.h"

#define REDIS_PRECISION 2 /*saklanacak ondalýklý sayýlarýn virgülden sonra kaç hane ilerleyeceðini belirler */

class Redis_Handler
{

protected:
	cpp_redis::redis_client client;
	cpp_redis::sync_client sync_client;
	cpp_redis::future_client future_client;
	cpp_redis::redis_subscriber subscriber;

	Shared_Memory_Extension *shm_handler;

	int db_index{ 0 };
	std::string sub_prefix;

public:
	Redis_Handler(std::string const & server_ip, int const & db_index, Shared_Memory_Extension *shm);

	~Redis_Handler();

	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	bool set_value(std::string const & key, T1 const & value, bool const & notification_enabled = true)
	{
		std::ostringstream oss_value;
		std::string str_value;
		oss_value.precision(REDIS_PRECISION);
		oss_value << std::fixed << value;
		str_value = oss_value.str();

		auto reply = sync_client.set(key, str_value);

		if (notification_enabled)
			publish_notificaton(key, value);

		return true;
	}

	template <typename T1>
	bool get_value(std::string const & key, T1 & value)
	{
		//auto process_get_reply = [&](cpp_redis::reply& reply)
		//{
		//	std::string str_reply = reply.as_string();

		//	std::cout << "get_value : " << str_reply << "\n";

		//	value = boost::lexical_cast<T1>(str_reply);

		//	std::cout << "get_value _value : " << value << "\n";
		//};

		//client.get(key, process_get_reply).commit();

		auto reply = sync_client.get(key);

		if (reply.is_string() == false)
		{
			std::cerr << " Notification : Key does NOT exits!" << std::endl;
			return false;
		}

		std::string str_reply = reply.as_string();
		value = boost::lexical_cast<T1>(str_reply);
		return true;

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	bool set_value(std::string const & key, std::vector<T1> const & arg_vector)
	{
		std::vector <std::string> multi_set_vector;

		for (auto it = arg_vector.begin(); it != arg_vector.end(); it++)
		{
			std::pair<std::string, std::string> multi_set_pair;
			std::ostringstream oss_value;

			oss_value.precision(REDIS_PRECISION);
			oss_value << std::fixed << *it;
			multi_set_vector.push_back(oss_value.str());
		}

		std::string uuid = set_lock(key);
		if (uuid == "-666")
			return false;

		delete_key(key);
		sync_client.rpush(key, multi_set_vector);
		release_lock(key, uuid);

		publish_notificaton(key, arg_vector);
		return true;
	}

	template <typename T1>
	bool get_value(std::string const & key, std::vector<T1> & arg_vector)
	{
		//int len{ 0 };

		//client.llen(key, [&](cpp_redis::reply& reply) {

		//	len = reply.as_integer();

		//	if (len == 0)
		//	{
		//		std::cerr << " Notification : list is empty or Key does NOT exits!" << std::endl;
		//		return;
		//	}
		//	else
		//	{
		//		std::cout << "reply llen: " << len << "\n";
		//	}


		//	std::cout << "reply llen 2: " << len << "\n";

		//	client.lrange(key, 0, len, [&](cpp_redis::reply& reply) {

		//		auto _array = reply.as_array();

		//		for (auto it = _array.begin(); it != _array.end(); it++)
		//		{
		//			T1 _val = boost::lexical_cast<T1>(((cpp_redis::reply)*it).as_string());
		//			arg_vector.push_back(_val);
		//		}

		//	}).commit();

		//}).commit();

		int len{0};

		std::string uuid = set_lock(key);
		if (uuid == "-666")
			return false;

		auto reply_llen = sync_client.llen(key);

		len = reply_llen.as_integer();

		if (len == 0)
		{
			release_lock(key, uuid);
			std::cerr << " Notification : list is empty or Key does NOT exits!" << std::endl;
			return false;
		}

		auto reply_lrange = sync_client.lrange(key, 0, len);
		release_lock(key, uuid);

		auto _array = reply_lrange.as_array();

		for (auto it = _array.begin(); it != _array.end(); it++)
		{
			T1 _val = boost::lexical_cast<T1>(((cpp_redis::reply)*it).as_string());
			arg_vector.push_back(_val);
		}

		return true;

	}
	/*---------------------------------------------------------------------------------------------------------------*/

	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1, typename T2>
	bool set_value(std::string const & key, std::map<T1, T2> const & arg_map)
	{

		std::vector < std::pair<std::string, std::string>> multi_set_vector;

		for (auto it = arg_map.begin(); it != arg_map.end(); it++)
		{
			std::pair<std::string, std::string> multi_set_pair;
			std::ostringstream oss_value, oss_key;

			oss_key.precision(REDIS_PRECISION);
			oss_key << std::fixed << it->first;

			oss_value.precision(REDIS_PRECISION);
			oss_value << std::fixed << it->second;

			multi_set_pair.first = oss_key.str();
			multi_set_pair.second = oss_value.str();

			multi_set_vector.push_back(multi_set_pair);
		}

		std::string uuid = set_lock(key);
		if (uuid == "-666")
			return false;

		delete_key(key);

		cpp_redis::reply reply = sync_client.hmset(key, multi_set_vector);
		release_lock(key, uuid);

		//std::cout << "hmset : " << reply.as_string() << std::endl;

		publish_notificaton(key, arg_map);
		return true;
	}

	template <typename T1, typename T2>
	bool get_value(std::string const & key, std::map<T1, T2> & arg_map)
	{
		//client.hgetall(key, [&](cpp_redis::reply& reply) {

		//	auto _array = reply.as_array();

		//	if (_array.size() <= 0)
		//	{
		//		std::cerr << " Notification Error: hash is empty or Key does NOT exits!" << std::endl;
		//		return;
		//	}

		//	for (auto it = _array.begin(); it != _array.end(); it++)
		//	{

		//		T1 _key = boost::lexical_cast<T1>(((cpp_redis::reply)*it).as_string());
		//		it++;
		//		T2 _val = boost::lexical_cast<T2>(((cpp_redis::reply)*it).as_string());

		//		arg_map[_key] = _val;
		//	}

		//}).commit();

		std::string uuid = set_lock(key);
		if (uuid == "-666")
			return false;

		auto reply = sync_client.hgetall(key);
		release_lock(key, uuid);

		auto _array = reply.as_array();

		if (_array.size() <= 0)
		{
			std::cerr << " Notification Error: hash is empty or Key does NOT exits!" << std::endl;
			return false;
		}

		for (auto it = _array.begin(); it != _array.end(); it++)
		{

			T1 _key = boost::lexical_cast<T1>(((cpp_redis::reply)*it).as_string());
			it++;
			T2 _val = boost::lexical_cast<T2>(((cpp_redis::reply)*it).as_string());

			arg_map[_key] = _val;
		}
		return true;
	}
	/*---------------------------------------------------------------------------------------------------------------*/

	void delete_key(std::string const & key)
	{
		std::vector<std::string> delete_vector;
		delete_vector.push_back(key);
		cpp_redis::reply reply = sync_client.del(delete_vector);

		//std::cout << "del : " << reply.as_integer() << std::endl;
	}

	void subscribe(std::string const & key)
	{
		auto func_subscribe_reply = [=](const std::string& chan, const std::string& msg) {

			std::string redis_key = chan.substr(chan.find(key)); /* client'ýn deðeri çekebilmesi için
																 key oluþturuluyor */

			std::cout << "subscribe_reply : " << key << " || " << msg << std::endl;

			handle_subscriber_reply(redis_key, msg);
		};

		subscriber.subscribe(sub_prefix + key, func_subscribe_reply).commit();
	}

	void handle_subscriber_reply(std::string const & key, std::string const & type)
	{
		if (type == "bool")
		{
			bool _value;

			if (get_value(key, _value))
			{
				shm_handler->set_value(key, _value);
			}
		}
		else if (type == "int")
		{
			int _value;

			if (get_value(key, _value))
			{
				shm_handler->set_value(key, _value);
			}
		}
		else if (type == "double")
		{
			double _value;

			if (get_value(key, _value))
			{
				shm_handler->set_value(key, _value);
			}
		}
		else if (type == "string")
		{
			std::string _value;

			if (get_value(key, _value))
			{
				shm_handler->set_value(key, _value);
			}
		}
		else if (type.find("vector") < type.size())
		{
			if (type == "vector_int")
			{
				std::vector<int> _vector;

				if (get_value(key, _vector))
				{
					shm_handler->set_value(key, _vector);
				}
			}
			else if (type == "vector_double")
			{
				std::vector<double> _vector;

				if (get_value(key, _vector))
				{
					shm_handler->set_value(key, _vector);
				}
			}
			else if (type == "vector_string")
			{
				std::vector<std::string> _vector;

				if (get_value(key, _vector))
				{
					shm_handler->set_value(key, _vector);
				}
			}
		}
		else if (type.find("map") < type.size())
		{
			if (type == "map_double_double")
			{
				std::map<double, double> _map;

				if (get_value(key, _map))
				{
					shm_handler->set_value(key, _map);
				}
			}
			else if (type == "map_string_double")
			{
				std::map<std::string, double> _map;

				if (get_value(key, _map))
				{
					shm_handler->set_value(key, _map);
				}
			}
			else if (type == "map_string_string")
			{
				std::map<std::string, std::string> _map;

				if (get_value(key, _map))
				{
					shm_handler->set_value(key, _map);
				}
			}
		}
		else
		{
			std::cout << "Redis_Handler::get_type : wrong type!" << std::endl;
		}
	}

	template <typename T1>
	std::string get_type(T1 const & arg)
	{
		std::string _type;
		_type = "";

		if (typeid(T1) == typeid(bool))
		{
			_type.append("bool");
		}
		else if (typeid(T1) == typeid(int))
		{
			_type.append("int");
		}
		else if (typeid(T1) == typeid(double))
		{
			_type.append("double");
		}
		else if (typeid(T1) == typeid(std::string))
		{
			_type.append("string");
		}
		else if (typeid(T1) == typeid(std::vector<int>))
		{
			_type.append("vector_int");
		}
		else if (typeid(T1) == typeid(std::vector<double>))
		{
			_type.append("vector_double");
		}
		else if (typeid(T1) == typeid(std::vector<std::string>))
		{
			_type.append("vector_string");
		}
		else if (typeid(T1) == typeid(std::map<double, double>))
		{
			_type.append("map_double_double");
		}
		else if (typeid(T1) == typeid(std::map<std::string, double>))
		{
			_type.append("map_string_double");
		}
		else if (typeid(T1) == typeid(std::map<std::string, std::string>))
		{
			_type.append("map_string_string");
		}
		else
		{
			std::cout << "Redis_Handler::get_type : wrong type!" << std::endl;
		}

		return _type;

	}

	template <typename T1>
	void publish_notificaton(std::string const & key, T1 const & arg)
	{
		std::string _type = get_type(arg);
		cpp_redis::reply reply = sync_client.publish(sub_prefix + key, _type);
		//std::cout << "publish : " << reply.as_string() << std::endl;
	}

	std::string set_lock(std::string const & key, int time_out = 1000);

	bool release_lock(std::string const & key, std::string const & uuid);

};

