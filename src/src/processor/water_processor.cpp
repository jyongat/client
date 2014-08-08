#include "logger.h"
#include "application.h"
#include "tcp_transmiter.h"

#include "water_processor.h"

/* disable water-processor debugger */
#undef DEBUG_WATER_PROCESSOR

#ifdef DEBUG_WATER_PROCESSOR
static const char *TAG = "WaterProcessor";
#endif

WaterProcessor::WaterProcessor()
{
	mProcessor = new Processor(this);
	ASSERT(mProcessor != NULL);
}


WaterProcessor::~WaterProcessor()
{
	if (NULL != mProcessor) {
		delete mProcessor;
		mProcessor = NULL;
	}

	mDatas.clear();
}

void WaterProcessor::post(void *data)
{
	ASSERT(data != NULL);

	int *value = (int *)data;
#ifdef DEBUG_WATER_PROCESSOR
	LOGI(TAG, "data wait to process.");
#endif

	if (mDatas.size() > 100) {
		mDatas.clear();
	}
	
	mDatas.push_back(*value);	
	mProcessor->notify();
}

void WaterProcessor::process()
{
#ifdef DEBUG_WATER_PROCESSOR
	LOGI(TAG, "process water-sampler's data.");
#endif

#ifdef DEBUG_WATER_PROCESSOR
	LOGI(TAG, "water processor's data size = %d.", mDatas.size());
#endif

	if (mDatas.size() <= 0) {
		return ;
	}
	
	/*fetch data*/
	int data = mDatas.front();
	mDatas.pop_front();
	
	theApp.getTransmiter()->post(&data);
}

status_t WaterProcessor::Processor::readyToRun()
{
	mLoop = uv_loop_new();
	ASSERT(mLoop != NULL);

	/*init and start exit async*/
	uv_async_init(mLoop, &mAsyncData, NULL);

	return NO_ERROR;
}

bool WaterProcessor::Processor::threadLoop()
{
	while (!exitPending()) {
		uv_run(mLoop, UV_RUN_ONCE);
		if (exitPending()) {
			break;
		}

		mWaterProcessor->process();
    }

	MAKE_VALGRIND_HAPPY(mLoop);
	
#ifdef DEBUG_WATER_PROCESSOR
	LOGI(TAG, "exit water-processor.");
#endif

	return true;
}

