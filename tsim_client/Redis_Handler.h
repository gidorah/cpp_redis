#pragma once

#include <cpp_redis/cpp_redis>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Shared_Memory_Extension.h"

#define REDIS_PRECISION 2 /*saklanacak ondalýklý sayýlarýn virgülden sonra kaç hane ilerleyeceðini belirler */

class Redis_Handler
{
public:
	Redis_Handler(std::string const & server_ip, Shared_Memory_Extension *shm);

	~Redis_Handler();

	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	void set_value(std::string const & key, T1 const & value, bool const & notification_enabled = true)
	{
		std::ostringstream oss_value;
		std::string str_value;
		oss_value.precision(REDIS_PRECISION);
		oss_value << std::fixed << value;
		str_value = oss_value.str();
		client.set(key, str_value, [=](cpp_redis::reply& reply)
		{
			std::string str_reply = reply.as_string();
		}).commit();

		if (notification_enabled)
			publish_notificaton(key, value);
	}
	
	template <typename T1>
	void get_value(std::string const & key, T1 const & value)
	{
		auto process_get_reply = [=](cpp_redis::reply& reply)
		{
			std::string str_reply = reply.as_string();
			T1 _value = boost::lexical_cast<T1>(str_reply);
			shm_handler->set_value(key, _value);
		};

		client.get(key, process_get_reply).commit();
	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	void set_value(std::string const & key, std::vector<T1> const & arg_vector)
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

		delete_key(key);
		sync_client.rpush(key, multi_set_vector);

		publish_notificaton(key, arg_vector);
	}

	template <typename T1>
	void get_value(std::string const & key, std::vector<T1> const & arg_vector)
	{
		client.llen(key, [=](cpp_redis::reply& reply) {

			if (reply.as_integer() == 0)
			{
				std::cerr << " Notification Error: list is empty or Key does NOT exits!" << std::endl;
				return;
			}

			client.lrange(key, 0, reply.as_integer(), [=](cpp_redis::reply& reply) {

				auto _array = reply.as_array();
				std::vector<T1> _vector;

				for (auto it = _array.begin(); it != _array.end(); it++)
				{
					T1 _val = boost::lexical_cast<T1>(((cpp_redis::reply)*it).as_string());
					_vector.push_back(_val);
				}

				shm_handler->set_value(key, _vector);

			}).commit();

		}).commit();
	}
	/*---------------------------------------------------------------------------------------------------------------*/

	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1, typename T2>
	void set_value(std::string const & key, std::map<T1, T2> const & arg_map)
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

		delete_key(key);
		sync_client.hmset(key, multi_set_vector);

		publish_notificaton(key, arg_map);
	}

	template <typename T1, typename T2>
	void get_value(std::string const & key, std::map<T1, T2> const & arg_map)
	{
		client.hgetall(key, [=](cpp_redis::reply& reply) {

			auto _array = reply.as_array();

			if (_array.size() <= 0)
			{
				std::cerr << " Notification Error: hash is empty or Key does NOT exits!" << std::endl;
				return;
			}

			std::map<T1, T2> _map;

			for (auto it = _array.begin(); it != _array.end(); it++)
			{

				T1 _key = boost::lexical_cast<T1>(((cpp_redis::reply)*it).as_string());
				it++;
				T2 _val = boost::lexical_cast<T2>(((cpp_redis::reply)*it).as_string());

				_map[_key] = _val;
			}

			shm_handler->set_value(key, _map);

		}).commit();
	}
	/*---------------------------------------------------------------------------------------------------------------*/

	void delete_key(std::string const & key)
	{
		std::vector<std::string> delete_vector;
		delete_vector.push_back(key);
		sync_client.del(delete_vector);
	}

	void subscribe(std::string const & key)
	{
		auto func_psubscribe_reply = [=](const std::string& chan, const std::string& msg) {

			std::string redis_key = chan.substr(chan.find(key)); /* client'ýn deðeri çekebilmesi için
																 key oluþturuluyor */

			handle_subscriber_reply(redis_key, msg);

		};

		subscriber.subscribe(key, func_psubscribe_reply).commit();
	}

	void handle_subscriber_reply(std::string const & key, std::string const & type)
	{
		if (type == "bool")
		{
			bool _value;
			get_value(key, _value);
		}
		else if (type == "int")
		{
			int _value;
			get_value(key, _value);
		}
		else if (type == "double")
		{
			double _value;
			get_value(key, _value);
		}
		else if (type == "string")
		{
			std::string _value;
			get_value(key, _value);
		}
		else if (type.find("vector") < type.size())
		{
			if (type == "vector_int")
			{
				std::vector<int> _vector;
				get_value(key, _vector);
			}
			else if (type == "vector_double")
			{
				std::vector<double> _vector;
				get_value(key, _vector);
			}
			else if (type == "vector_string")
			{
				std::vector<std::string> _vector;
				get_value(key, _vector);
			}
		}
		else if (type.find("map") < type.size())
		{
			if (type == "map_double_double")
			{
				std::map<double, double> _map;
				get_value(key, _map);
			}
			else if (type == "map_string_double")
			{
				std::map<std::string, double> _map;
				get_value(key, _map);

			}
			else if (type == "map_string_string")
			{
				std::map<std::string, std::string> _map;
				get_value(key, _map);
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
		auto reply = sync_client.publish(key, _type);

		if (reply.is_string())
		{
			std::string str_reply = reply.as_string();
		}
	}

	std::string set_lock(std::string const & key, int time_out = 1000);

	bool release_lock(std::string const & key, std::string const & uuid);

protected:
	cpp_redis::redis_client client;
	cpp_redis::sync_client sync_client;
	cpp_redis::future_client future_client;
	cpp_redis::redis_subscriber subscriber;

	Shared_Memory_Extension *shm_handler;
};

