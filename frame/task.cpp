/*************************************************************************
	> File Name: task.cpp
	> Created Time: 2016/10/26 19:25:49
 ************************************************************************/

#include "task.h"

#include "server_conf.h"
#include "utils/auto_mutex.h"
using namespace std;

int TaskQueue::_queue_size=100;
int TaskQueue::_rtimeout=100;
ElementQueue<Task*> TaskQueue::_task_queue;
pthread_mutex_t TaskQueue::_mutex = PTHREAD_MUTEX_INITIALIZER;

int TaskQueue::Initialize()
{
    auto_mutex locker(_mutex);
    _queue_size = ServerConf::task_queue_size;
    _rtimeout = ServerConf::task_get_timeout;
    return _task_queue.init(_queue_size);
}

int TaskQueue::Put(Task* task, int timeout)
{
    double t = timeout/1000.0;
    return _task_queue.put(task, t);
}

int TaskQueue::Pop(Task*& task)
{
    int ret = _task_queue.get(task, _rtimeout/(double)1000);
    if(ret < 0)
        task = NULL;
    return ret;
}

void TaskQueue::Destroy()
{
    _task_queue.release();
}
int TaskQueue::length()
{
	return _task_queue.length();
}


