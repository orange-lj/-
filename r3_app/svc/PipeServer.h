#pragma once
#include<windows.h>
#include "../common/list.h"
#include "map.h"
#include"pool.h"
#include"msgids.h"
#include"misc.h"

extern "C" const ULONG tzuk;

#define LONG_REPLY(ln)  (PipeServer::GetPipeServer()->AllocMsg(ln))
#define SHORT_REPLY(st) (PipeServer::GetPipeServer()->AllocShortMsg(st))

class PipeServer
{
public:
	//返回全局PipeServer的实例
	static PipeServer* GetPipeServer();

	//完成所有注册后启动管道服务器
	bool Start();

	//子服务器的处理程序功能原型
	typedef MSG_HEADER* (*Handler)(void* context, MSG_HEADER* msg);

	//为已知的请求消息id注册处理程序函数。如果模拟=TRUE，则在模拟调用客户端后将调用子服务器处理程序
	void Register(
		ULONG serverId, void* context, Handler handler);

	//制造带有错误的短回复消息子服务器可以使用short_REPPLY宏
	MSG_HEADER* AllocShortMsg(ULONG status);

	//分配请求 / 回复消息缓冲区
	MSG_HEADER* AllocMsg(ULONG length);

	//释放请求 / 回复消息缓冲区
	void FreeMsg(MSG_HEADER* msg);

	//获取调用方的进程id
	static ULONG GetCallerProcessId();

	//获取呼叫者的会话id
	static ULONG GetCallerSessionId();

	//模拟调用方安全上下文
	static ULONG ImpersonateCaller(MSG_HEADER** pmsg = NULL);

protected:
	//私有构造函数
	PipeServer();
	//私有初始化程序
	bool Init();
	//线程函数的静态包装器
	static void ThreadStub(void* parm);
	//用于侦听hServerPort的线程函数
	void Thread(void);
	//Port Connect
	void PortConnect(PORT_MESSAGE * msg);
	//Port Request
	void PortRequest(
		HANDLE PortHandle, PORT_MESSAGE* msg, void* voidClient);
	//Port Reply
	void PortReply(PORT_MESSAGE* msg, void* voidClient);
	//端口查找客户端
	void* PortFindClient(PORT_MESSAGE* msg);
	//调用注册的子服务器
	MSG_HEADER* CallTarget(
		MSG_HEADER* msg, HANDLE PortHandle, PORT_MESSAGE* PortMessage);
protected:
	void PortFindClientUnsafe(const CLIENT_ID& ClientId, struct tagCLIENT_PROCESS*& clientProcess, struct tagCLIENT_THREAD*& clientThread);
	LIST m_targets;
	HASH_MAP m_client_map;
	CRITICAL_SECTION m_lock;
	POOL* m_pool;
	ULONG m_TlsIndex;
	volatile HANDLE m_hServerPort;
	HANDLE* m_Threads;
	static PipeServer* m_instance;
};

