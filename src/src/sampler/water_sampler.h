#ifndef SAMPLE_WATER_H
#define SAMPLE_WATER_H

#include "thread.h"
#include "task.h"

#include "water_processor.h"

class WaterSampler {
public:
	WaterSampler();
	virtual ~WaterSampler();

	class Sampler : public Thread {
    public:
		Sampler(WaterSampler *sampler) : Thread(), mWaterSampler(sampler) {}

		bool start()
		{			
			ASSERT(mWaterSampler->mWaterProcessor->mProcessor->start() == true);
			
			if (run("Water Sampler", PRIORITY_NORMAL) != NO_ERROR) {
				return false;
			}
			
			return true;
        }

		void stop()
		{	
			requestExit();
			uv_async_send(&mAsyncExit);
			join();
			
			mWaterSampler->mWaterProcessor->mProcessor->stop();
		}

	private:
		uv_loop_t *mLoop;
		uv_async_t mAsyncExit;
		
    private:
        WaterSampler *mWaterSampler;
		
	private:
		status_t readyToRun();
        bool threadLoop();
    };
	
protected:
	void onSample();

private:
	static void onTimer(uv_timer_t* handle);
		
private:
	WaterProcessor *mWaterProcessor;
	uv_timer_t mTimer;

public:
	Sampler *mSampler;
	
};

#endif // SAMPLE_WATER_H
