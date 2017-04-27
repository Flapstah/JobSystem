// JobSystem.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <functional>
#include <iostream>
#include <memory>

#include "JobSystem.h"

//////////////////////////////////////////////////////////////////////////
//	A job requires the following:
//		1)	a worker function (with arguments) to call (a std::bind()
//				forwarding call)
//	At first, I thought it would also require a callback function, but
//	any callback function required is an argument to the worker function
//	described above.
//	The trick is to be able to create and store the std::bind() object in
//	a type agnostic way (SFINAE), such that they can be invoked on the
//	called thread, and any callback invoked on the callers thread
//  N.B. Don't need dynamic return types - where would the callback
//  'return' to?
//////////////////////////////////////////////////////////////////////////

struct _callback_base
{
	_callback_base() {}
	virtual ~_callback_base() {}
	virtual void operator()() { std::cout << "_callback_base::operator() - probably should crash if this is called" << std::endl; };
};

template <typename _function, typename ... _arguments>
struct _callback_wrapper : public _callback_base
{
	std::_Binder<std::_Unforced, _function, _arguments ...> m_callback;
	explicit _callback_wrapper(_function&& f, _arguments&&... a) : m_callback(std::bind(std::forward<_function>(f), std::forward<_arguments>(a)...)) {}
	virtual void operator()() { m_callback(); }
};

_callback_base* g_pcb = nullptr;

template <typename _function, typename ... _arguments>
inline bool add_callback(_function&& f, _arguments&&... a)
{
	g_pcb = new _callback_wrapper<_function, _arguments...>(std::forward<_function>(f), std::forward<_arguments>(a)...);
	return (g_pcb != nullptr) ? true : false;
}

inline void call_callback()
{
	if (g_pcb != nullptr)
	{
		g_pcb->operator()();
		delete g_pcb;
		g_pcb = nullptr;
	}
}


void function1(int a, float b)
{
	std::cout << "void f1(" << a << ", " << b << ")" << std::endl;
}

void function2(const char* a)
{
	std::cout << "void f2(\"" << a << "\")" << std::endl;
}

class CClass
{
public:
	void method(double x)
	{
		std::cout << "CClass::f1(" << x << ")" << std::endl;
	};

protected:
private:
};

int main(int argc, char *argv[])
{
	/*
	add_callback(function1, 12, 34.56f);
	call_callback();

	add_callback(function2, "wallop");
	call_callback();

	CClass instance;
	add_callback(&CClass::method, &instance, 123.456);
	call_callback();
	*/

	CJobSystem js;
	js.Update();

	return 0;
}