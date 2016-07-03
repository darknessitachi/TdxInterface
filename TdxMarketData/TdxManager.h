#ifndef TDXW_MANAGER_H
#define TDXW_MANAGER_H

#include <iostream>
#include <vector>
#include <list>

#include "TdxApi.h"
#include "stdafx.h"
#include "ProtoBufMsgHub.h"

#define RESULT_SIZE 1024*1024
#define ERROR_INFO_SIZE 512

using namespace std;

class TdxManager {
public:
	TdxManager();
	~TdxManager();

	int logon();
	void initialize();
	int startService();

private:
	const static int initLatency = 50;
	const static int frequency = 1000;

	static ProtoBufMsgHub msgHub;
	static string sendAddress;

	static list<DataRequest> subBuf;
	
	static int clientID;
	static CTdxApi TdxApi;

	//don't make it local, it will cause stack overflow
	static char resultQueue[RESULT_SIZE];
	static char resultSendOrder[RESULT_SIZE];
	static char errInfo[ERROR_INFO_SIZE];

	static void getQuote(DataRequest&);
	int onMsg(MessageBase msg);
	static VOID CALLBACK queueing(PVOID lpParam, BOOLEAN TimerOrWaitFired);
	inline static string getTime();
};

#endif