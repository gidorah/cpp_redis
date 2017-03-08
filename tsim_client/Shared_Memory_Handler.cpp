#include "Shared_Memory_Handler.h"



Shared_Memory_Handler::Shared_Memory_Handler(std::string const & segment_name) :
	str_lock(".##LOCK##")
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

	segment_manager = segment->get_segment_manager();

	Shared::void_allocator alloc_inst(segment_manager);

	incoming_notifications = NULL;

	incoming_notifications = segment->find<Shared::sh_notification_map>(incoming_notification_key.data()).first; // karşı tarafın outgoing'i bu sınıfın incoming'i oluyor ve vice versa.

	if (!incoming_notifications)
	{
		incoming_notifications = segment->construct<Shared::sh_notification_map>(incoming_notification_key.data())(std::less<Shared::string>(), alloc_inst);
	}

	outgoing_notifications = NULL;

	outgoing_notifications = segment->find<Shared::sh_notification_map>(outgoing_notification_key.data()).first; // karşı tarafın outgoing'i bu sınıfın incoming'i oluyor ve vice versa.

	if (!outgoing_notifications)
	{
		outgoing_notifications = segment->construct<Shared::sh_notification_map>(outgoing_notification_key.data())(std::less<Shared::string>(), alloc_inst);
	}


	call_test_method([](std::string& reply) {
		std::cout << "first test of method call : " << reply << std::endl;
	});
	std::cout << "hello " << std::endl;

	test_receive_handler("oh la la!!!");

}


Shared_Memory_Handler::~Shared_Memory_Handler()
{
}

void Shared_Memory_Handler::add_notification(std::string const & key, std::string const & type)
{
	Shared::void_allocator alloc_inst(segment_manager);
	Shared::Sh_Notification_Struct _notification(key, type, alloc_inst);
	Shared::string  key_object(key.c_str(), alloc_inst);
	//Shared::string  key_object(std::to_string(getMilliCount()).c_str(), alloc_inst);
	Shared::sh_notification_pair _pair(key_object, _notification);

	outgoing_notifications->insert(_pair);
}

int Shared_Memory_Handler::get_notification_queue_size()
{
	return incoming_notifications->size();
}

Shared::Notification_Struct Shared_Memory_Handler::pop_notification()
{
	Shared::Notification_Struct return_struct;

	if (incoming_notifications->begin() != incoming_notifications->end())
	{
		Shared::sh_notification_map::iterator it = incoming_notifications->begin();

		//std::cout << it->first << std::endl;

		return_struct._key = it->second._key.c_str();
		return_struct._type = it->second._type.c_str();

		incoming_notifications->erase(incoming_notifications->begin());
	}
	else
	{
		std::cerr << " incoming_notifications vector is empty!" << std::endl;
	}

	return return_struct;
}

