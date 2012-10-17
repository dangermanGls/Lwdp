
#include <LwDp.h>
#include <LwApiLib/LwApi/win32/Api4Win32.h>


LWDP_NAMESPACE_BEGIN;
EXTERN_C_BEGIN;

/****************************************************************************
*  Function:       WIN_TaskDelay
*  Description:    
*  Input:          ��
*  Output:         // �����������˵��
*  Return:         // ��������ֵ��˵��	
*  Others:         // ����˵��
*****************************************************************************/
void WIN_IMPL_API(TaskDelay)(ulong_ tick)
{
    ::Sleep(tick);
}

/****************************************************************************
*  Function:       WIN_HaltSystem
*  Description:    
*  Input:          ��
*  Output:         // �����������˵��
*  Return:         // ��������ֵ��˵��	
*  Others:         // ����˵��
*****************************************************************************/
void WIN_IMPL_API(HaltSystem)()
{
    while(1) {::Sleep(1000);}    
}

EXTERN_C_END;
LWDP_NAMESPACE_END;