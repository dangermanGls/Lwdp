#ifndef TIMERMGR_INTERFACE_H
#define TIMERMGR_INTERFACE_H

#include <Interface/Ix_Object.h>
#include "Id_TimerMgr.h"
#include <Interface/EventMgr/Ix_EventMgr.h>

LWDP_NAMESPACE_BEGIN;

typedef void* TimerHandle; 
typedef void (*TIMER_CALLBACK)(void* arg);


INTERFACE_BEGIN(TimerMgr)
	virtual LWRESULT Init() = 0;
	virtual TimerHandle CreateTimer(TIMER_CALLBACK call_back, void* arg, double_ delay, double_ repeat) = 0;
	virtual LWRESULT    DestoryTimer(TimerHandle timer_handle) = 0;

	virtual LWRESULT TimerStart(TimerHandle timer_handle) = 0;
	virtual LWRESULT TimerStop(TimerHandle timer_handle) = 0;
	
	
INTERFACE_END();

INTERFACE_BEGIN(TimerTick)
virtual bool SetOption(const std::string& strKey, const void* pValue) = 0;

		/** Resets timer */
		virtual void Reset() = 0;

		/** Returns milliseconds since initialisation or last reset */
		virtual uint32_ GetMilliseconds() = 0;

		/** Returns microseconds since initialisation or last reset */
		virtual uint32_ GetMicroseconds() = 0;

		/** Returns milliseconds since initialisation or last reset, only CPU time measured */
		virtual uint32_ GetMillisecondsCPU() = 0;

		/** Returns microseconds since initialisation or last reset, only CPU time measured */
		virtual uint32_ GetMicrosecondsCPU() = 0;

INTERFACE_END();

LWDP_NAMESPACE_END;

#endif // LOGMGR_INTERFACE_H
