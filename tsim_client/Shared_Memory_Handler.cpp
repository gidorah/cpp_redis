#include "Shared_Memory_Handler.h"



Shared_Memory_Handler::Shared_Memory_Handler(std::string const & segment_name) :
	segment_name{ segment_name }
{
	std::string str_segment = "Redis_Shared_Memory"; /* shared memory ismi argüman olarak girebilmeli.
													 Yoksa yazılımlar arası bir karışıklık olabilir. */

	std::string incoming_notification_key; /* master ya da slave olma durumuna göre shared memory üze+rinde tutulacak
										   notifikasyon alan isimlerini belirler */

	std::string outgoing_notification_key; /* master ya da slave olma durumuna göre shared memory üze+rinde tutulacak
										   notifikasyon alan isimlerini belirler */

	std::cout << "segment_name : " << segment_name << std::endl;

	try /*memory bloğuna open_only ile ulaşmaya çalıştığımızda başarısız olduğunda, yani bulamadığında throw eder.
		Buradaki yapı bunu kullanarak bir master-slave ilişkisi kurar. Bu sayede karşılıklı notification gönderme
		kısımları daha dinamik hale geldi. */
	{
		segment = new Shared::segment(bip::open_only, segment_name.data());

		std::cout << "break my chains and set me free!" << std::endl;

		incoming_notification_key = "master_notification";
		outgoing_notification_key = "slave_notification";		

		master = false;
	}
	catch (const std::exception&)
	{
		segment = new Shared::segment(bip::create_only, segment_name.data(), MEMORY_SIZE);

		std::cout << "Now. I, Skeletor, am Master of the Universe!" << std::endl;

		incoming_notification_key = "slave_notification";
		outgoing_notification_key = "master_notification";

		master = true;
	}

	std::string incoming_notification_key_data = incoming_notification_key + "_data";
	std::string outgoing_notification_key_data = outgoing_notification_key + "_data";
	std::string incoming_notification_key_rpc = incoming_notification_key + "_rpc";
	std::string outgoing_notification_key_rpc = outgoing_notification_key + "_rpc";

	segment_manager = segment->get_segment_manager();

	Shared::void_allocator alloc_inst(segment_manager);

	incoming_data_notifications = NULL;

	incoming_data_notifications =
		segment->find<Shared::sh_data_notification_map>(incoming_notification_key_data.data()).first;

	if (!incoming_data_notifications)
	{
		incoming_data_notifications =
			segment->construct<Shared::sh_data_notification_map>(incoming_notification_key_data.data())(std::less<Shared::string>(), alloc_inst);
	}

	outgoing_data_notifications = NULL;

	outgoing_data_notifications =
		segment->find<Shared::sh_data_notification_map>(outgoing_notification_key_data.data()).first;

	if (!outgoing_data_notifications)
	{
		outgoing_data_notifications =
			segment->construct<Shared::sh_data_notification_map>(outgoing_notification_key_data.data())(std::less<Shared::string>(), alloc_inst);
	}

	incoming_rpc_notifications = NULL;

	incoming_rpc_notifications =
		segment->find<Shared::sh_rpc_notification_map>(incoming_notification_key_rpc.data()).first;

	if (!incoming_rpc_notifications)
	{
		incoming_rpc_notifications =
			segment->construct<Shared::sh_rpc_notification_map>(incoming_notification_key_rpc.data())(std::less<Shared::string>(), alloc_inst);
	}

	outgoing_rpc_notifications = NULL;

	outgoing_rpc_notifications =
		segment->find<Shared::sh_rpc_notification_map>(outgoing_notification_key_rpc.data()).first;

	if (!outgoing_rpc_notifications)
	{
		outgoing_rpc_notifications =
			segment->construct<Shared::sh_rpc_notification_map>(outgoing_notification_key_rpc.data())(std::less<Shared::string>(), alloc_inst);
	}

	std::cout << "shm thread id : " << std::this_thread::get_id() << "\n";
}


Shared_Memory_Handler::~Shared_Memory_Handler()
{
}

void Shared_Memory_Handler::add_data_notification(std::string const & key, std::string const & type)
{
	bip::named_mutex shm_mutex{ bip::open_or_create,  str_notification_lock.c_str() };
	shm_mutex.lock();

	Shared::void_allocator alloc_inst(segment_manager);
	Shared::Sh_Data_Notification_Struct _notification(key, type, alloc_inst);
	Shared::string  key_object(key.c_str(), alloc_inst);
	Shared::sh_data_notification_pair _pair(key_object, _notification);

	outgoing_data_notifications->insert(_pair);

	shm_mutex.unlock();
}

int Shared_Memory_Handler::get_data_notification_queue_size()
{
	bip::named_mutex shm_mutex{ bip::open_or_create, str_notification_lock.c_str() };
	shm_mutex.lock();

	int _size = incoming_data_notifications->size();

	shm_mutex.unlock();

	return _size;
}

Shared::Data_Notification_Struct Shared_Memory_Handler::pop_data_notification()
{
	bip::named_mutex shm_mutex{ bip::open_or_create, str_notification_lock.c_str() };
	shm_mutex.lock();

	Shared::Data_Notification_Struct return_struct;

	if (incoming_data_notifications->begin() != incoming_data_notifications->end())
	{
		Shared::sh_data_notification_map::iterator it = incoming_data_notifications->begin();

		//std::cout << it->first << std::endl;

		return_struct._key = it->second._key.c_str();
		return_struct._type = it->second._type.c_str();

		incoming_data_notifications->erase(incoming_data_notifications->begin());
	}
	else
	{
		std::cerr << " incoming_data_notifications vector is empty!" << std::endl;
	}

	shm_mutex.unlock();

	return return_struct;
}

void Shared_Memory_Handler::add_rpc_notification(std::string const & uuid, std::string const & method_name, Shared::rpc_notification_type type)
{
	bip::named_mutex shm_mutex{ bip::open_or_create,  str_notification_lock.c_str() };
	shm_mutex.lock();

	Shared::void_allocator alloc_inst(segment_manager);
	Shared::Sh_Rpc_Notification_Struct _notification(uuid, method_name, type, alloc_inst);
	Shared::string  key_object(uuid.c_str(), alloc_inst);
	Shared::sh_rpc_notification_pair _pair(key_object, _notification);

	outgoing_rpc_notifications->insert(_pair);

	shm_mutex.unlock();
}

int Shared_Memory_Handler::get_rpc_notification_queue_size()
{
	bip::named_mutex shm_mutex{ bip::open_or_create,  str_notification_lock.c_str() };
	shm_mutex.lock();

	int _size = incoming_rpc_notifications->size();

	shm_mutex.unlock();

	return _size;
}

Shared::Rpc_Notification_Struct Shared_Memory_Handler::pop_rpc_notification()
{
	bip::named_mutex shm_mutex{ bip::open_or_create,  str_notification_lock.c_str() };
	shm_mutex.lock();

	Shared::Rpc_Notification_Struct return_struct;

	if (incoming_rpc_notifications->begin() != incoming_rpc_notifications->end())
	{
		Shared::sh_rpc_notification_map::iterator it = incoming_rpc_notifications->begin();

		return_struct._uuid = it->second._uuid.c_str();
		return_struct._method_id = it->second._method_id.c_str();
		return_struct._type = it->second._type;

		incoming_rpc_notifications->erase(incoming_rpc_notifications->begin());
	}
	else
	{
		std::cerr << " incoming_rpc_notifications vector is empty!" << std::endl;
	}

	shm_mutex.unlock();

	return return_struct;
}

