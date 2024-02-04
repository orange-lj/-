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
	//����ȫ��PipeServer��ʵ��
	static PipeServer* GetPipeServer();

	//�������ע��������ܵ�������
	bool Start();

	//�ӷ������Ĵ��������ԭ��
	typedef MSG_HEADER* (*Handler)(void* context, MSG_HEADER* msg);

	//Ϊ��֪��������Ϣidע�ᴦ������������ģ��=TRUE������ģ����ÿͻ��˺󽫵����ӷ������������
	void Register(
		ULONG serverId, void* context, Handler handler);

	//������д���Ķ̻ظ���Ϣ�ӷ���������ʹ��short_REPPLY��
	MSG_HEADER* AllocShortMsg(ULONG status);

	//�������� / �ظ���Ϣ������
	MSG_HEADER* AllocMsg(ULONG length);

	//�ͷ����� / �ظ���Ϣ������
	void FreeMsg(MSG_HEADER* msg);

	//��ȡ���÷��Ľ���id
	static ULONG GetCallerProcessId();

	//��ȡ�����ߵĻỰid
	static ULONG GetCallerSessionId();

	//ģ����÷���ȫ������
	static ULONG ImpersonateCaller(MSG_HEADER** pmsg = NULL);

protected:
	//˽�й��캯��
	PipeServer();
	//˽�г�ʼ������
	bool Init();
	//�̺߳����ľ�̬��װ��
	static void ThreadStub(void* parm);
	//��������hServerPort���̺߳���
	void Thread(void);
	//Port Connect
	void PortConnect(PORT_MESSAGE * msg);
	//Port Request
	void PortRequest(
		HANDLE PortHandle, PORT_MESSAGE* msg, void* voidClient);
	//Port Reply
	void PortReply(PORT_MESSAGE* msg, void* voidClient);
	//�˿ڲ��ҿͻ���
	void* PortFindClient(PORT_MESSAGE* msg);
	//����ע����ӷ�����
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

