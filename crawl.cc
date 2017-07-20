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
#define NOTDEBUG

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
		case error: std::cout << now << "\033[1;31mError: \033[0m" << s << std::endl; break;
		case warning: std::cout << now << "\033[1;34mWarning: \033[0m" << s << std::endl; break;
	}
}

//Initialize Log
static Log log;

class TaskQueue{
//Serve as the Url waiting list
//Act as a deque, with the ability to eliminate the same url in it

public:
	TaskQueue(): curr(0) {}
	~TaskQueue() = default;
	//As the unique TaskQueue, it should have any duplicate
	TaskQueue(const TaskQueue&) = delete;
	TaskQueue& operator=(const TaskQueue&) = delete;
	size_t size() { std::lock_guard<std::mutex> g(lock); return urllist.size() - curr; }
	void push(const std::string &s) { std::lock_guard<std::mutex> g(lock); if(!has(s)) urllist.push_back(s); };
	//pop() has two return call, so two unlock call are needed
	std::string pop() { std::lock_guard<std::mutex> g(lock);
		if(curr == urllist.size()) { return std::string(); }
		return urllist[curr++]; };
private:
	constexpr bool has(const std::string &s) const 
		{ for(auto &c: urllist) if(c == s) return true; return false; }
	std::vector<std::string> urllist;
	std::mutex lock;
	size_t curr;
};

//Initialize Taskqueue
static TaskQueue taskqueue;

class Tasker {
public:
	Tasker() = default;
	~Tasker() = default;
	void operator()(const unsigned) const;
	std::string download(const std::string &s) const { return std::string(s); }
	//Include the saving process
	void pmatch(const std::string &s) const { std::cout << s << std::endl; }
};

//Initialize Tasker
static Tasker tasker;

class TaskerPool {
//Construct function accordig to thread creating process
	friend class Tasker;
	friend void debug();
public:
	TaskerPool() = default;
	~TaskerPool() = default;
	//As the unique TaskerPool, it should have any duplicate
	TaskerPool(TaskerPool&) = delete;
	TaskerPool &operator=(const TaskerPool&) = delete;
	//Return number of running threads
	constexpr unsigned count() const { int sum = 0;
		for(auto &i: flags) {sum += i;} return sum; }
	void run(const unsigned);
private:
	std::vector<std::thread> taskers;
	//1 for ACTIVE, 0 for IDLE
	//For the convenience of statics
	//std::vector<std::atomic<unsigned short>> flags;
	std::vector<unsigned short> flags;
};

void TaskerPool::run(const unsigned thr_cnt) {
	//Creating...
	for(unsigned index = 0; index != thr_cnt; ++index){
		taskers.push_back(std::thread(tasker,index));
		flags.push_back(TASKER_ACTIVE);
	}

	for(auto &thread: taskers)
		thread.join();
}

//Initialize TaskerPool
static TaskerPool taskerpool;

void Tasker::operator()(const unsigned id) const {
	std::string url;
	while(!(taskqueue.size() == 0 && taskerpool.count() == 0)){ 
		if((url = taskqueue.pop()) == std::string()){
			taskerpool.flags[id] = TASKER_IDLE;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		} else {
			taskerpool.flags[id] = TASKER_ACTIVE;
			std::string  page = download(url);
			pmatch(page);
		}
	}
	
}

class Scheduler {
//Designed to control the Tasker, handle the error, interact with user, and manage scale
//May add monitoring function later...

public:
	Scheduler() = default;
	~Scheduler() = default;
	//As the unique Scheduler, it should have any duplicate
	Scheduler(const Scheduler&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;
	void run(const std::string &s, const unsigned thr_cnt) const 
	{ taskqueue.push(s); taskerpool.run(thr_cnt); }
};

#ifndef NOTDEBUG
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
	taskqueue.push("test1");
	taskqueue.push("test2");
	assert(taskqueue.size() == 0);
	log("TaskQueue pass");

	//TaskerPool and Scheduler
	log("Test TaskerPool and Scheduler");
	TaskerPool taskertest;
	for(unsigned index = 0; index != 4; ++index){
		taskertest.taskers.push_back(std::thread([&taskertest](const unsigned i)
					{std::this_thread::sleep_for(std::chrono::milliseconds(500));
					taskertest.flags[i] = TASKER_IDLE;},index));
		taskertest.flags.push_back(TASKER_ACTIVE);
	}
	assert(taskertest.count() == 4);
	for(auto &thread: taskertest.taskers)
		thread.join();
	assert(taskertest.count() == 0);
	log("TaskerPool and Scheduler pass");
}
#endif

int main(int argc,char *argv[]){
	//Initialize Scheduler
	Scheduler scheduler;

	//Start scheduler
	scheduler.run("www.baidu.com",4);

#ifndef NOTDEBUG
	debug();
#endif

	return 0;
}
