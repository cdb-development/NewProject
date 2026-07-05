#pragma once
class Task
{
	int MsSleep = 0;
	std::function<void()> Function;
	int LastExecution;
public:
	Task(int time, std::function<void()>func);
	void Execute();
};