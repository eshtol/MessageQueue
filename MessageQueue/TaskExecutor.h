#pragma once

#include "IExecutable.h"
#include <memory>


class TaskScheduler
{
public:
	static void ExecuteTask(std::shared_ptr<IExecutable> task) { task->execute(); }
};
