#pragma once
#include <unordered_set>
#include <queue>
#include <memory>
#include <algorithm>
#include <functional>


struct MainChannelMessage
{
	MainChannelMessage(const int dsc) : event_descritor(dsc) {}
	int event_descritor;
};

struct MemeMessage
{
	MemeMessage(const std::string str) : funny_thing(str) {}
	std::string funny_thing;
	~MemeMessage() { std::cout << "Meme destructed\n"; }
};

struct FileSystemMessage
{
	FILE* file_descriptor;
	std::string path;
	bool some_flag;
};

/* THIS HEADER REQUIRES C++17 */


template <typename Queue, typename Message> class ChannelListener 
{
	protected:
		std::queue<std::shared_ptr<Message>> m_queue;

		void ReceiveMessage(typename decltype(m_queue)::value_type mess_ptr) { m_queue.emplace(std::move(mess_ptr)); }
		void SetSubscription(const bool subscribe) { subscribe ? Queue::AddSubscriber(this) : Queue::RemoveSubscriber(this); } // Move constructor/assign
		~ChannelListener() { SetSubscription(false); }
		friend Queue;
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
				void PushMessage(std::shared_ptr<MessT> mess_ptr) { m_queue.push(std::move(mess_ptr)); }

			protected:
				std::queue<std::shared_ptr<MessT>> m_queue;
				std::unordered_set<ListenerT*> m_listeners;

				void SendOutQueue() 
				{
					while (m_queue.size())
						std::for_each(m_listeners.begin(), m_listeners.end(), std::bind(&ListenerT::ReceiveMessage, std::placeholders::_1, m_queue.front())),
						m_queue.pop();
				}
		};

		struct : public Channel<Messages>...
		{ 
			void BroadcastIteration() { (Channel<Messages>::SendOutQueue(), ...); }		// Magic is here
		} static inline m_channels;	
		

	public:
		template <typename MessT> static void PostMessage(std::shared_ptr<MessT> mess_ptr) { m_channels.Channel<MessT>::PushMessage(std::move(mess_ptr)); }
		template <typename MessT> static void AddSubscriber(ChannelListener<MessageQueue, MessT> *const subscriber) { m_channels.Channel<MessT>::AddListener(subscriber); }
		template <typename MessT> static void RemoveSubscriber(ChannelListener<MessageQueue, MessT> *const subscriber) { m_channels.Channel<MessT>::RemoveListener(subscriber); }
		static void BroadcastMessages() { m_channels.BroadcastIteration(); }
};