/*! \file Cx_ConsoleMgr.cpp
 *  \brief ConsoleMgr Module Impl
 *  \author Guolisen, LwDp
 *  \date   2012.10.17
 */
 
#include <LwDp.h>
#include <PluginInc.h>
#include <iostream>

#include <Interface/ConfigMgr/Ix_ConfigMgr.h>
#include <Interface/LogMgr/Ix_LogMgr.h>

#include "ConsoleMgrDef.h"
#include "Cx_ConsoleMgr.h"



LWDP_NAMESPACE_BEGIN;
namespace CON
{
enum CALLBACK_LINE_ERR_ENUM
{
	CONSOLE_EXIT = 1
};
};
Cx_ConsoleMgr::Cx_ConsoleMgr()
{
}

Cx_ConsoleMgr::~Cx_ConsoleMgr()
{
}

LWRESULT Cx_ConsoleMgr::Init()
{
	lw_log_info(LWDP_LUA_LOG, __T("Cx_ConsoleMgr::Init OK!"));

	return LWDP_OK;
}

LWRESULT Cx_ConsoleMgr::RunConsole()
{
	LWRESULT res = 0;
	std::string lineStr("");
	std::cout << std::endl;
    while(1)
    {
    	COMMAND_LINE commandList;
        std::cout << "TS> ";
        std::getline(std::cin, lineStr);           		/*读命令*/
		if(lineStr.empty() || lineStr[0] == '\r' || lineStr[0] == '\n')
			continue;

		res = PraseCommandLine(lineStr, commandList);
        if(res != LWDP_OK)
    	{
			std::cout << "Command Line Error" << std::endl;
			continue;
    	}
        res = CallBackCommand(commandList);          /*处理命令，跳到相应函数*/
        if(res != LWDP_OK)
    	{
			if(CON::CONSOLE_EXIT == res)
			{
				std::cout << "Bye!" << std::endl;
					break;
			}
    	}
	}
	
	return LWDP_OK;
}

LWRESULT Cx_ConsoleMgr::RegisteCommand(const std::string& command_str, ConsoleCBDelegate command_fun, 
								             const std::string& command_info)
{
	if(command_str.empty() || !command_fun)
		return LWDP_PARAMETER_ERROR;

	COMMAND_STRU entry;
	entry.commandFun  = command_fun;
	entry.commandInfo = command_info;

	mCommandMap.insert(std::make_pair(command_str, entry));
	
	return LWDP_OK;
}
LWRESULT Cx_ConsoleMgr::CallBackCommand(COMMAND_LINE& command_line)	
{
	if(command_line.empty())
		return LWDP_OK;

	COMMAND_LINE::iterator lineIter = command_line.begin();
	
	if(*lineIter == "break")
		return CON::CONSOLE_EXIT;

	COMMAND_MAP::iterator iter = mCommandMap.find(*lineIter);
	if(iter == mCommandMap.end())
	{
		std::cout << "Unknow Command!" << std::endl;
		return LWDP_OK;
	}

	iter->second.commandFun(command_line);
	
	return LWDP_OK;
}
LWRESULT Cx_ConsoleMgr::PraseCommandLine(const std::string& command_str, COMMAND_LINE& command_line)
{
	std::string match(" ");
	bool removeEmpty = true;
    std::string::size_type start = 0;           // starting position for searches
    std::string::size_type skip  = 1;            // positions to skip after a match

	command_line.clear();
    skip = match.length();
    while (start != std::string::npos)
    {
        // get a complete range [start..end)
        std::string::size_type end = command_str.find_first_of(match, start);

        // null strings always match in string::find, but
        // a skip of 0 causes infinite loops. pretend that
        // no tokens were found and extract the whole string
        if (skip == 0) end = std::string::npos;

        std::string token = command_str.substr(start, end - start);

        if (!(removeEmpty && token.empty()))
        {
            // extract the token and add it to the result list
            command_line.push_back(token);
        }

        // start the next range
        if ((start = end) != std::string::npos) start += skip;
    }

	return LWDP_OK;
}


LWDP_NAMESPACE_END;

