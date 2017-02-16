#pragma once
#include "Shared_Memory_Handler.h"

namespace Shared_Extension
{
	template <typename T> using sh_alloc = bip::allocator<T, Shared::segment_manager>;
	template <typename T> using sh_vector = bip::vector<T, sh_alloc<T> >;
	template <typename T1, typename T2> using sh_map = bip::map<T1, T2, sh_alloc<T1>>;

	/*------------------------------------------test struct-------------------------------------------------*/
	typedef struct
	{
		std::map<std::string, double>   test_dbl_map;
		std::vector<std::string>		test_str_vector;
		std::vector<double>				test_dbl_vector;
		std::string						test_string;
		double							test_double;
		int								test_int;
		bool							test_bool;
	}Test_Struct;

	template <typename Alloc = std::allocator<void>>
	struct basic_test_data
	{
		typedef sh_vector<Shared::string> sh_str_vector;
		typedef sh_vector<double> sh_double_vector;
		typedef sh_map<Shared::string, double> sh_double_map;

		basic_test_data(Alloc alloc = {}) :
			test_str_vector(alloc),
			test_dbl_vector(alloc),
			test_string(alloc),
			tset_dbl_map(alloc)
		{ }

		sh_double_map    tset_dbl_map;
		sh_str_vector    test_str_vector;
		sh_double_vector test_dbl_vector;
		Shared::string   test_string;
		double		     test_double;
		int			     test_int;
		bool		     test_bool;
	};
	using test_data = basic_test_data<>; // just heap allocated
	using test = basic_test_data<sh_alloc<void>>; // shared memory version
	/*------------------------------------------test struct-------------------------------------------------*/
}


class Shared_Memory_Extension :
	public Shared_Memory_Handler
{
public:
	Shared_Memory_Extension(std::string const & segment_name = "extension_segment");
	~Shared_Memory_Extension();

	void set_test_struct(std::string const & key, Shared_Extension::Test_Struct const & arg_struct)
	{
		Shared_Extension::test *_test = segment->find_or_construct<Shared_Extension::test>(key.data())(segment_manager);

		_test->test_bool = arg_struct.test_bool;
		_test->test_int = arg_struct.test_int;
		_test->test_double = arg_struct.test_double;
		_test->test_string = arg_struct.test_string.c_str();


		for (auto it = arg_struct.test_str_vector.begin(); it != arg_struct.test_str_vector.end(); it++) 
		{
			Shared::string sh_str(segment_manager);
			sh_str = (*it).c_str();
			_test->test_str_vector.push_back(sh_str);
		}

		for (auto it = arg_struct.test_dbl_vector.begin(); it != arg_struct.test_dbl_vector.end(); it++)
		{
			_test->test_dbl_vector.push_back(*it);
		}

		add_notification(key.c_str(), typeid(Shared_Extension::Test_Struct).name());

	}
	void get_test_struct(std::string const & key, Shared_Extension::Test_Struct & return_struct)
	{
		Shared_Extension::test *_test = segment->find<Shared_Extension::test>(key.data()).first;

		if (_test == NULL)
		{
			throw std::invalid_argument(key);
		}

		return_struct.test_bool = _test->test_bool;
		return_struct.test_int = _test->test_int;
		return_struct.test_double = _test->test_double;
		return_struct.test_string = _test->test_string.c_str();

		for (auto it = _test->test_str_vector.begin(); it != _test->test_str_vector.end(); it++) /* standart vektör
																								 shared tipi vektöre aktarýlýyor */
		{
			std::string str;
			str = (*it).c_str();
			return_struct.test_str_vector.push_back(str);
		}

		for (auto it = _test->test_dbl_vector.begin(); it != _test->test_dbl_vector.end(); it++)/* standart vektör
																								shared tipi vektöre aktarýlýyor */
		{
			return_struct.test_dbl_vector.push_back(*it);
		}
	}
	
};

