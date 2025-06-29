#pragma once

#include <memory>
#include <thread>
#include "ConcurrentContainers.h"
#include "TaskExecutor.h"
#include "IExecutable.h"
#include "TaskQueueThread.h"
#include <mutex>

/* THIS HEADER REQUIRES C++17 */

class Messaging
{
	public:
		template <typename... Messages> class MessageListener;  // forwarded declaration

	private:
		template <typename Message> class MessageChannel
		{
			public:
				using MessagePtr = std::shared_ptr<const Message>;

			private:
				using ListenerT = MessageListener<Message>;
				static inline concurrent_uset<ListenerT*> m_listeners;

				static void SendOutMessage(void (ListenerT::*const receive_method)(MessagePtr), const MessagePtr& mess_ptr)
				{
					const auto traverse_func = [&](const auto& listeners)
					{
						std::for_each(std::begin(listeners), std::end(listeners), std::bind(receive_method, std::placeholders::_1, std::cref(mess_ptr)));
					};

					static_cast<const decltype(m_listeners)&>(m_listeners).invoke(traverse_func);
				}

				struct DispatchingTask : IExecutableT<MessagePtr>
				{
					void execute() override
					{
						SendOutMessage(&ListenerT::ReceiveMessageAsync, std::move(std::get<0>(IExecutableT<MessagePtr>::args)));
					}

					using IExecutableT<MessagePtr>::IExecutableT;
				};

			protected:
				static void SendOutMessageAsync(MessagePtr&& mess_ptr) { DispatchingThread.AcceptTask(std::make_shared<DispatchingTask>(std::move(mess_ptr))); }
				static void SendOutMessageSync(MessagePtr&& mess_ptr) { SendOutMessage(&ListenerT::ReceiveMessageSync, std::move(mess_ptr));	}

			public:
				static void AddListener(ListenerT *const listener) { m_listeners.emplace(listener); }
				static void RemoveListener(ListenerT *const listener) { m_listeners.erase(listener); }
		};

		template <typename Message> class MessageListener<Message>
		{
			private:
				using Channel = MessageChannel<Message>;

			public:
				using MessagePtr = typename Channel::MessagePtr;

			private:
				concurrent_queue<MessagePtr> m_received_messages;
				bool m_subscription = false;
				std::mutex m_handle_task_mtx;

				void RunHanldeLoop() 
				{
					while (auto message = ExtractFirstUnhandledMessage()) HandleMessage(std::move(message));
					m_handle_task_mtx.unlock();
				}

				struct MessageHanldeTask : IExecutableT<MessageListener&>
				{
					void execute() override { std::get<0>(IExecutableT<MessageListener&>::args).RunHanldeLoop(); }
					using IExecutableT<MessageListener&>::IExecutableT;
				};

			protected:
				MessagePtr ExtractFirstUnhandledMessage()
				{
					return HaveUnhandledMessages() ? m_received_messages.extract_first() : nullptr;
				}

				bool HaveUnhandledMessages() const { return !std::empty(m_received_messages); }
				bool GetSubscription() const { return m_subscription; } // Move & copy constructor/assign

				void SetSubscription(const bool subscribe)	 // Move & copy constructor/assign
				{
					if (m_subscription != subscribe)
						((m_subscription = subscribe) ? Channel::AddListener : Channel::RemoveListener)(this);
				}

				virtual void HandleMessage(const MessagePtr) = 0;

				~MessageListener()	// Doesn't need to be virtual.
				{
					SetSubscription(false);
					std::lock_guard<decltype(m_handle_task_mtx)> lock(m_handle_task_mtx);
				}
				
				void ResetQueue() { m_received_messages.clear(); }

			public:
//				MessageListener() = default;
//				MessageListener(const MessageListener& other) { SetSubscription(other.GetSubscription()); }
//
//				MessageListener(MessageListener&& other)
//				{ 
//					SetSubscription(other.GetSubscription());
//					other.SetSubscription(false);
//					m_received_messages.swap(other.m_received_messages);
//				}

				void ReceiveMessageAsync(MessagePtr mess_ptr)
				{
					m_received_messages.emplace(std::move(mess_ptr));
					if (m_handle_task_mtx.try_lock()) TaskScheduler::ExecuteTask(std::make_shared<MessageHanldeTask>(*this));
				}

				void ReceiveMessageSync(MessagePtr mess_ptr) { HandleMessage(std::move(mess_ptr));	}
		};

		template <typename Message> class ChannelPublisher {};  // Is it needed?

		static inline TaskQueueThread<IExecutable, std::shared_ptr> DispatchingThread;

	public:
		template <typename... Messages> class MessageListener : protected MessageListener<Messages>...
		{
			private:
				template <typename Message> using Base = MessageListener<Message>;

			protected:
				template <typename Message> using MessagePtr = typename Base<Message>::MessagePtr;
				template <typename Message> typename Base<Message>::MessagePtr ExtractFirstUnhandledMessage() { return Base<Message>::ExtractFirstUnhandledMessage(); }
				template <typename Message> bool HaveUnhandledMessages() const { return Base<Message>::HaveUnhandledMessages(); }
				template <typename Message> void SetSubscription(const bool subscribe) { Base<Message>::SetSubscription(subscribe); }
				template <typename Message> void GetSubscription() const { return Base<Message>::GetSubscription(); }
				template <typename Message> void HandleMessage(MessagePtr<Message>&& message) { Base<Message>::HandleMessage(std::move(message)); };
				template <typename Message> void ResetQueue() { Base<Message>::ResetQueue(); }

				void SetAllSubscriptions(const bool subscribe) { (SetSubscription<Messages>(subscribe), ...); }
				void ResetAllQueues() { (ResetQueue<Messages>(), ...); }

			public:
				template <typename Message> void ReceiveMessageAsync(MessagePtr<Message> mess_ptr) { Base<Message>::ReceiveMessageAsync(mess_ptr); }
				template <typename Message> void ReceiveMessageSync(MessagePtr<Message> mess_ptr) { Base<Message>::ReceiveMessageSync(mess_ptr); }
		};

		template <typename... Messages> class MessageQueue : MessageChannel<Messages>...
		{
			public:
				template <typename Msg> static void SendMessageAsync(std::unique_ptr<Msg>& mess_ptr) { MessageChannel<Msg>::SendOutMessageAsync(std::move(mess_ptr)); }
				template <typename Msg> static void SendMessageAsync(std::unique_ptr<Msg>&& mess_ptr) { SendMessageAsync(mess_ptr); }
				template <typename Msg, typename... Args> static void SendMessageAsync(Args&&... args) { SendMessageAsync(CreateMessage<Msg>(std::forward<Args>(args)...)); }

				template <typename Msg> static void SendMessageSync(std::unique_ptr<Msg>& mess_ptr) { MessageChannel<Msg>::SendOutMessageSync(std::move(mess_ptr)); }
				template <typename Msg> static void SendMessageSync(std::unique_ptr<Msg>&& mess_ptr) { SendMessageSync(mess_ptr); }
				template <typename Msg, typename... Args> static void SendMessageSync(Args&&... args) { SendMessageSync(CreateMessage<Msg>(std::forward<Args>(args)...)); }

				template <typename Msg, typename... Args> static std::unique_ptr<Msg> CreateMessage(Args&&... args) { return std::make_unique<Msg>(std::forward<Args>(args)...); }
		};
};


// Exports
template <typename... Messages> using MessageListener = Messaging::MessageListener<Messages...>;
template <typename... Messages> using MessageQueue = Messaging::MessageQueue<Messages...>;

namespace messages 
{
	struct Notification {};
}
