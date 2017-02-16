#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

/// Wrapper for std::thread that automatically joins at end of scope.
class Joining_Thread
{
public:
	/// Constructor taking a Function with its arguments.
	template <typename Function, typename... Args>
	Joining_Thread(Function&& f, Args&&... args)
		:
		thread(std::forward<Function>(f), std::forward<Args>(args)...)
	{
	}

	Joining_Thread(Joining_Thread&&) = default;

	~Joining_Thread()
	{
		thread.join();
	}

	Joining_Thread(const Joining_Thread&) = delete;

	Joining_Thread& operator=(const Joining_Thread&) = delete;

private:
	std::thread thread;
};

class Call_Back_Timer
{
public:

	Call_Back_Timer(int interval, std::function<void(void)> func)
		:
		execute(true),
		thread([=]()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(interval)); // ilk çalýþtýrmada interval kadar beklemesi için.

		while (execute)
		{
			func();
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
		}
	})
	{
	}

	void stop()
	{
		execute = false;
	}

private:
	std::atomic<bool> execute;
	Joining_Thread thread;

};