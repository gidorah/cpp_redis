#pragma once

#define BOOST_DATE_TIME_NO_LIB

#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <string>
#include <cstdlib> //std::system
#include <typeinfo>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include <queue>
#include <future>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>


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

	/*------------------------------------data notification vector-----------------------------------------------*/
	typedef struct
	{
		std::string _key; /* değişikliğin vuku bulduğu key */
		std::string _type; /* key'in tanımladığı değişkenin tipi */

	}Data_Notification_Struct;

	class Sh_Data_Notification_Struct
	{
	public:
		string _key; /* değişikliğin vuku bulduğu key */
		string _type; /* key'in tanımladığı değişkenin tipi */

		Sh_Data_Notification_Struct(const void_allocator &void_alloc)
			: _key(void_alloc),
			_type(void_alloc)
		{}

		Sh_Data_Notification_Struct(Data_Notification_Struct const &original, const void_allocator &void_alloc)
			: _key(original._key.c_str(), void_alloc),
			_type(original._type.c_str(), void_alloc)
		{}

		Sh_Data_Notification_Struct(std::string const &original_key, std::string const &original_type, const void_allocator &void_alloc)
			: _key(original_key.c_str(), void_alloc),
			_type(original_type.c_str(), void_alloc)
		{}
	};

	typedef std::pair<const string, Sh_Data_Notification_Struct>            sh_data_notification_pair;
	typedef std::pair<string, Sh_Data_Notification_Struct>                  movable_sh_data_notification_pair;
	typedef bip::allocator<sh_data_notification_pair, segment_manager>      sh_data_notification_allocator;
	typedef bip::map< string, Sh_Data_Notification_Struct
		, std::less<string>, sh_data_notification_allocator>              sh_data_notification_map;
	/*------------------------------------data notification vector-----------------------------------------------*/

	/*------------------------------------RPC notification vector-----------------------------------------------*/

	enum rpc_notification_type
	{
		method_call = 0,
		reply = 1
	};

	typedef struct
	{
		std::string _uuid; /* remote method için unique id */
		std::string _method_id; /* remote methodun adı */
		rpc_notification_type _type; /* gelen notification tipi */

	}Rpc_Notification_Struct;

	class Sh_Rpc_Notification_Struct
	{
	public:
		string _uuid; /* remote method için unique id */
		string _method_id; /* remote methodun adı */
		rpc_notification_type _type; /* gelen notification tipi */

		Sh_Rpc_Notification_Struct(const void_allocator &void_alloc)
			: _uuid(void_alloc),
			_method_id(void_alloc)
		{}

		Sh_Rpc_Notification_Struct(Rpc_Notification_Struct const &original, const void_allocator &void_alloc)
			: _uuid(original._uuid.c_str(), void_alloc),
			_method_id(original._method_id.c_str(), void_alloc),
			_type(original._type)
		{}

		Sh_Rpc_Notification_Struct(std::string const &original_uuid, std::string const &original_method_id, rpc_notification_type const &original_type, const void_allocator &void_alloc)
			: _uuid(original_uuid.c_str(), void_alloc),
			_method_id(original_method_id.c_str(), void_alloc),
			_type(original_type)
		{}
	};

	typedef std::pair<const string, Sh_Rpc_Notification_Struct>            sh_rpc_notification_pair;
	typedef std::pair<string, Sh_Rpc_Notification_Struct>                  movable_sh_rpc_notification_pair;
	typedef bip::allocator<sh_rpc_notification_pair, segment_manager>      sh_rpc_notification_allocator;
	typedef bip::map< string, Sh_Rpc_Notification_Struct
		, std::less<string>, sh_rpc_notification_allocator>              sh_rpc_notification_map;
	/*------------------------------------RPC notification vector-----------------------------------------------*/
}

//static inline int getMilliCount() {
//	timeb tb;
//	ftime(&tb);
//	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
//	return nCount;
//}

class Shared_Memory_Handler
{
protected:

	std::string str_lock{".##LOCK##"};

	Shared::segment *segment;
	Shared::segment_manager *segment_manager;

	Shared::sh_data_notification_map *incoming_data_notifications;
	Shared::sh_data_notification_map *outgoing_data_notifications;

	Shared::sh_rpc_notification_map *incoming_rpc_notifications;
	Shared::sh_rpc_notification_map *outgoing_rpc_notifications;


	using shmh = Shared_Memory_Handler;
	using result_type = void();

	typedef std::function<result_type> reply_callback_t;
	using call_t = std::function<shmh&(const reply_callback_t&)>;

	std::map<std::string, reply_callback_t> m_callbacks;
	std::mutex m_callbacks_mutex;
	std::condition_variable m_sync_condvar;

public:

	bool master;
	double test_count{0};

	Shared_Memory_Handler(std::string const & segment_name = "Redis_Shared_Memory");
	~Shared_Memory_Handler();

	/*---------------------------------------------------------------------------------------------------------------*/
	template <typename T1>
	void set_value(std::string const & key, T1 const & arg_value, bool const &disable_notification = false)
	{	
		T1 *_var = segment->find_or_construct<T1>(key.c_str())(arg_value);
		*_var = arg_value;

		if(disable_notification == false) add_data_notification(key.c_str(), typeid(T1).name());
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
		}
		else
		{
			throw std::invalid_argument(key);
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	void set_value(std::string const & key, std::string const & arg_str, bool const &disable_notification = false)
	{
		int uuid = set_lock(key);

		if (uuid)
		{
			Shared::string sh_str_value(segment_manager);
			sh_str_value = arg_str.c_str();

			Shared::string *_var = segment->find_or_construct<Shared::string>(key.c_str())(sh_str_value);
			*_var = sh_str_value;

			release_lock(key, uuid);

			if(disable_notification == false) add_data_notification(key.c_str(), typeid(std::string).name());
		}
	}

	void get_value(std::string const & key, std::string & return_string) /* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
																		 invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
																		 kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{
		int uuid = set_lock(key);

		if (uuid)
		{
			std::pair<Shared::string *, size_t > _pair = segment->find<Shared::string>(key.c_str());

			if (_pair.second > 0)
			{
				Shared::string _sh_str(segment_manager);

				_sh_str = *_pair.first;
				return_string = _sh_str.data();
				release_lock(key, uuid);
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
	void set_value(std::string const & key, std::vector<T1> const & arg_vector, bool const &disable_notification = false)
	{
		typedef bip::allocator<T1, Shared::segment_manager> sh_allocator;
		typedef bip::vector<T1, sh_allocator> sh_vector;

		int uuid = set_lock(key);

		if (uuid)
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
			}

			release_lock(key, uuid);

			if(disable_notification == false) add_data_notification(key.c_str(), typeid(std::vector<T1>).name());
		}

	}

	template <typename T1>
	void get_value(std::string const & key, std::vector<T1> & return_vector)/* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
																			invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
																			kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{
		typedef bip::allocator<T1, Shared::segment_manager> sh_allocator;
		typedef bip::vector<T1, sh_allocator> sh_vector;

		int uuid = set_lock(key);

		if (uuid)
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

				release_lock(key, uuid);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	void set_value(std::string const & key, std::vector<std::string> const & arg_vector, bool const &disable_notification = false)
	{
		typedef bip::allocator<Shared::string, Shared::segment_manager> sh_allocator;
		typedef bip::vector<Shared::string, sh_allocator> sh_vector;

		int uuid = set_lock(key);

		if (uuid)
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

			release_lock(key, uuid);

			if(disable_notification == false) add_data_notification(key.c_str(), typeid(std::vector<std::string>).name());
		}
	}

	void get_value(std::string const & key, std::vector<std::string> & return_vector)/* Eğer çekmek istediğimiz key shared memory üzerinde bulunmuyorsa
																					 invalid_argument exception'ı throw ediyor. Dolayısıyla try-catch ile
																					 kullanılması gerek. Aksi takdirde yazılım çakabilir!*/
	{
		typedef bip::allocator<Shared::string, Shared::segment_manager> sh_allocator;
		typedef bip::vector<Shared::string, sh_allocator> sh_vector;

		int uuid = set_lock(key);

		if (uuid)
		{
			sh_vector *shm_vector = NULL;
			shm_vector = segment->find<sh_vector>(key.data()).first;

			if (shm_vector && shm_vector->size() > 0)
			{

				for (sh_vector::iterator it = shm_vector->begin(); it != shm_vector->end(); it++)
				{
					return_vector.push_back((*it).c_str());
				}

				release_lock(key, uuid);
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
	void set_value(std::string const & key, std::map<T1, T2> const & arg_map, bool const &disable_notification = false)
	{
		typedef std::pair<const T1, T2> map_value_type;
		typedef std::pair<T1, T2> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< T1, T2, std::less<T1>, map_value_type_allocator> sh_map;


		int uuid = set_lock(key);

		if (uuid)
		{
			sh_map *shm_map = segment->find<sh_map>(key.data()).first;

			if (!shm_map)
			{
				shm_map = segment->construct<sh_map>(key.data())(std::less<T1>(), segment_manager);
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

			release_lock(key, uuid);

			if(disable_notification == false) add_data_notification(key.c_str(), typeid(std::map<T1, T2>).name());
		}
	}

	template <typename T1, typename T2>
	void get_value(std::string const & key, std::map<T1, T2> & arg_map)
	{
		typedef std::pair<const T1, T2> map_value_type;
		typedef std::pair<T1, T2> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< T1, T2, std::less<T1>, map_value_type_allocator> sh_map;

		int uuid = set_lock(key);

		if (uuid)
		{
			sh_map *shm_map = segment->find<sh_map>(key.data()).first;

			if (shm_map)
			{
				for (sh_map::iterator it = shm_map->begin(); it != shm_map->end(); it++)
				{
					T1 map_key;
					T2 map_val;

					map_key = (*it).first;
					map_val = (*it).second;
					arg_map[map_key] = map_val;
				}

				release_lock(key, uuid);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}
	}
	/*---------------------------------------------------------------------------------------------------------------*/


	/*---------------------------------------------------------------------------------------------------------------*/
	void set_value(std::string const & key, std::map<std::string, std::string> const & arg_map, bool const &disable_notification = false)
	{
		typedef std::pair<const Shared::string, Shared::string> map_value_type;
		typedef std::pair<Shared::string, Shared::string> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, Shared::string, std::less<Shared::string>, map_value_type_allocator> sh_map;

		int uuid = set_lock(key);

		if (uuid)
		{
			sh_map *shm_map = segment->find<sh_map>(key.data()).first;

			if (!shm_map)
			{
				shm_map = segment->construct<sh_map>(key.data())(std::less<Shared::string>(), segment_manager);
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

			release_lock(key, uuid);

			if(disable_notification == false) add_data_notification(key.c_str(), typeid(std::map<std::string, std::string>).name());
		}
	}

	void get_value(std::string const & key, std::map<std::string, std::string> & arg_map)
	{
		typedef std::pair<const Shared::string, Shared::string> map_value_type;
		typedef std::pair<Shared::string, Shared::string> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, Shared::string, std::less<Shared::string>, map_value_type_allocator> sh_map;

		int uuid = set_lock(key);

		if (uuid)
		{
			sh_map *shm_map = segment->find<sh_map>(key.data()).first;

			if (shm_map)
			{
				for (sh_map::iterator it = shm_map->begin(); it != shm_map->end(); it++)
				{
					std::string map_key;
					std::string map_val;

					map_key = (*it).first.c_str();
					map_val = (*it).second.c_str();
					arg_map[map_key] = map_val;					
				}

				release_lock(key, uuid);
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
	void set_value(std::string const & key, std::map<std::string, T1> const & arg_map, bool const &disable_notification = false)
	{
		typedef std::pair<const Shared::string, T1> map_value_type;
		typedef std::pair<Shared::string, T1> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, T1, std::less<Shared::string>, map_value_type_allocator> sh_map;

		int uuid = set_lock(key);

		if (uuid)
		{
			sh_map *shm_map = segment->find<sh_map>(key.data()).first;

			if (!shm_map)
			{
				shm_map = segment->construct<sh_map>(key.data())(std::less<Shared::string>(), segment_manager);
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

			release_lock(key, uuid);

			if(disable_notification == false) add_data_notification(key.c_str(), typeid(std::map<std::string, T1>).name());
		}

	}

	template <typename T1>
	void get_value(std::string const & key, std::map<std::string, T1> & arg_map)
	{
		typedef std::pair<const Shared::string, T1> map_value_type;
		typedef std::pair<Shared::string, T1> movable_to_map_value_type;
		typedef bip::allocator<map_value_type, Shared::segment_manager> map_value_type_allocator;
		typedef bip::map< Shared::string, T1, std::less<Shared::string>, map_value_type_allocator> sh_map;

		int uuid = set_lock(key);

		if (uuid)
		{
			sh_map *shm_map = segment->find<sh_map>(key.data()).first;

			if (shm_map)
			{
				for (sh_map::iterator it = shm_map->begin(); it != shm_map->end(); it++)
				{
					std::string map_key;
					T1 map_val;

					map_key = (*it).first.c_str();
					map_val = (*it).second;
					arg_map[map_key] = map_val;
				}

				release_lock(key, uuid);
			}
			else
			{
				throw std::invalid_argument(key);
			}
		}

	}
	/*---------------------------------------------------------------------------------------------------------------*/

	void add_data_notification(std::string const & key, std::string const & type);

	int get_data_notification_queue_size();

	Shared::Data_Notification_Struct pop_data_notification();

	void add_rpc_notification(std::string const &uuid, std::string const &method_name, Shared::rpc_notification_type type);

	int get_rpc_notification_queue_size();

	Shared::Rpc_Notification_Struct pop_rpc_notification();	

	int set_lock(std::string const & key, int time_out = 1000)
	{
		int uuid = (rand() % INT_MAX) + 1;
		int interval = 10; // msec

		std::string _key = "##LOCK##:" + key;

		while ((time_out) > 0)
		{
			time_out -= interval;

			int *lock = segment->find<int>(_key.data()).first;

			if (lock == NULL)
			{
				//std::cout << key << " : lock set!" << std::endl;
				lock = segment->construct<int>(_key.data())(uuid);
				return uuid;
			}
			else
			{
				//std::cout << key << " : lock failed trying again... | " << lock << " - " << uuid << std::endl;
				boost::this_thread::sleep_for(boost::chrono::milliseconds(interval));
				//Sleep(interval);
			}
		}

		std::cout << key << " : Shared_Memory_Handler lock failed!" << std::endl;
		return 0;
	}

	bool release_lock(std::string const & key, int const & uuid)
	{
		std::string _key = "##LOCK##:" + key;
		bool result = false;

		int *lock = segment->find<int>(_key.data()).first;

		if (lock != NULL && *lock == uuid)
		{
			result = segment->destroy<int>(_key.data());
		}

		//std::cout << key << " : release_lock : " << result << std::endl;


		return result;
	}

	std::string push_remote_call(std::string const &method_name, const reply_callback_t& callback = nullptr)
	{
		boost::uuids::uuid uuid;
		std::string str_uuid;

		do /*Aynı key'e sahip bir işlemi sıraya koyma olasılığını ortadan kaldırmak için*/
		{
			uuid = boost::uuids::random_generator()();
			str_uuid = boost::lexical_cast<std::string>(uuid);
		} while (m_callbacks.find(str_uuid) != m_callbacks.end());

		m_callbacks[str_uuid] = callback;
		add_rpc_notification(str_uuid, method_name, Shared::rpc_notification_type::method_call);

		return str_uuid;
	}

	void synced_remote_call(std::string const &method_name, const reply_callback_t& callback = nullptr)
	{
		std::string uuid = push_remote_call(method_name, callback);

		std::unique_lock<std::mutex> lock_callback(m_callbacks_mutex);
		m_sync_condvar.wait(lock_callback, [&] { return m_callbacks.empty(); });
	}

	bool handle_remote_call_reply(std::string reply_uuid)
	{
		reply_callback_t callback = nullptr;

		{
			if (m_callbacks.find(reply_uuid) != m_callbacks.end()) {
				callback = m_callbacks[reply_uuid];
				m_callbacks.erase(reply_uuid);
			}
		}

		if (callback) {
			callback();
			m_sync_condvar.notify_all();
			return true;
		}

		m_sync_condvar.notify_all();
		return false;
	}

};
