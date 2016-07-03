#include <chrono>
#include "stdafx.h"
#include "TdxManager.h"
#include "boost\date_time\posix_time\posix_time.hpp"

using namespace std;

CTdxApi TdxManager::TdxApi;
string TdxManager::sendAddress;
int TdxManager::clientID;

char TdxManager::errInfo[ERROR_INFO_SIZE];
char TdxManager::resultQueue[RESULT_SIZE];
char TdxManager::resultSendOrder[RESULT_SIZE];

list<DataRequest> TdxManager::subBuf;
ProtoBufMsgHub TdxManager::msgHub;

TdxManager::TdxManager() {
}

TdxManager::~TdxManager() {
}

int TdxManager::logon()	{

	std::string ip;
	short port;
	std::string version;
	short yybID;
	std::string accountNo;
	std::string tradeAccount;
	std::string jyPassword;
	std::string txPassword;
	
	short tmp;
	string stmp;

	CedarJsonConfig::getInstance().getStringByPath("Logon.IP_Address", stmp);
	ip = stmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.Version", stmp);
	version = stmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.AccountNo", stmp);
	accountNo = stmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.TradeAccountNo", stmp);
	tradeAccount = stmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.JyPassword", stmp);
	jyPassword = stmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.TxPassword", stmp);
	txPassword = stmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.Port", stmp);
	tmp = atoi(stmp.c_str());
	port = tmp;
	CedarJsonConfig::getInstance().getStringByPath("Logon.YybID", stmp);
	tmp = atoi(stmp.c_str());
	yybID = tmp;

	LOG(INFO) << "Login Configuration:" << endl;
	LOG(INFO) << "IP" << ": " << ip << endl;
	LOG(INFO) << "Port" << ": " << port << endl;
	LOG(INFO) << "Version" << ": " << version << endl;
	LOG(INFO) << "YybID" << ": " << yybID << endl;
	LOG(INFO) << "AccountNo" << ": " << accountNo << endl;
	LOG(INFO) << "TradeAccount" << ": " << tradeAccount << endl;
	LOG(INFO) << "JyPassword" << ": " << jyPassword << endl;
	LOG(INFO) << "TxPassword" << ": " << txPassword << endl;

	clientID = TdxApi.Logon((char*)ip.c_str(), port, (char*)version.c_str(), yybID, 
		(char*)accountNo.c_str(), (char*)tradeAccount.c_str(), (char*)jyPassword.c_str(), (char*)txPassword.c_str(), errInfo);
	if (clientID == -1) {	
		LOG(FATAL) << "Logon failed. " << errInfo;
		getchar();
		exit(-1);
	}

	LOG(INFO) << "Login successfully!";

	return clientID;
}

void TdxManager::initialize() {
	TdxApi.OpenTdx();
	CedarJsonConfig::getInstance().getStringByPath("SendToAddress", sendAddress);
}

VOID CALLBACK TdxManager::queueing(PVOID lpParam, BOOLEAN TimerOrWaitFired)	{
	for (list<DataRequest>::iterator iter = subBuf.begin(); iter != subBuf.end(); iter++) {
		getQuote(*iter);
	}
}

int TdxManager::startService() {
	ProtoBufHelper::setupProtoBufMsgHub(msgHub);
	msgHub.registerCallback(std::bind(&TdxManager::onMsg, this, std::placeholders::_1));

	HANDLE queueEvent = NULL;
	HANDLE queueTimer = NULL;
	HANDLE queueTimerQueue;

	queueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == queueEvent) {
		LOG(ERROR) << "Create Queueing Event failed. " << GetLastError();
		return 1;
	}

	queueTimerQueue = CreateTimerQueue();
	if (NULL == queueTimerQueue) {
		LOG(ERROR) << "Create Queueing TimerQueue failed. " << GetLastError();
		return 2;
	}
	
	if (!CreateTimerQueueTimer(&queueTimer, queueTimerQueue,
		(WAITORTIMERCALLBACK)queueing, NULL, initLatency, frequency, 0)) {
		LOG(ERROR) << "Create Queueing TimerQueueTimer failed. " << GetLastError();
		return 3;
	}
	
	LOG(INFO) << "queueing service start";
	
	if (WaitForSingleObject(queueEvent, INFINITE) != WAIT_OBJECT_0)
		LOG(ERROR) << "WaitForSingleObject failed. " << GetLastError();
	
	return 0;
}

string TdxManager::getTime() {
	string s = to_iso_string(boost::posix_time::microsec_clock::local_time());
	string sTime = s.substr(9, 6) + s.substr(16, 3);
	return sTime;
}

void TdxManager::getQuote(DataRequest &dReq) {
	MarketUpdate mUpdate;

	vector<string> dataInLine, inLineData;
	
	TdxApi.GetQuote(clientID, (char*)dReq.code().c_str(), resultSendOrder, errInfo);
	
	boost::split(dataInLine, resultSendOrder, boost::is_any_of("\n\0"), boost::token_compress_on);

	if (dataInLine.size() < 2) {
		//TODO report a error info to server
		LOG(ERROR) << "GetQuote result decoding error";
		return;
	}
	
	boost::split(inLineData, dataInLine.at(1), boost::is_any_of("\t\0"), boost::token_compress_on);
	
	mUpdate.set_code(inLineData[0]);
	mUpdate.set_exchange(dReq.exchange());
	//change this to atof instead of atoi
	mUpdate.set_pre_close_price(atoi(inLineData[2].c_str()));
	mUpdate.set_open_price(atof(inLineData[3].c_str()));
	mUpdate.set_last_price(atof(inLineData[5].c_str()));
	mUpdate.set_recv_timestamp(getTime());

	mUpdate.add_bid_price(atof(inLineData[6].c_str()));
	mUpdate.add_bid_price(atof(inLineData[7].c_str()));
	mUpdate.add_bid_price(atof(inLineData[8].c_str()));
	mUpdate.add_bid_price(atof(inLineData[9].c_str()));
	mUpdate.add_bid_price(atof(inLineData[10].c_str()));

	mUpdate.add_bid_volume(atoi(inLineData[11].c_str()));
	mUpdate.add_bid_volume(atoi(inLineData[12].c_str()));
	mUpdate.add_bid_volume(atoi(inLineData[13].c_str()));
	mUpdate.add_bid_volume(atoi(inLineData[14].c_str()));
	mUpdate.add_bid_volume(atoi(inLineData[15].c_str()));

	mUpdate.add_ask_price(atof(inLineData[16].c_str()));
	mUpdate.add_ask_price(atof(inLineData[17].c_str()));
	mUpdate.add_ask_price(atof(inLineData[18].c_str()));
	mUpdate.add_ask_price(atof(inLineData[19].c_str()));
	mUpdate.add_ask_price(atof(inLineData[20].c_str()));

	mUpdate.add_ask_volume(atoi(inLineData[21].c_str()));
	mUpdate.add_ask_volume(atoi(inLineData[22].c_str()));
	mUpdate.add_ask_volume(atoi(inLineData[23].c_str()));
	mUpdate.add_ask_volume(atoi(inLineData[24].c_str()));
	mUpdate.add_ask_volume(atoi(inLineData[25].c_str()));	
	
	msgHub.pushMsg(sendAddress, ProtoBufHelper::wrapMsg(TYPE_MARKETUPDATE, mUpdate));
}

int TdxManager::onMsg(MessageBase msg) {

	DataRequest ordReq = ProtoBufHelper::unwrapMsg<DataRequest>(msg);
	subBuf.push_back(ordReq);
	
	return 0;
}
