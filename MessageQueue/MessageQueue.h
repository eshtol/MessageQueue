#pragma once
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <concurrent_unordered_map.h>
#include <memory>



class Message {};

struct MainChannelMessage
{
	int event_descritor;
};

struct MemeMessage
{
	std::string funny_thing;
};

struct FileSystemMessage
{
	FILE* file_descriptor;
	std::string path;
	bool some_flag;
};




template <typename Queue, typename Message> class ChannelListener 
{
	protected:
		std::queue<std::shared_ptr<Message>> m_queue;

		void ReceiveMessage(typename decltype(m_queue)::value_type mess_ptr) { m_queue.emplace(std::move(mess_ptr)); }
		void SetSubscription(const bool subscribe) { subscribe ? Queue::AddSubscriber(this) : Queue::RemoveSubscriber(this); } // Move constructor/assign
		~ChannelListener() { SetSubscription(false); }
};


template <typename... Messages> class MessageQueue 
{
	private:
		template <typename MessT> class Channel 
		{
			public:
				typedef ChannelListener<MessageQueue, MessT> ListenerT;
				void AddListener(ListenerT *const listener) { m_listeners.emplace(listener); };
				void RemoveListener(ListenerT *const listener) { m_listeners.erase(listener); };
				void PushMessage(std::shared_ptr<MessT> mess_ptr) { m_queue.push(mess_ptr); }

			private:
				std::queue<std::shared_ptr<MessT>> m_queue;
				std::unordered_set<ListenerT*> m_listeners;
			
		};
public:
		static inline class Core : public Channel<Messages>...
		{

		} m_channels;

	public:
		template <typename MessT> static void PostMessage(std::shared_ptr<MessT> mess_ptr) { m_channels.Channel<MessT>::PushMessage(mess_ptr); }
		template <typename MessT> static void AddSubscriber(ChannelListener<MessageQueue, MessT> *const subscriber) { m_channels.Channel<MessT>::AddListener(subscriber); }
		template <typename MessT> static void RemoveSubscriber(ChannelListener<MessageQueue, MessT> *const subscriber) { m_channels.Channel<MessT>::RemoveListener(subscriber); }
};