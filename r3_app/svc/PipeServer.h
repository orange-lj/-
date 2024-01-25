#pragma once
#include<windows.h>
#include "../common/list.h"
#include "map.h"
#include"pool.h"
#include"msgids.h"

extern "C" const ULONG tzuk;


class PipeServer
{
public:
	//返回全局PipeServer的实例
	static PipeServer* GetPipeServer();
	//子服务器的处理程序功能原型
	typedef MSG_HEADER* (*Handler)(void* context, MSG_HEADER* msg);

	//为已知的请求消息id注册处理程序函数。如果模拟=TRUE，则在模拟调用客户端后将调用子服务器处理程序
	void Register(
		ULONG serverId, void* context, Handler handler);

protected:
	//私有构造函数
	PipeServer();
	//私有初始化程序
	bool Init();
protected:
	LIST m_targets;
	HASH_MAP m_client_map;
	CRITICAL_SECTION m_lock;
	POOL* m_pool;
	ULONG m_TlsIndex;
	volatile HANDLE m_hServerPort;
	HANDLE* m_Threads;
	static PipeServer* m_instance;
};

