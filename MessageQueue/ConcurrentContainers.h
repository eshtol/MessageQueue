#pragma once

#include <unordered_set>
#include <queue>
#include <shared_mutex>


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

		mutable std::shared_mutex m_mutex;
		using shared_lock = std::shared_lock<decltype(m_mutex)>;
		using exclusive_lock = std::lock_guard<decltype(m_mutex)>;

	public:
		using typename MyBase::value_type;

		size_type erase(const key_type& _Keyval) 
		{
			exclusive_lock lock(m_mutex);
			return MyBase::erase(_Keyval);
		}

		template<class... _Valty> decltype(auto) emplace(_Valty&&... _Val)
		{
			exclusive_lock lock(m_mutex);
			return (void)MyBase::emplace(std::forward<_Valty>(_Val)...).first;
		}

		decltype(auto) size() const noexcept(noexcept(MyBase::size()))
		{
			shared_lock lock(m_mutex);
			return MyBase::size();
		}

		template <typename F> decltype(auto) invoke(F&& func) const
		{
			shared_lock lock(m_mutex);
			return func(static_cast<const MyBase&>(*this));
		}
		
		template <typename F> decltype(auto) invoke(F&& func)
		{
			exclusive_lock lock(m_mutex);
			return func(static_cast<MyBase&>(*this));		// Providing full unprotected interface under lock.
		}
};


template <class _Ty,
		  class _Container = std::deque<_Ty>>
class concurrent_queue : protected std::queue<_Ty, _Container>
{
	private:
		using MyBase = std::queue<_Ty, _Container>;

		mutable std::shared_mutex m_mutex;
		using shared_lock = std::shared_lock<decltype(m_mutex)>;
		using exclusive_lock = std::lock_guard<decltype(m_mutex)>;

	public:
		using typename MyBase::value_type;

		value_type extract_first()
		{
			exclusive_lock lock(m_mutex);

			auto element = std::move(MyBase::front());
			MyBase::pop();

			return element;
		}

		template<class... _Valty> decltype(auto) emplace(_Valty&&... _Val) 
		{
			exclusive_lock lock(m_mutex);
			return (void)MyBase::emplace(std::forward<_Valty>(_Val)...);
		}

		void clear()
		{
			exclusive_lock lock(m_mutex);
			while (!MyBase::empty()) MyBase::pop();
		}

		decltype(auto) size() const noexcept(noexcept(MyBase::size()))
		{
			shared_lock lock(m_mutex);
			return MyBase::size();
		}

		decltype(auto) empty() const noexcept(noexcept(MyBase::empty()))
		{
			shared_lock lock(m_mutex);
			return MyBase::empty();
		}
};
