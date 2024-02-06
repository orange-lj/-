#include "pch.h"
#include "AutoPlay.h"
#include"../common/my_version.h"

static GUID CLSID_MyAutoPlay = { MY_AUTOPLAY_CLSID };

HRESULT AutoPlay::QueryInterface(REFIID riid, void** ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IQueryCancelAutoPlay)) {

        ++m_refcount;
        *ppv = this;
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG AutoPlay::AddRef()
{
    return ++m_refcount;
}

void AutoPlay::Install()
{
    HRESULT hr;
    IMoniker* pMoniker;
    IRunningObjectTable* pRunningObjectTable;
    ULONG cookie;

    AutoPlay* pAutoPlay = new AutoPlay();
    if (!pAutoPlay) 
    {
        return;
    }
    AutoPlay::m_instance = pAutoPlay;
    pAutoPlay->m_refcount = 1;
    pAutoPlay->m_pMoniker = NULL;
    pAutoPlay->m_pRunningObjectTable = NULL;
    pAutoPlay->m_cookie = 0;
    //创建com类对象标识符
    hr = CreateClassMoniker(CLSID_MyAutoPlay, &pMoniker);
    if (FAILED(hr))
        return;
    pAutoPlay->m_pMoniker = pMoniker;
    //获取运行中对象表的实例
    hr = GetRunningObjectTable(0, &pRunningObjectTable);
    if (FAILED(hr))
        return;
    pAutoPlay->m_pRunningObjectTable = pRunningObjectTable;
    //将对象 pAutoPlay 与标识符 pMoniker 关联并注册到运行中对象表中
    hr = pRunningObjectTable->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,
        pAutoPlay,
        pMoniker,
        &cookie);
    if (FAILED(hr))
        return;
    pAutoPlay->m_cookie = cookie;
}
