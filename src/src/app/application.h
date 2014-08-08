#ifndef APPLICATION_H
#define APPLICATION_H

#include "userdef.h"

class WaterSampler;
class TcpTransmiter;

class Application {
public:
	Application();
	~Application();

public:
	void init();
	void uninit();
	void exit();
	
public:
	TcpTransmiter *getTransmiter()
	{
		return mTcpTransmiter;
	}
	
private:
	WaterSampler *mWaterSampler;
	TcpTransmiter *mTcpTransmiter;

public:
	app_ctx_t mContext;
};

extern Application theApp;
#endif // APPLICATION_H

