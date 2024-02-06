#pragma once
class AutoPlay:public IQueryCancelAutoPlay
{
private:
    static AutoPlay* m_instance;
    ULONG m_refcount;

    IMoniker* m_pMoniker;
    IRunningObjectTable* m_pRunningObjectTable;
    ULONG m_cookie;
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IQueryCancelAutoPlay methods
    STDMETHOD(AllowAutoPlay)(
        const WCHAR* path, ULONG cttype, const WCHAR* label, ULONG sn);

    // static methods
    static void Install();
};

