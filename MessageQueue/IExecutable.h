#pragma once
#include <tuple>

class IExecutable
{
	public:
		virtual void execute() = 0;
		virtual ~IExecutable() = default;
};


template <typename ...Args> class IExecutableT : public IExecutable
{
	protected:
		const std::tuple<Args...> args;

	public:
		IExecutableT(Args&&... _args) : args(std::forward<Args>(_args)...) {}

		~IExecutableT() override = default;
};