#include <iostream>
#include "Messaging.h"
#include "messages_test.h"





using gMessageQueue = MessageQueue<MainChannelMessage, MemeMessage, FileSystemMessage>;


using gMessageQueue2 = MessageQueue<MemeMessage>;

class MemeListener : public MessageListener<MemeMessage>
{
public:
	MemeListener() { SetSubscription(true); }

	void HandleMessage(const MessagePtr message) override
	{
		std::cout << "Hi! I got some funny memes. Can't wait to share them with you:\n";
		std::cout << '\t' << message->funny_thing << '\n';
	}

};

class TwoChannelListener : public MessageListener<MainChannelMessage, MemeMessage>
{
public:
	TwoChannelListener()
	{
		SetAllSubscriptions(true);
	}

	void HandleMessage(const MessagePtr<MainChannelMessage> message) override
	{
		std::cout << "Got new system messages! Handling them...\n";
		std::cout << "\tWorking on event " << message->event_descritor << '\n';
	}

	void HandleMessage(const MessagePtr<MemeMessage> message) override
	{
		std::cout << "Got new memes! Printing them...\n";
		std::cout << '\t' << message->funny_thing << '\n';
	}

	~TwoChannelListener() { ResetAllQueues(); }
};


class NotRegisteredMessageListener : public MessageListener<NotRegisteredMessage>
{
private:
	void HandleMessage(const MessagePtr message)
	{
		std::cout << "Got unregistered message: " << message->information << "\n";
	}
};



int main()
{
	MemeListener ml1, ml2;
	TwoChannelListener tcl;

	ml1.ReceiveMessageAsync(gMessageQueue::CreateMessage<MemeMessage>("Test"));
//	auto ml3 = std::move(ml1);

	gMessageQueue::SendMessageAsync<MemeMessage>("One does not simply use templates without kilobytes of error logs.");
	gMessageQueue::SendMessageAsync<MainChannelMessage>(4538);

	gMessageQueue2::SendMessageAsync<MemeMessage>("I created new instance of message queue but the second thread didn't appeared :(.");

	NotRegisteredMessageListener nrml;   // You can send messages bypass of message queue (directly, privately) as it is observer pattern based system.
	auto msg = std::make_shared<NotRegisteredMessage>();
	msg->information = 42;
	nrml.ReceiveMessageAsync(msg);


	gMessageQueue::SendMessageSync<MainChannelMessage>(666);

	std::cin.get();
	return 0;
}

