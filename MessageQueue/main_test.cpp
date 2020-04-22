#include <iostream>
#include "MessageQueue.h"
#include <string>

using gMessageQueue = MessageQueue<MainChannelMessage, MemeMessage, FileSystemMessage>;

class MemeListener : public ChannelListener<gMessageQueue, MemeMessage> 
{
public:
	MemeListener() { SetSubscription(true); }

	void CheckIfNewMemes() 
	{
		auto& queue = ChannelListener<gMessageQueue, MemeMessage>::m_queue;
		if (queue.size()) std::cout << "Hi! I got some funny memes. Can't wait to share them with you:\n";
		while (m_queue.size())
			std::cout << '\t' << m_queue.front()->funny_thing << '\n',
			m_queue.pop();
	}
};

class TwoChannelListener : public ChannelListener<gMessageQueue, MainChannelMessage>, public ChannelListener<gMessageQueue, MemeMessage>
{
public:
	TwoChannelListener() 
	{
		ChannelListener<gMessageQueue, MainChannelMessage>::SetSubscription(true);
		ChannelListener<gMessageQueue, MemeMessage>::SetSubscription(true);
	}

	void HandleMessages() 
	{
		HandleMainMessages();
		HandleMemeMessages();
	}

	void HandleMainMessages() 
	{
		auto& queue = ChannelListener<gMessageQueue, MainChannelMessage>::m_queue;
		if (queue.size()) std::cout << "Got new system messages! Handling them...\n";
		while (queue.size())
			std::cout << "\tWorking on event " << queue.front()->event_descritor << '\n',
			queue.pop();
	}

	void HandleMemeMessages() 
	{
		auto& queue = ChannelListener<gMessageQueue, MemeMessage>::m_queue;
		if (queue.size()) std::cout << "Got new memes! Printing them...\n";
		while (queue.size())
			std::cout << '\t' << queue.front()->funny_thing << '\n',
			queue.pop();
	}
};

struct NotRegisteredMessage {};

int main()
{
	MemeListener ml1, ml2;
	TwoChannelListener tcl;
//	auto& q = gMessageQueue::m_channels;


	gMessageQueue::PostMessage(std::make_shared<MemeMessage>("One does not simply use templates without kilobytes of error logs."));
	gMessageQueue::PostMessage(std::make_shared<MainChannelMessage>(4538));
	gMessageQueue::BroadcastMessages();

	ml1.CheckIfNewMemes();
	ml2.CheckIfNewMemes();
	tcl.HandleMessages();

	return 0;
}

