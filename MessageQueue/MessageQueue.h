#pragma once
#include <unordered_set>
#include <queue>
#include <memory>
#include <algorithm>
#include <functional>
#include <mutex>
#include <thread>
#include "ConcurrentContainers.h"

/* THIS HEADER REQUIRES C++17 */


template <typename MsgQueue, typename Message>
class ChannelListener 
{
	private:
		friend MsgQueue;
		concurrent_queue<std::shared_ptr<Message>> m_received_messages;

	protected:		
		typename decltype(m_received_messages)::value_type GetUnhandledMessage() { return HaveUnhandledMessages() ? m_received_messages.extract_first() : nullptr; }
		void ReceiveMessage(typename decltype(m_received_messages)::value_type mess_ptr) { m_received_messages.emplace(std::move(mess_ptr)); }
		bool HaveUnhandledMessages() const { return m_received_messages.size(); }
		void SetSubscription(const bool subscribe) { subscribe ? MsgQueue::AddSubscriber(this) : MsgQueue::RemoveSubscriber(this); } // Move & copy constructor/assign
		~ChannelListener() { SetSubscription(false); }
};


template <typename... Messages> class MessageQueue 
{
	private:
		template <typename MessT> class Channel 
		{
			private:
				typedef ChannelListener<MessageQueue, MessT> ListenerT;
				concurrent_queue<std::shared_ptr<MessT>> m_queue;
				concurrent_uset<ListenerT*> m_listeners;

			protected:
				void SendOutQueue()
				{
					if (m_queue.size()) 
					{
						const auto iters = m_listeners.iteration_lock();
						while (m_queue.size())
							std::for_each(iters.first, iters.second, std::bind(&ListenerT::ReceiveMessage, std::placeholders::_1, m_queue.extract_first()));
						m_listeners.iteration_unlock();
					}
				}

			public:
				inline void AddListener(ListenerT *const listener) { m_listeners.emplace(listener); }
				inline void RemoveListener(ListenerT *const listener) { m_listeners.erase(listener); }
				inline void PushMessage(typename decltype(m_queue)::value_type mess_ptr) { m_queue.emplace(std::move(mess_ptr)); }
		};

		class QueueCore : public Channel<Messages>...
		{ 
			private:
				void CoreLoop()
				{
					while (true) (Channel<Messages>::SendOutQueue(), ..., std::this_thread::sleep_for(std::chrono::milliseconds(1)));	// Magic is here
				};

			public:
				QueueCore() { std::thread([this]() { CoreLoop(); }).detach(); }
			
		} static inline m_core;	
		
	public:
		template <typename MessT> static inline void PostMessage(std::shared_ptr<MessT> mess_ptr) { m_core.Channel<MessT>::PushMessage(std::move(mess_ptr)); }
		template <typename MessT> static inline void AddSubscriber(ChannelListener<MessageQueue, MessT> *const subscriber) { m_core.Channel<MessT>::AddListener(subscriber); }
		template <typename MessT> static inline void RemoveSubscriber(ChannelListener<MessageQueue, MessT> *const subscriber) { m_core.Channel<MessT>::RemoveListener(subscriber); }
};