#pragma once

#define BOOST_DATE_TIME_NO_LIB

#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>
#include <cstdlib> //std::system
#include <typeinfo>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
//#include <typeindex>

#define MEMORY_SIZE 67108864 /* shared memory için kullanılacak belleğin boyutu (byte) */

namespace bip = boost::interprocess;

namespace Shared {
	/*-------------------------------------generic Shared definitions---------------------------------------*/
	typedef bip::managed_windows_shared_memory segment;
	typedef segment::segment_manager segment_manager;

	typedef bip::allocator<void, segment_manager> void_allocator;
	typedef bip::allocator<char, segment_manager> char_allocator;
	typedef bip::basic_string<char, std::char_traits<char>, char_allocator> string;
	/*-------------------------------------generic Shared definitions---------------------------------------*/

	/*------------------------------------notification vector-----------------------------------------------*/

	typedef struct
	{
		std::string _key; /* değişikliğin vuku bulduğu key */
		std::string _type; /* key'in tanımladığı değişkenin tipi */

	}Notification_Struct;

	class Sh_Notification_Struct
	{
	public:
		string _key; /* değişikliğin vuku bulduğu key */
		string _type; /* key'in tanımladığı değişkenin tipi */

		Sh_Notification_Struct(const void_allocator &void_alloc)
			: _key(void_alloc),
			_type(void_alloc)
		{}

		Sh_Notification_Struct(Notification_Struct const &original, const void_allocator &void_alloc)
			: _key(original._key.c_str(), void_alloc),
			_type(original._type.c_str(), void_alloc)
		{}

		Sh_Notification_Struct(std::string const &original_key, std::string const &original_type, const void_allocator &void_alloc)
			: _key(original_key.c_str(), void_alloc),
			_type(original_type.c_str(), void_alloc)
		{}
	};

	typedef std::pair<const string, Sh_Notification_Struct>            sh_notification_pair;
	typedef std::pair<string, Sh_Notification_Struct>                  movable_sh_notification_pair;
	typedef bip::allocator<sh_notification_pair, segment_manager>      sh_notification_allocator;
	typedef bip::map< string, Sh_Notification_Struct
		, std::less<string>, sh_notification_allocator>              sh_notification_map;
	/*------------------------------------notification vector-----------------------------------------------*/
}

static inline int getMilliCount() {
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}

class Shared_Memory_Handler
{
public:

	bool master;
	double test_count = 0;
	Shared::segment *segment;
	Shared::segment_manager *segment_manager;

	Shared_Memory_Handler(std::string const & segment_name = "Redis_Shared_Memory");
	~Shared_Memory_Handler();

	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	void set_value(std::string const & key, T1 const & arg_value)
	{
		std::cout << key << " : " << arg_value << std::endl;

		T1 *_var = segment->find_or_construct<T1>(key.c_str())(arg_value);
		*_var = arg_value;

		add_notification(key.c_str(), typeid(T1).name());
	}

	template <typename T1>
	void get_value(std::string const & key, T1 & return_value) /* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
															   invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
															   kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{
		T1 *_result = segment->find<T1>(key.c_str()).first;

		if (_result)
		{
			return_value = *_result;
			segment->destroy<T1>(key.c_str());
		}
		else
		{
			throw std::invalid_argument(key);
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	void set_value(std::string const & key, std::string const & arg_str)
	{
		if (set_lock(key))
		{
			Shared::string sh_str_value(segment_manager);
			sh_str_value = arg_str.c_str();

			Shared::string *_var = segment->find_or_construct<Shared::string>(key.c_str())(sh_str_value);
			*_var = sh_str_value;

			release_lock(key);

			std::cout << "\nset_string : " << key << std::endl;

			add_notification(key.c_str(), typeid(std::string).name());
		}
	}

	void get_value(std::string const & key, std::string & return_string) /* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
																		 invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
																		 kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{
		if (set_lock(key))
		{
			std::pair<Shared::string *, size_t > _pair = segment->find<Shared::string>(key.c_str());

			if (_pair.second > 0)
			{
				Shared::string _sh_str(segment_manager);

				_sh_str = *_pair.first;
				return_string = _sh_str.data();
				segment->destroy<Shared::string>(key.c_str());
				release_lock(key);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	void set_value(std::string const & key, std::vector<T1> const & arg_vector)
	{
		typedef bip::allocator<T1, Shared::segment_manager> sh_allocator;
		typedef bip::vector<T1, sh_allocator> sh_vector;

		if (set_lock(key))
		{
			sh_vector *shm_vector = NULL;
			shm_vector = segment->find<sh_vector>(key.data()).first;

			if (!shm_vector)
			{
				shm_vector = segment->construct<sh_vector>(key.data())(segment_manager);
			}

			shm_vector->clear();

			for (std::vector<T1>::const_iterator it = arg_vector.begin(); it != arg_vector.end(); it++)
			{
				shm_vector->push_back(*it);
				std::cout << "it : " << *it << std::endl;
			}

			release_lock(key);

			add_notification(key.c_str(), typeid(std::vector<T1>).name());
		}

	}

	template <typename T1>
	void get_value(std::string const & key, std::vector<T1> & return_vector)/* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
																			invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
																			kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{
		typedef bip::allocator<T1, Shared::segment_manager> sh_allocator;
		typedef bip::vector<T1, sh_allocator> sh_vector;

		if (set_lock(key))
		{
			sh_vector shm_vector(segment_manager);

			std::pair<sh_vector *, size_t > _result = segment->find<sh_vector>(key.data());

			if (_result.second > 0)
			{
				shm_vector = *_result.first;

				for (sh_vector::iterator it = shm_vector.begin(); it != shm_vector.end(); it++)
				{
					return_vector.push_back(*it);
				}

				segment->destroy<sh_vector>(key.c_str());
				release_lock(key);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	void set_value(std::string const & key, std::vector<std::string> const & arg_vector)
	{
		typedef bip::allocator<Shared::string, Shared::segment_manager> sh_allocator;
		typedef bip::vector<Shared::string, sh_allocator> sh_vector;

		if (set_lock(key))
		{
			sh_vector *shm_vector = NULL;
			shm_vector = segment->find<sh_vector>(key.data()).first;

			if (!shm_vector)
			{
				shm_vector = segment->construct<sh_vector>(key.data())(segment_manager);
			}

			shm_vector->clear();

			for (std::vector<std::string>::const_iterator it = arg_vector.begin(); it != arg_vector.end(); it++)
			{
				Shared::string sh_str((*it).c_str(), segment_manager);
				shm_vector->push_back(sh_str);
			}

			release_lock(key);

			add_notification(key.c_str(), typeid(std::vector<std::string>).name());
		}
	}

	void get_value(std::string const & key, std::vector<std::string> & return_vector)/* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
																					 invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
																					 kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{

		typedef bip::allocator<Shared::string, Shared::segment_manager> sh_allocator;
		typedef bip::vector<Shared::string, sh_allocator> sh_vector;

		if (set_lock(key))
		{
			sh_vector *shm_vector = NULL;
			shm_vector = segment->find<sh_vector>(key.data()).first;

			if (shm_vector && shm_vector->size() > 0)
			{

				for (sh_vector::iterator it = shm_vector->begin(); it != shm_vector->end(); it++)
				{
					return_vector.push_back((*it).c_str());
				}

				segment->destroy<sh_vector>(key.c_str());
				release_lock(key);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1, typename T2>
	void set_value(std::string const & key, std::map<T1, T2> const & arg_map)
	{
		typedef std::pair<const T1, T2> map_value_type;
		typedef std::pair<T1, T2> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< T1, T2, std::less<T1>, map_value_type_allocator> sh_str_map;


		if (set_lock(key))
		{
			sh_str_map *shm_map = segment->find<sh_str_map>(key.data()).first;

			if (!shm_map)
			{
				shm_map = segment->construct<sh_str_map>(key.data())(std::less<T1>(), segment_manager);
			}

			for (std::map<T1, T2>::const_iterator it = arg_map.begin(); it != arg_map.end(); it++)
			{
				T1 map_key;
				T2 map_val;

				map_key = (*it).first;
				map_val = (*it).second;

				map_value_type _pair(map_key, map_val);

				shm_map->insert(_pair);
			}

			release_lock(key);

			add_notification(key.c_str(), typeid(std::map<T1, T2>).name());
		}
	}

	template <typename T1, typename T2>
	void get_value(std::string const & key, std::map<T1, T2> & arg_map)
	{
		typedef std::pair<const T1, T2> map_value_type;
		typedef std::pair<T1, T2> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< T1, T2, std::less<T1>, map_value_type_allocator> sh_str_map;

		if (set_lock(key))
		{
			sh_str_map *shm_map = segment->find<sh_str_map>(key.data()).first;

			if (shm_map)
			{
				for (sh_str_map it = shm_map->begin(); it != shm_map->end(); it++)
				{
					T1 map_key;
					T2 map_val;

					map_key = (*it).first;
					map_val = (*it).second;

					//std::cout << "map_key : " << map_key << " | map_val : " << map_val << std::endl;

					arg_map[map_key] = map_val;
				}

				segment->destroy<sh_str_map>(key.c_str());
				release_lock(key);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}
	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	void set_value(std::string const & key, std::map<std::string, std::string> const & arg_map)
	{
		typedef std::pair<const Shared::string, Shared::string> map_value_type;
		typedef std::pair<Shared::string, Shared::string> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, Shared::string, std::less<Shared::string>, map_value_type_allocator> sh_str_map;

		if (set_lock(key))
		{
			sh_str_map *shm_map = segment->find<sh_str_map>(key.data()).first;

			if (!shm_map)
			{
				shm_map = segment->construct<sh_str_map>(key.data())(std::less<Shared::string>(), segment_manager);
			}

			for (std::map<std::string, std::string>::const_iterator it = arg_map.begin(); it != arg_map.end(); it++)
			{
				Shared::string map_key(segment_manager);
				Shared::string map_val(segment_manager);

				map_key = (*it).first.c_str();
				map_val = (*it).second.c_str();

				map_value_type _pair(map_key, map_val);

				shm_map->insert(_pair);
			}

			release_lock(key);

			add_notification(key.c_str(), typeid(std::map<std::string, std::string>).name());
		}
	}

	void get_value(std::string const & key, std::map<std::string, std::string> & arg_map)
	{
		typedef std::pair<const Shared::string, Shared::string> map_value_type;
		typedef std::pair<Shared::string, Shared::string> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, Shared::string, std::less<Shared::string>, map_value_type_allocator> sh_str_map;

		if (set_lock(key))
		{
			sh_str_map *shm_map = segment->find<sh_str_map>(key.data()).first;

			if (shm_map)
			{
				for (sh_str_map::iterator it = shm_map->begin(); it != shm_map->end(); it++)
				{
					std::string map_key;
					std::string map_val;

					map_key = (*it).first.c_str();
					map_val = (*it).second.c_str();

					test_count++;


					//std::cout << "map_key : " << map_key << " | map_val : " << map_val << std::endl;

					arg_map[map_key] = map_val;
				}

				segment->destroy<sh_str_map>(key.c_str());
				release_lock(key);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	void set_value(std::string const & key, std::map<std::string, T1> const & arg_map)
	{
		typedef std::pair<const Shared::string, T1> map_value_type;
		typedef std::pair<Shared::string, T1> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, T1, std::less<Shared::string>, map_value_type_allocator> sh_str_map;

		if (set_lock(key))
		{
			sh_str_map *shm_map = segment->find<sh_str_map>(key.data()).first;

			if (!shm_map)
			{
				shm_map = segment->construct<sh_str_map>(key.data())(std::less<Shared::string>(), segment_manager);
			}

			for (std::map<std::string, T1>::const_iterator it = arg_map.begin(); it != arg_map.end(); it++)
			{
				Shared::string map_key(segment_manager);
				T1 map_val;

				map_key = (*it).first.c_str();
				map_val = (*it).second;

				map_value_type _pair(map_key, map_val);

				shm_map->insert(_pair);
			}

			release_lock(key);

			add_notification(key.c_str(), typeid(std::map<std::string, T1>).name());
		}

	}

	template <typename T1>
	void get_value(std::string const & key, std::map<std::string, T1> & arg_map)
	{
		typedef std::pair<const Shared::string, T1> map_value_type;
		typedef std::pair<Shared::string, T1> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, T1, std::less<Shared::string>, map_value_type_allocator> sh_str_map;

		if (set_lock(key))
		{
			sh_str_map *shm_map = segment->find<sh_str_map>(key.data()).first;

			if (shm_map)
			{
				for (sh_str_map::iterator it = shm_map->begin(); it != shm_map->end(); it++)
				{
					std::string map_key;
					T1 map_val;

					map_key = (*it).first.c_str();
					map_val = (*it).second;

					//std::cout << "map_key : " << map_key << " | map_val : " << map_val << std::endl;

					arg_map[map_key] = map_val;
				}

				segment->destroy<sh_str_map>(key.c_str());
				release_lock(key);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/

	void add_notification(std::string const & key, std::string const & type);

	int get_notification_queue_size();

	Shared::Notification_Struct pop_notification();

	std::string str_lock;

	bool set_lock(std::string const & key)
	{
		std::string lock_key = key + str_lock;

		bool *lock = segment->find<bool>(lock_key.data()).first;

		if (lock != NULL && *lock == false)
		{
			*lock = true;
			return true;
		}
		else if (lock == NULL)
		{
			lock = segment->construct<bool>(lock_key.data())(segment_manager);
			*lock = true;
			return true;
		}
		else
		{
			std::cout << "Couldn't set the lock!" << std::endl;
			return false;
		}
	}

	bool release_lock(std::string const & key)
	{
		std::string lock_key = key + str_lock;

		bool *lock = segment->find<bool>(lock_key.data()).first;

		if (lock != NULL)
		{
			*lock = false;
			return true;
		}
		else
		{
			lock = segment->construct<bool>(lock_key.data())(segment_manager);
			*lock = false;
			return true;
		}
	}

private:

	Shared::sh_notification_map *incoming_notifications;
	Shared::sh_notification_map *outgoing_notifications;
};