#ifndef PERFMGR_INTERFACE_H
#define PERFMGR_INTERFACE_H

#include <Interface/Ix_Object.h>
#include "Id_PerfMgr.h"

LWDP_NAMESPACE_BEGIN;


INTERFACE_BEGIN(PerfMgr_Cps)
	virtual double GetCps() = 0;
	virtual void   Update() = 0;
	virtual void   Reset()  = 0;
	virtual double GetKbps() = 0;
	virtual void   Update(uint32_ mbyte) = 0;
INTERFACE_END();

INTERFACE_BEGIN(PerfMgr_timer)
	virtual void   Start()  = 0;
	virtual uint32_ Now()  = 0;
	virtual uint32_ End() = 0;
INTERFACE_END();


INTERFACE_BEGIN(PerfMgr)
	virtual LWRESULT Init() = 0;

INTERFACE_END();


LWDP_NAMESPACE_END;

#endif // LOGMGR_INTERFACE_H
