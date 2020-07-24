#include <iostream>
#include "Messaging.h"
#include "messages_test.h"





using gMessageQueue = MessageQueue<MainChannelMessage, MemeMessage, FileSystemMessage>;

class MemeListener : public MessageListener<MemeMessage>
{
public:
	MemeListener() { SetSubscription(true); }

	void HandleMessage(message_tag<MemeMessage>) override
	{
		if (HaveUnhandledMessages()) std::cout << "Hi! I got some funny memes. Can't wait to share them with you:\n";
		while (auto message = GetUnhandledMessage()) std::cout << '\t' << message->funny_thing << '\n';
	}

};

class TwoChannelListener : public MessageListener<MainChannelMessage, MemeMessage>
{
public:
	TwoChannelListener() 
	{
		SetAllSubscriptions(true);
	}

	void HandleMessage(message_tag<MainChannelMessage>) override
	{
		if (HaveUnhandledMessages<MainChannelMessage>()) std::cout << "Got new system messages! Handling them...\n";
		while (auto message = GetUnhandledMessage<MainChannelMessage>())
			std::cout << "\tWorking on event " << message->event_descritor << '\n';
	}

	void HandleMessage(message_tag<MemeMessage>) override
	{
		if (HaveUnhandledMessages<MemeMessage>()) std::cout << "Got new memes! Printing them...\n";
		while (auto message = GetUnhandledMessage<MemeMessage>())
			std::cout << '\t' << message->funny_thing << '\n';
	}
};


class NotRegisteredMessageListener : public MessageListener<NotRegisteredMessage> 
{
	private:
		void HandleMessage(message_tag<NotRegisteredMessage>)
		{
			std::cout << "Got unregistered message: " << GetUnhandledMessage()->information << "\n";
		}
};



int main()
{
	MemeListener ml1, ml2;
	TwoChannelListener tcl;

	//auto ml3= std::move( ml1);

	gMessageQueue::SendMessageAsync<MemeMessage>("One does not simply use templates without kilobytes of error logs.");
	gMessageQueue::SendMessageAsync<MainChannelMessage>(4538);

	NotRegisteredMessageListener nrml;   // You can send messages bypass of message queue (directly, privately) as it is observer pattern based system.
	auto msg = std::make_shared<NotRegisteredMessage>();
	msg->information = 42;
	nrml.ReceiveMessage(msg);


	std::cin.get();
	return 0;
}

