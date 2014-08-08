#ifndef PROCESSOR_WATER_H
#define PROCESSOR_WATER_H

#include <list>
#include "thread.h"
#include "task.h"

class WaterProcessor {
public:
	WaterProcessor();
	virtual ~WaterProcessor();

	class Processor : public Thread {
    public:
		Processor(WaterProcessor *processor) : Thread(), mWaterProcessor(processor) {}

		bool start()
		{			
			if (run("Water Processor", PRIORITY_NORMAL) != NO_ERROR) {
				return false;
			}
			
			return true;
        }

		void stop()
		{	
			requestExit();
			uv_async_send(&mAsyncData);
			join();
		}

		void notify()
		{
			if (exitPending()) {
				return ;
			}
			
			uv_async_send(&mAsyncData);
		}

	private:
		uv_loop_t *mLoop;
		uv_async_t mAsyncData;
		
    private:
        WaterProcessor *mWaterProcessor;
	
	private:
		status_t readyToRun();
        bool threadLoop();
    };
	
public:
	void post(void *data);

private:
	void process();

private:
	std::list<int> mDatas;
	
public:
	Processor *mProcessor;
};


#endif // PROCESSOR_WATER_H

