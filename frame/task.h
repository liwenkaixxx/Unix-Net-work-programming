/*************************************************************************
 > File Name: task.h

 ************************************************************************/

#ifndef __TASK_H__
#define __TASK_H__

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include "proto/message_head.h"
#include "utils/element_queue.h"
#include "task_base.h"

using namespace search_message;

class Task: public TaskBase
{
public:
	request_head rq_head;
	response_head rp_head;
	uint64_t timeflag;

	char* rq_body;
	int32_t rq_size;
	char* rp_body;
	int32_t rp_size;

	const static int32_t dump_head_size = sizeof(rq_head) + sizeof(rp_head)
			+ 64;
	const static int32_t min_buf_size = dump_head_size * 2;

public:
	Task()
	{
		buf = NULL;
		rq_body = NULL;
		rp_body = NULL;
		rq_size = 0;
		rp_size = 0;
		buf_size = 0;
		buf_left = 0;
		timeflag = 0;
	}
	virtual ~Task()
	{
	}

	void Reset()
	{
		if (buf)
		{
			memset(&rq_head, 0, sizeof(rq_head));
			memset(&rp_head, 0, sizeof(rp_head));
			rq_body = buf + dump_head_size;
			rq_size = 0;
			rp_body = NULL;
			rp_size = 0;
			buf_left = buf_size - dump_head_size;
			timeflag = 0;
		}
	}
	virtual int Dump(FILE* fp)
	{
		if (fp == NULL)
			return -1;

		char* addr = NULL;
		int32_t size = 0;
		GetData(addr, size);

		fwrite(&size, sizeof(size), 1, fp);
		fwrite(addr, 1, size, fp);

		return size + sizeof(size);
	}
	virtual int Load(FILE* fp)
	{
		if (fp == NULL)
			return -1;

		char* addr = buf;
		size_t size = 0;

		if (1 != fread(&size, sizeof(size), 1, fp))
			return -1;
		if (size != fread(addr, 1, size, fp))
			return -1;

		SetData(addr, size);
		return 0;
	}

	virtual void SetBuf(char* addr, int32_t size)
	{
		buf = addr;
		buf_size = size;
		buf_left = size;
		Reset();
		return;
	}
	virtual void GetBuf(char*& addr, int32_t& size)
	{
		addr = buf;
		size = buf_size;
		return;
	}
	virtual void SetData(char* data, int32_t size)
	{
		static int head_size = sizeof(rq_head) + sizeof(rp_head);

		if (size > buf_size)
			return;
		if (buf != data)
		{
			memmove(buf, data, size);
		}
		buf_left = buf_size - size;

		char* q;
		char* base;
		char* p = buf;

		base = *(char**) p;
		p += sizeof(char*);

		memcpy(&rq_head, p, sizeof(rq_head));
		p += sizeof(rq_head);
		memcpy(&rp_head, p, sizeof(rp_head));
		p += sizeof(rp_head);
		timeflag = *(uint64_t*)p;
		p += sizeof(uint64_t);

		q = *(char**) p;
		rq_body = (q==NULL) ? NULL : buf + (q - base);
		p += sizeof(char*);
		rq_size = *(int32_t*) p;
		p += sizeof(int32_t);

		q = *(char**) p;
		rp_body = (q==NULL) ? NULL : buf + (q - base);
		p += sizeof(char*);
		rp_size = *(int32_t*) p;
		p += sizeof(int32_t);


		return;
	}
	virtual void GetData(char*& data, int32_t& size)
	{
		data = buf;
		size = buf_size - buf_left;

		char* p = buf;
		memcpy(p, &rq_head, sizeof(rq_head));

		*(char**) p = buf;
		p += sizeof(char*);

		memcpy(p, &rq_head, sizeof(rq_head));
		p += sizeof(rq_head);
		memcpy(p, &rp_head, sizeof(rp_head));
		p += sizeof(rp_head);
		*(uint64_t*)p = timeflag;
		p += sizeof(uint64_t);

		*(char**) p = rq_body;
		p += sizeof(char*);
		*(int32_t*) p = rq_size;
		p += sizeof(int32_t);

		*(char**) p = rp_body;
		p += sizeof(char*);
		*(int32_t*) p = rp_size;
		p += sizeof(int32_t);

		return;
	}
	int SetRequest(char* data, int32_t size)
	{
		if (size >= buf_left)
			return -1;

		if (rq_body != data)
		{
			memmove(rq_body, data, size);
		}
		rq_body[size] = 0;
		rq_size = size;
		buf_left -= size + 1;
		return 0;
	}

};

class TaskQueue
{
public:
	static int Initialize();
	static int Put(Task* task, int timeout);
	static int Pop(Task*& task);
	static void Destroy();
	static int length();
protected:
	static int _queue_size;
	static int _rtimeout;
	static ElementQueue<Task*> _task_queue;
	static pthread_mutex_t _mutex;
private:
	TaskQueue();
};

#endif //__TASK_H__



