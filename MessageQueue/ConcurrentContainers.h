#pragma once
#include <unordered_set>
#include <queue>
#include <mutex>

template <class _Kty,
		  class _Hasher = std::hash<_Kty>,
		  class _Keyeq = std::equal_to<_Kty>,
		  class _Alloc = std::allocator<_Kty>>
class concurrent_uset : protected std::unordered_set<_Kty, _Hasher, _Keyeq, _Alloc>
{
	private:
		typedef std::unordered_set<_Kty, _Hasher, _Keyeq, _Alloc> MyBase;
		std::mutex mtx;
		typedef std::lock_guard<decltype(mtx)> lock_guard;
		using typename MyBase::size_type;
		using typename MyBase::key_type;
		using typename MyBase::iterator;

	public:
		using typename MyBase::value_type;
		using MyBase::size;

		size_type erase(const key_type& _Keyval) 
		{
			lock_guard lock(mtx);
			return MyBase::erase(_Keyval);
		}

		template<class... _Valty> iterator emplace(_Valty&&... _Val)
		{
			lock_guard lock(mtx);
//			return MyBase::emplace(std::forward<_Valty>(_Val)...);
			return MyBase::emplace(std::forward<_Valty>(_Val)...).first;
		}

		std::pair<iterator, iterator> iteration_lock() 
		{
			mtx.lock();
			return std::make_pair(MyBase::begin(), MyBase::end());
		}

		void iteration_unlock() { mtx.unlock(); }
};


template <class _Ty,
		  class _Container = std::deque<_Ty> >
class concurrent_queue : protected std::queue<_Ty, _Container>
{
	private:
		typedef std::queue<_Ty, _Container> MyBase;
		using MyBase::front;
		using MyBase::pop;
		std::mutex mtx;
		typedef std::lock_guard<decltype(mtx)> lock_guard;

	public:
		using typename MyBase::value_type;
		using MyBase::size;

		value_type extract_first()
		{
			lock_guard lock(mtx);
			auto element = std::move(front());
			pop();
			return element;
		}

		template<class... _Valty> decltype(auto) emplace(_Valty&&... _Val) 
		{
			lock_guard lock(mtx);
			return MyBase::emplace(std::forward<_Valty>(_Val)...);
		}
};
