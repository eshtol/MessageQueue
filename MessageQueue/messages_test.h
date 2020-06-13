#pragma once
#include <string>
#include <iostream>

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


struct NotRegisteredMessage 
{
	int information;
};