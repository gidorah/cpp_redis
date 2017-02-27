#pragma once

#include <cpp_redis/cpp_redis>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#define REDIS_PRECISION 2 /*saklanacak ondalýklý sayýlarýn virgülden sonra kaç hane ilerleyeceðini belirler */

enum subscribe_type { value, vector, map, field }; /*

		* Redis_Handler class'ý subscribe method'u için takip edilecek deðer,
		  yapý ya da alanýn belirlenmesi için.
		* value:  int, double ve bool için ortak tip. Redis içerisinde sadece tek tip veri dolaþacak.
		  dolayýsýyla shared memory içine paslanan deðerler de bu cinste olacak.
		* vector: redis karþýlýðý list
		* map:
		* field:
		  belli bir alandaki ya da alt alandaki tüm deðerleri
		  takip etmeyi saðlar. Belli bir konu baþlýðý olarak algýlanabileceði gibi struct olarak da
		  düþünülebilir. Örnek:
		  subscribe(Evren.Samanyolu.Gunes_Sistemi, subscribe_type::field);
*/



class Redis_Handler
{
public:
	Redis_Handler();
	~Redis_Handler();

	template <typename T1>
	void set_value(std::string const & key, T1 const & value, bool const & notification_enabled = true)
	{
		//std::cout << "set_value | key: " << key << " value: " << value << std::endl;

		std::ostringstream oss_value;
		std::string str_value;
		oss_value.precision(REDIS_PRECISION);
		oss_value << std::fixed << value;
		str_value = oss_value.str();
		client.set(key, str_value, [=](cpp_redis::reply& reply)
		{
			std::string str_reply = reply.as_string();
			//std::cout << str_reply << std::endl;
			//std::cout << typeid(T1).name() << std::endl;
			//std::cout << key << std::endl;
			//std::cout << "set '" << key << "' to " << typeid(T1).name() << " '" << str_value << "' | reply: " << str_reply << std::endl;
		}).commit();

		if (notification_enabled)
			add_notificaton(key, value);

		//client.sync_commit(std::chrono::milliseconds(50));
	}

	template <typename T1>
	void set_vector(std::string const & key, std::vector<T1> const & arg_vector)
	{
		std::cout << "set_vector : " << key << std::endl;

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
		client.rpush(key, multi_set_vector).commit();

		add_notificaton(key, arg_vector);
	}

	template <typename T1, typename T2>
	void set_map(std::string const & key, std::map<T1, T2> const & arg_map)
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
		client.hmset(key, multi_set_vector).commit;

		add_notificaton(key, arg_vector);
	}

	void delete_key(std::string const & key)
	{
		std::vector<std::string> delete_vector;
		delete_vector.push_back(key);
		client.del(delete_vector).commit();
	}

	template <typename OBJECT>
	void subscribe(std::string const & key, subscribe_type const & type, OBJECT shm_handler)
	{
		//std::cout << "subscribe | key: " << key << std::endl;
		// '*' karakteri kabul edilmeyecek
		//std::cout << " func : " << shm_handler->set_value(key,111.6) << std::endl;

		std::string str_key = "__key*__:" + key;

		if (type == field)
			str_key += "*";


		auto func_psubscribe_reply = [=](const std::string& chan, const std::string& msg) {

			std::string redis_key = chan.substr(chan.find(key)); /* client'ýn deðeri çekebilmesi için
																 key oluþturuluyor */

			std::cout << " redis_key : " << redis_key << " msg : " << msg << std::endl;


			if (msg.find("test_time") < msg.size() || msg.find("rpush") < msg.size() || msg.find("hmset") < msg.size() || msg.find("set") < msg.size())
				handle_subscriber_reply(redis_key, shm_handler);

		};

		subscriber.psubscribe(str_key, func_psubscribe_reply).commit();
	}

	template <typename OBJECT>
	void handle_subscriber_reply(std::string const & key, OBJECT shm_handler) /* subscriberdan gelen bildirim üzerine
																			   deðiþen alanýn tipini çeker ve içeriði
																			   çekecek method'u çaðýrýr */
	{
		auto process_type_reply = [=](cpp_redis::reply& reply)
		{
			std::string redis_type = reply.as_string();

			std::cout << "---- Subscriber Notification ----" << std::endl;
			std::cout << "redis_type : " << redis_type << std::endl;
			std::cout << "---- Subscriber Notification ----" << std::endl;

			process_to_shm(key, reply.as_string(), shm_handler);
		};

		client.type(key, process_type_reply).commit();
	}

	template <typename OBJECT>
	void process_to_shm(std::string const & key, std::string const & redis_type, OBJECT shm_handler)
	{/* TODO: Burasý daha az karmaþýk bir hale getirilebilir */

		auto process_get_reply = [=](cpp_redis::reply& reply)
		{
			std::string str_reply = reply.as_string();
			//std::cout << "get " << key << ": " << reply.as_string() << std::endl;

			double _value;

			try /* gelen deðeri double a çevirmeye çalýþýr. baþarýsýz olur ise string olduðunu farzeder */
			{
				_value = boost::lexical_cast<double>(str_reply);
				shm_handler->set_value(key, _value);
			}
			catch (const std::exception&)
			{
				shm_handler->set_value(key, str_reply);
			}

		};

		if (redis_type == "string")
		{
			client.get(key, process_get_reply).commit();
		}
		else if (redis_type == "list")
		{
			client.llen(key, [=](cpp_redis::reply& reply) {

				//std::cout << "llen : " << reply.as_integer() << std::endl;

				if (reply.as_integer() == 0)
				{
					std::cerr << " Notification Error: list is empty or Key does NOT exits!" << std::endl;
					return;
				}

				client.lrange(key, 0, reply.as_integer(), [=](cpp_redis::reply& reply) {

					auto _array = reply.as_array();

					std::cout << "array : " << _array << std::endl;

					try /* gelen deðeri double a çevirmeye çalýþýr. baþarýsýz olur ise string olduðunu farzeder */
					{
						std::vector<double> d_vector;

						for (auto it = _array.begin(); it != _array.end(); it++)
						{

							double d_val = boost::lexical_cast<double>(((cpp_redis::reply)*it).as_string());

							//std::cout << "d_val : " << d_val << std::endl;

							d_vector.push_back(d_val);

						}
						shm_handler->set_value(key, d_vector);
					}
					catch (const std::exception&)
					{
						std::vector<std::string> s_vector;

						for (auto it = _array.begin(); it != _array.end(); it++)
						{
							std::string s_val = ((cpp_redis::reply)*it).as_string();
							s_vector.push_back(s_val);

							//std::cout << "s_val : " << s_val << std::endl;

						}
						shm_handler->set_value(key, s_vector);
					}

				}).commit();

			}).commit();

		}
		else if (redis_type == "hash")
		{
			client.hgetall(key, [=](cpp_redis::reply& reply) {

				auto _array = reply.as_array();

				if (_array.size() <= 0)
				{
					std::cerr << " Notification Error: hash is empty or Key does NOT exits!" << std::endl;
					return;
				}

				//std::cout << "array : " << _array << std::endl;

				//for (auto it = _array.begin(); it != _array.end(); it++)
				//{
				//	std::cout << "_key : " << ((cpp_redis::reply)*it).as_string() << std::endl;
				//	it++;
				//	std::cout << "_val : " << ((cpp_redis::reply)*it).as_string() << std::endl;

				//	//d_vector.push_back(d_val);
				//}

				try /* gelen deðeri double a çevirmeye çalýþýr. baþarýsýz olur ise string olduðunu farzeder */
				{
					std::map<double, double> d_map;

					for (auto it = _array.begin(); it != _array.end(); it++)
					{

						double d_key = boost::lexical_cast<double>(((cpp_redis::reply)*it).as_string());
						it++;
						double d_val = boost::lexical_cast<double>(((cpp_redis::reply)*it).as_string());

						//std::cout << "d_val : " << d_val << std::endl;

						d_map[d_key] = d_val;

					}

					shm_handler->set_value(key, d_map);
				}
				catch (const std::exception&)
				{
					try
					{
						std::map<std::string, double> sd_map;

						for (auto it = _array.begin(); it != _array.end(); it++)
						{

							std::string s_key = ((cpp_redis::reply)*it).as_string();
							it++;
							double d_val = boost::lexical_cast<double>(((cpp_redis::reply)*it).as_string());

							//std::cout << "d_val : " << d_val << std::endl;

							sd_map[s_key] = d_val;
						}


						shm_handler->set_value(key, sd_map);
					}
					catch (const std::exception&)
					{

						std::map<std::string, std::string> s_map;

						for (auto it = _array.begin(); it != _array.end(); it++)
						{

							std::string s_key = ((cpp_redis::reply)*it).as_string();
							it++;
							std::string s_val = ((cpp_redis::reply)*it).as_string();

							//std::cout << "d_val : " << d_val << std::endl;

							s_map[s_key] = s_val;

						}

						shm_handler->set_value(key, s_map);
					}

				}

			}).commit();
		}
		else
		{
			std::cerr << " Notification Error: Unknown type!" << std::endl;
			return;
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

		std::cout << _type << std::endl;
		return _type;

	}

	template <typename T1>
	void add_notificaton(std::string const & key, T1 const & arg)
	{
		std::string _type = get_type(arg);

		client.publish(key, _type, [=](cpp_redis::reply& reply)
		{
			if (reply.is_string())
			{
				std::string str_reply = reply.as_string();
				std::cout << str_reply << std::endl;
			}
		}).commit();
	}


	std::string set_lock(std::string const & key, int time_out = 1000);

	bool release_lock(std::string const & key, std::string const & uuid);
	
	void client_commit(); // client'a verilen komutlarý server'a iþler
	void subscriber_commit(); // subscriber'a verilen komutlarý server'a iþler

protected:
	cpp_redis::redis_client client;
	cpp_redis::sync_client sync_client;
	cpp_redis::redis_subscriber subscriber;
};

