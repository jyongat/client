#include <signal.h>

#include "water_sampler.h"
#include "tcp_transmiter.h"
#include "logger.h"

#include "application.h"

Application theApp;

Application::Application()
{
	mContext.exit = false;
	mWaterSampler = new WaterSampler();
	ASSERT(mWaterSampler != NULL);

	mTcpTransmiter = new TcpTransmiter();
	ASSERT(mTcpTransmiter != NULL);
}

Application::~Application()
{
	if (NULL != mTcpTransmiter) {
		delete mTcpTransmiter;
		mTcpTransmiter = NULL;
	}

	if (NULL != mWaterSampler) {
		delete mWaterSampler;
		mWaterSampler =  NULL;
	}
}

void Application::init()
{
	ASSERT(mWaterSampler->mSampler->start() == true);
	ASSERT(mTcpTransmiter->mTransmiter->start() == true);
}

void Application::uninit()
{
	mTcpTransmiter->mTransmiter->stop();
	mWaterSampler->mSampler->stop();
}

void Application::exit()
{
	raise(SIGINT);
}