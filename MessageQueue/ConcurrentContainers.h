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
		using MyBase = std::unordered_set<_Kty, _Hasher, _Keyeq, _Alloc>;
		using typename MyBase::size_type;
		using typename MyBase::key_type;
		using typename MyBase::iterator;

		mutable std::mutex mtx;
		using lock_guard = std::lock_guard<decltype(mtx)>;

	public:
		using typename MyBase::value_type;

		size_type erase(const key_type& _Keyval) 
		{
			lock_guard lock(mtx);
			return MyBase::erase(_Keyval);
		}

		template<class... _Valty> decltype(auto) emplace(_Valty&&... _Val)
		{
			lock_guard lock(mtx);
			/*return*/ MyBase::emplace(std::forward<_Valty>(_Val)...).first;
		}

		decltype(auto) size() const noexcept(noexcept(MyBase::size()))
		{
			lock_guard lock(mtx);
			return MyBase::size();
		}

		template <typename F> decltype(auto) invoke(F&& func)
		{
			lock_guard lock(mtx);
			return func(MyBase::begin(), MyBase::end());
		}
};


template <class _Ty,
		  class _Container = std::deque<_Ty> >
class concurrent_queue : protected std::queue<_Ty, _Container>
{
	private:
		using MyBase = std::queue<_Ty, _Container>;

		mutable std::mutex mtx;
		using lock_guard = std::lock_guard<decltype(mtx)>;

	public:
		using typename MyBase::value_type;

		value_type extract_first()
		{
			lock_guard lock(mtx);

			auto element = std::move(MyBase::front());
			MyBase::pop();

			return element;
		}

		template<class... _Valty> decltype(auto) emplace(_Valty&&... _Val) 
		{
			lock_guard lock(mtx);
			/*return*/ MyBase::emplace(std::forward<_Valty>(_Val)...);
		}

		void clear()
		{
			lock_guard lock(mtx);
			while (!MyBase::empty()) MyBase::pop();
		}

		decltype(auto) size() const noexcept(noexcept(MyBase::size()))
		{
			lock_guard lock(mtx);
			return MyBase::size();
		}

		decltype(auto) empty() const noexcept(noexcept(MyBase::empty()))
		{
			lock_guard lock(mtx);
			return MyBase::empty();
		}
};
