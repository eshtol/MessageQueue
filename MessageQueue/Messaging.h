#pragma once
#include <memory>
#include <thread>
#include "ConcurrentContainers.h"
#include "TaskExecutor.h"
#include "IExecutable.h"
#include "TaskQueueThread.h"

/* THIS HEADER REQUIRES C++17 */

class Messaging 
{
	public:
		template <typename... Messages> class MessageListener;  // forwarded declaration

	private:
		template <typename Message> class MessageChannel
		{
			private:
				typedef MessageListener<Message> ListenerT;
				static inline concurrent_queue<std::shared_ptr<const Message>> m_queue;
				static inline concurrent_uset<ListenerT*> m_listeners;

			protected:
				static void SendOutQueue()
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
				static inline void AddListener(ListenerT *const listener) { m_listeners.emplace(listener); }
				static inline void RemoveListener(ListenerT *const listener) { m_listeners.erase(listener); }
				static inline void PushMessage(typename decltype(m_queue)::value_type&& mess_ptr) { m_queue.emplace(std::move(mess_ptr)); }
		};

		template <typename T> struct message_tag {};

		template <typename... Messages> class MessageListener : protected MessageListener<Messages>...
		{
			private:
				template <typename Message> using Base = MessageListener<Message>;
			protected:
				template <typename Message> using message_tag = typename Base<Message>::template message_tag<Message>;	//Хитрая строчка, правда? :)
				template <typename Message> using MessagePtr = typename Base<Message>::MessagePtr;
				template <typename Message> inline typename Base<Message>::MessagePtr GetUnhandledMessage() { return Base<Message>::GetUnhandledMessage(); }
				template <typename Message> inline bool HaveUnhandledMessages() const { return Base<Message>::HaveUnhandledMessages(); }
				template <typename Message> inline void SetSubscription(const bool subscribe) { Base<Message>::SetSubscription(subscribe); }
				inline void SetAllSubscriptions(const bool subscribe) { (SetSubscription<Messages>(subscribe), ...); }
				template <typename Message> inline void GetSubscription() const { return Base<Message>::GetSubscription(); }
				template <typename Message> inline void HandleMessage() { Base<Message>::HandleMessage(message_tag<Message>); };
			public:
				template <typename Message> inline void ReceiveMessage(MessagePtr<Message> mess_ptr) { Base<Message>::ReceiveMessage(mess_ptr); }
		};

		template <typename Message> class MessageListener<Message>
		{
			protected:
				typedef std::shared_ptr<const Message> MessagePtr;
				template <typename T> using message_tag = Messaging::message_tag<T>;
			private:
				typedef MessageChannel<Message> Channel;
				friend class Channel;
				concurrent_queue<MessagePtr> m_received_messages;
				bool m_subscription = false;

				struct MessageHanldeTask : public IExecutableT<MessageListener&>
				{
					void execute() override { std::get<0>(IExecutableT<MessageListener&>::args).HandleMessage(message_tag<Message>()); }
					using IExecutableT<MessageListener&>::IExecutableT;
				};

			protected:
				MessagePtr GetUnhandledMessage() { return HaveUnhandledMessages() ? m_received_messages.extract_first() : nullptr; }
				bool HaveUnhandledMessages() const { return m_received_messages.size(); }
				void SetSubscription(const bool subscribe) { if (m_subscription != subscribe) (m_subscription = subscribe) ? Channel::AddListener(this) : Channel::RemoveListener(this); } // Move & copy constructor/assign
				void GetSubscription() const { return m_subscription; } // Move & copy constructor/assign
				virtual void HandleMessage(message_tag<Message>) = 0;
				~MessageListener() { SetSubscription(false); }  // Doesn't need to be virtual.
			public:
				void ReceiveMessage(MessagePtr mess_ptr)
				{
					m_received_messages.emplace(std::move(mess_ptr));
					TaskScheduler::ExecuteTask(std::make_shared<MessageHanldeTask>(*this));
				}

		};

		template <typename Message> class ChannelPublisher {};  // Is it needed?


		//TaskQueueThread<IExecutable, std::shared_ptr> DispatchingThread;

	public:
		template <typename... Messages> class MessageQueue 
		{
			private:
				class QueueCore : public MessageChannel<Messages>...
				{
					private:
						void DispatchingLoop()
						{
							while (true) (MessageChannel<Messages>::SendOutQueue(), ..., std::this_thread::sleep_for(std::chrono::milliseconds(1)));	// Magic is here
						};

					public:
						QueueCore() { std::thread([this]() { DispatchingLoop(); }).detach(); }

				} static inline m_core;

			public:
				template <typename Msg> static inline void SendMessageAsync(std::unique_ptr<Msg>& mess_ptr) { m_core.MessageChannel<Msg>::PushMessage(std::move(mess_ptr)); }
				template <typename Msg> static inline void SendMessageAsync(std::unique_ptr<Msg>&& mess_ptr) { SendMessageAsync(mess_ptr); }
				template <typename Msg, typename... Args> static inline void SendMessageAsync(Args&&... args) { SendMessageAsync(CreateMessage<Msg>(std::forward<Args>(args)...)); }

				template <typename Msg> static inline void SendMessageSync(std::unique_ptr<Msg>& mess_ptr) { throw std::exception("Does not implemented yet :("); }
				template <typename Msg> static inline void SendMessageSync(std::unique_ptr<Msg>&& mess_ptr) { SendMessageSync(mess_ptr); }
				template <typename Msg, typename... Args> static inline void SendMessageSync(Args&&... args) { SendMessageSync(CreateMessage<Msg>(std::forward<Args>(args)...)); }

				template <typename Msg, typename... Args> static inline std::unique_ptr<Msg> CreateMessage(Args&&... args) { return std::make_unique<Msg>(std::forward<Args>(args)...); }
		};
};


// Exports
template <typename... Messages> using MessageListener = Messaging::MessageListener<Messages...>;
template <typename... Messages> using MessageQueue = Messaging::MessageQueue<Messages...>;

namespace messages 
{
	struct Notification {};
}