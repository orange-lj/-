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
	//����ȫ��PipeServer��ʵ��
	static PipeServer* GetPipeServer();
	//�ӷ������Ĵ��������ԭ��
	typedef MSG_HEADER* (*Handler)(void* context, MSG_HEADER* msg);

	//Ϊ��֪��������Ϣidע�ᴦ������������ģ��=TRUE������ģ����ÿͻ��˺󽫵����ӷ������������
	void Register(
		ULONG serverId, void* context, Handler handler);

protected:
	//˽�й��캯��
	PipeServer();
	//˽�г�ʼ������
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

