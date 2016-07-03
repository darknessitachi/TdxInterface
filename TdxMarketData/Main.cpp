#include "stdafx.h"

#include "TdxManager.h"
#include "CedarLogging.h"
#include "CedarJsonConfig.h"
#include <fstream>

int main() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	CedarLogging::init("TdxMarketData");
	CedarJsonConfig::getInstance().loadConfigFile("TdxMarketData.json");
	
	TdxManager tdm;
	tdm.initialize();
	tdm.logon();

	if (tdm.startService() != 0) {
		LOG(FATAL) << "Error Happened in Start Service, please check.";
	}

	//TODO
	getchar();
	return 0;
}
