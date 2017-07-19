#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <ctime>
#include <set>
#include <deque>
#include <cassert>
#define TASKER_ACTIVE 1
#define TASKER_IDLE 0
#define DEBUG

class Log{
//Serve as the log function
public:
	Log() = default;
	~Log() = default;
	//As the unique Log, it should have any duplicate
	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;
	enum logstate {normal, error, warning};
	void operator()(const std::string&, const logstate) const;
};

void Log::operator()(const std::string &s, const logstate state = normal) const {
	std::time_t result = std::time(nullptr);
	char now[25];
	std::strftime(now,25,"[%F %T]",std::localtime(&result));
	switch(state){
		case normal: std::cout << now << s << std::endl; break;
		default: break;
	}
}

//Initialize Log
static Log log;

class TaskQueue{
//Serve as the TaskQueue to store the urls, containing essential webpage or resources needed to be downloaded later
//FIFO container
//When size() return 0 or pop() return empty string, TaskQueue is empty

public:
	TaskQueue() = default;
	~TaskQueue() = default;
	//As the unique TaskQueue, it should have any duplicate
	TaskQueue(const TaskQueue&) = delete;
	TaskQueue& operator=(const TaskQueue&) = delete;
	constexpr size_t size() const { return queue.size(); }
	void push(const std::string &s) { queue.push_back(s); };
	std::string pop() { if(queue.empty()) return std::string();
		std::string tmp(queue.front()); 
		queue.pop_front(); return tmp; };
private:
	std::deque<std::string> queue;
};

//Initialize Taskqueue
static TaskQueue taskqueue;

class Storage{
//Serve as a Register Table
//Using Hash Table to maintain the stroed file infomatin(MD5)
//Regardless the actual file-store-to-disk process

public:
	Storage() = default;
	~Storage() = default;
	//As the unique Storage, it should have any duplicate
	Storage(const Storage&) = delete;
	Storage& operator=(const Storage&) = delete;
	constexpr size_t size() const { return files.size(); }
	void store(const std::string &file) { files.insert(file); }
	bool have(const std::string &s) const { 
		return (files.find(s) == files.cend())? false: true; };
private:
	std::set<std::string> files;
};

//Initialize Storage
static Storage storage;

class Scheduler {
//Designed to control the Tasker, handle the error, interact with user, and manage scale
//May add monitoring function later...

public:
	Scheduler() = default;
	~Scheduler() = default;
	//As the unique Scheduler, it should have any duplicate
	Scheduler(const Scheduler&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;
	void run(const unsigned) const;
};

class Tasker {
	friend void Scheduler::run(const unsigned) const;
public:
	void operator()(const unsigned) const;
};

void Tasker::operator()(const unsigned id) const {
	//Wait
	//while(!(taskerpool.count() == 0 && taskqueue.size() == 0)){ /*Do nothing*/ }
	std::cout << id << std::endl;
}

//Initialize Tasker
static Tasker tasker;

class TaskerPool {
//Construct function accordig to thread creating process
	friend class Tasker;
	friend void debug();
	friend void Scheduler::run(const unsigned) const;
public:
	TaskerPool() = default;
	~TaskerPool() = default;
	//As the unique Scheduler, it should have any duplicate
	TaskerPool(TaskerPool&) = delete;
	TaskerPool &operator=(const TaskerPool&) = delete;
	//Return number of running threads
	unsigned count() const { int sum = 0;
		for(auto &i: flags) {sum += i;} return sum; }
private:
	std::vector<std::thread> taskers;
	//1 for ACTIVE, 0 for IDLE
	//For the convenience of statics
	std::vector<unsigned short> flags;
};

//Initialize Scheduler
static TaskerPool taskerpool;

void Scheduler::run(const unsigned thr_cnt) const {
	//Creating...
	for(unsigned index = 0; index != thr_cnt; ++index){
		taskerpool.taskers.push_back(std::thread(tasker,index));
		taskerpool.flags.push_back(TASKER_ACTIVE);
	}

	for(auto &thread: taskerpool.taskers)
		thread.join();
}

//Initialize Scheduler
static Scheduler scheduler;

#ifdef DEBUG
void debug(){
	//TaskQueue
	log("Test the TaskQueue");
	assert(taskqueue.size() == 0);
	assert(taskqueue.pop() == std::string());
	taskqueue.push("test1");
	taskqueue.push("test2");
	assert(taskqueue.size() == 2);
	assert(taskqueue.pop() == "test1");
	assert(taskqueue.pop() == "test2");
	assert(taskqueue.size() == 0);
	assert(taskqueue.pop() == std::string());
	assert(taskqueue.size() == 0);
	log("TaskQueue pass");

	//Storage
	log("Test the Storage");
	assert(storage.size() == 0);
	storage.store("test.txt");
	assert(storage.have("test.txt"));
	storage.store("test2.txt");
	assert(storage.have("test2.txt"));
	assert(storage.have("test.txt"));
	storage.store("test2.txt");
	assert(storage.size() == 2);
	log("Storage pass");

	//TaskerPool and Scheduler
	log("Test TaskerPool and Scheduler");
	TaskerPool taskertest;
	for(unsigned index = 0; index != 4; ++index){
		taskertest.taskers.push_back(std::thread([]
					{std::this_thread::sleep_for(std::chrono::milliseconds(1000));}));
		taskertest.flags.push_back(TASKER_ACTIVE);
	}
	assert(taskertest.count() == 4);
	for(auto &thread: taskertest.taskers)
		thread.join();
	log("TaskerPool and Scheduler pass");
}
#endif

int main(int argc,char *argv[]){
	//Start scheduler
	scheduler.run(4);

#ifdef DEBUG
	debug();
#endif

	return 0;
}
