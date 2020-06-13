#include <iostream>
#include "MessageQueue.h"
#include "messages_test.h"





using gMessageQueue = MessageQueue<MainChannelMessage, MemeMessage, FileSystemMessage>;

class MemeListener : public ChannelListener<gMessageQueue, MemeMessage> 
{
public:
	MemeListener() { SetSubscription(true); }

	void HandleMessage(message_tag<MemeMessage>) override
	{
		if (HaveUnhandledMessages()) std::cout << "Hi! I got some funny memes. Can't wait to share them with you:\n";
		while (auto message = GetUnhandledMessage()) std::cout << '\t' << message->funny_thing << '\n';
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

	void HandleMessage(message_tag<MainChannelMessage>) override
	{
		typedef ChannelListener<gMessageQueue, MainChannelMessage> ThisChannel;
		if (ThisChannel::HaveUnhandledMessages()) std::cout << "Got new system messages! Handling them...\n";
		while (auto message = ThisChannel::GetUnhandledMessage())
			std::cout << "\tWorking on event " << message->event_descritor << '\n';
	}

	void HandleMessage(message_tag<MemeMessage>) override
	{
		typedef ChannelListener<gMessageQueue, MemeMessage> ThisChannel;
		if (ThisChannel::HaveUnhandledMessages()) std::cout << "Got new memes! Printing them...\n";
		while (auto message = ThisChannel::GetUnhandledMessage())
			std::cout << '\t' << message->funny_thing << '\n';
	}
};



int main()
{
	MemeListener ml1, ml2;
	TwoChannelListener tcl;

	gMessageQueue::PostMessage<MemeMessage>("One does not simply use templates without kilobytes of error logs.");
	gMessageQueue::PostMessage<MainChannelMessage>(4538);

	std::cin.get();
	return 0;
}

