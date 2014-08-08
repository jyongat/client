#include "logger.h"
#include "water_sampler.h"

/* disable water-sampler debugger */
#undef DEBUG_WATER_SAMPLER

#ifdef DEBUG_WATER_SAMPLER
static const char *TAG = "WaterSampler";
#endif

WaterSampler::WaterSampler()
{
	mSampler = new Sampler(this);
	ASSERT(mSampler != NULL);

	mWaterProcessor = new WaterProcessor();
	ASSERT(mWaterProcessor != NULL);
}


WaterSampler::~WaterSampler()
{
	if (NULL != mWaterProcessor) {
		delete mWaterProcessor;
		mWaterProcessor = NULL;
	}
				
	if (NULL != mSampler) {
		delete mSampler;
		mSampler = NULL;
	}
}

void WaterSampler::onSample()
{
	static int index = 0;

	index++;
#ifdef DEBUG_WATER_SAMPLER
	LOGI(TAG, "index = %d.", index);
#endif
	
	mWaterProcessor->post(&index);
}

void WaterSampler::onTimer(uv_timer_t* handle)
{
	ASSERT(handle != NULL);
	ASSERT(1 == uv_is_active((uv_handle_t*) handle));

	WaterSampler *self = reinterpret_cast<WaterSampler *>(handle->data);
	ASSERT(self != NULL);
	
	self->onSample();
}

status_t WaterSampler::Sampler::readyToRun()
{
	mLoop = uv_loop_new();
	ASSERT(mLoop != NULL);

	/*init and start exit async*/
	uv_async_init(mLoop, &mAsyncExit, NULL);

	/*init and start timer*/
	mWaterSampler->mTimer.data = mWaterSampler;
	uv_timer_init(mLoop, &mWaterSampler->mTimer);
	uv_timer_start(&mWaterSampler->mTimer, WaterSampler::onTimer, 50, 5);

	return NO_ERROR;
}

bool WaterSampler::Sampler::threadLoop()
{
	while (!exitPending()) {
		uv_run(mLoop, UV_RUN_ONCE);
    }

	MAKE_VALGRIND_HAPPY(mLoop);

#ifdef DEBUG_WATER_SAMPLER
	LOGI(TAG, "exit water sampler.");
#endif

	return true;
}

