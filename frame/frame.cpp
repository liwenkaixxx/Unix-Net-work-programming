/*************************************************************************
 > File Name: frame.cpp
 > Created Time: 2016/11/01 18:39:23
 ************************************************************************/

#include <syscall.h>
#include <sys/time.h>
#include <signal.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include "logger/logger.h"
#include "flags.h"
#include "plugins.h"
#include "connect_handler.h"
#include "request_handler.h"
#include "request_handler_http.h"
#include "task_thread.h"
#include "server_conf.h"
#include "global_resouce.h"
#include "frame.h"
#include "main_flow.h"

#include "crash_signal_handler.h"

int EnvInitialize(int argc, char** argv, int flag=1)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    //后台运行
    if (flag && FLAGS_d)
        daemon(1, 1);

    //初始化配置
    if (0 > ServerConf::Initialize())
    {
        std::cerr << "Initializing config error!" << std::endl; 
        return -1;
    }

    ProcInitialize();

    //初始化日志
    GlogInitialize();

	std::string conf;
	ServerConf::ToString(conf);
	LOG(LEVEL_I)<<conf;

	//初始化全局状态服务
	global_init();

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sig_stop);
    signal(SIGINT, sig_stop);

    if(ServerConf::crash_info)
    {
        CrashSignalHandler::Initialize(ServerConf::log.c_str());
    }

    return 0;

}

int FrameInitialize()
{
    LOG(LEVEL_I) << "Initializing ...";

    int ret = -1;
    //组件初始化
    ret = Plugins::Initialize();
    LOG_CHECK(ret>=0, LEVEL_I, LEVEL_E, "Initializing plugins:" << ret )

	if(ServerConf::http_port)
	{
    	ret = g_http_connect.Initialize(ServerConf::http_port);
    	LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Initializing http_connect:" << ret << " port:" << ServerConf::http_port );

		ret = RequestHandlerHttp::Initialize();
		LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Initializing http_handler:" << ret );
	}
	if(ServerConf::work_port)
	{
		ret = g_binary_connect.Initialize(ServerConf::work_port);
		LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Initializing binary_connect:" << ret << " port:" << ServerConf::work_port );
        
    	ret = RequestHandler::Initialize();
    	LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Initializing binary_handler:" << ret );
	}
	
    ret = TaskThread::Initialize();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Initializing task_threads:" << ret );

    return ret;
}
int FrameRun()
{
    //启动组件工作线程

    LOG(LEVEL_I) << "Start up working threads ...";

    int ret;
    ret = Plugins::Update();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up plugins_updating:" << ret );

    ret = TaskThread::Run();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up task_threads:" << ret );
	if(ServerConf::http_port)
	{
	    ret = RequestHandlerHttp::Run();
	    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up http_request_handler:" << ret );
	    ret = g_http_connect.Run();
	    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up http_connect:" << ret );
	}
	if(ServerConf::work_port)
	{
		ret = RequestHandler::Run();
		LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up binary_request_handler:" << ret );
    	ret = g_binary_connect.Run();
    	LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up binary_connect:" << ret );
	}

    ret = ServerConf::Update();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret, "Start up config_updating:" << ret );
    return ret;
}
int FrameRunCmdLine()
{
    return TaskThread::RunCmdline();
}
int FrameWait()
{
    //等待停止信号，并调用组件状态更新
    Wait();
    return 0;
}

int FrameStop()
{
    //停止组件线程
    LOG(LEVEL_I) << "Stop working threads ... ";
    ServerConf::Stop();
    LOG(LEVEL_I) << "Config_updating stopped";
	if(ServerConf::http_port)
	{
		g_http_connect.Stop();
        LOG(LEVEL_I) << "Http_connect stopped";
		RequestHandlerHttp::Stop();
        LOG(LEVEL_I) << "Http_request_handler stopped";
	}
	if(ServerConf::work_port)
	{
        g_binary_connect.Stop();
        LOG(LEVEL_I) << "Binary_connect stopped";

		RequestHandler::Stop();
        LOG(LEVEL_I) << "Binary_request_handler stopped";
	}
    TaskThread::Stop();
    LOG(LEVEL_I) << "Task_threads stopped";

    Plugins::Stop();
    LOG(LEVEL_I) << "Plugins_updating stopped";
    return 0;
}

int FrameDestroy()
{
    //销毁组件
    LOG(LEVEL_I) << "Destroy ... ";
    TaskThread::Destroy();
    LOG(LEVEL_I) << "Task_threads destroryed";

	if(ServerConf::http_port)
	{
	    RequestHandlerHttp::Destroy();
        LOG(LEVEL_I) << "Http_request_handler destroryed";
    	g_http_connect.Destroy();
        LOG(LEVEL_I) << "Http_connect destroryed:";
	}
	if(ServerConf::work_port)
	{
	    RequestHandler::Destroy();
        LOG(LEVEL_I) << "Binary_request_handler destroryed";
   	 	g_binary_connect.Destroy();
        LOG(LEVEL_I) << "Binary_connect destroryed";
	}

    Plugins::Destroy();
    LOG(LEVEL_I) << "Plugins destroryed";

    global_destory();

    if(ServerConf::crash_info)
    {
        CrashSignalHandler::Reset();
    }

    ServerConf::Destroy();
    LOG(LEVEL_I) << "ServerConf destroryed:";
    return 0;
}

int _main(int argc, char** argv)
{
    LOG(LEVEL_I) << "Start up ...";

    int ret = 0;

    ret = EnvInitialize(argc, argv);
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "EnvInitialize:" << ret )

    ret = FrameInitialize();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "FrameInitialize:" << ret )

    ret = FrameRun();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "FrameRun:" << ret )

    if (ServerConf::run_mode != 0x2)
    {
        LOG(LEVEL_I) << "Service mode ...";
        ret = FrameWait();
        LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "FrameWait:" << ret )
    }
    else
    {
        LOG(LEVEL_I) << "Command-line mode ...";
        ret = FrameRunCmdLine();
        LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "FrameRunCmdLine:" << ret )
    }

    ret = FrameStop();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "FrameStop :" << ret )

    ret = FrameDestroy();
    LOG_CHECK_RETURN(ret>=0, LEVEL_I, LEVEL_E, ret,  "FrameDestroy :" << ret )

    LOG(LEVEL_I) << "Stopped";
    return ret;
}
