#include "Pch.h"
#include "Task.h"

Task::Task(int time, std::function<void()> func)
{
	MsSleep = time;
	Function = func;
}

void Task::Execute()
{

	if (GetTickCount64() - LastExecution > MsSleep)
	{
		Function();
		LastExecution = GetTickCount64();
	}
}