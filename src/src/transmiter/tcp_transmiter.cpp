#include <string.h>
#include "logger.h"
#include "tcp_transmiter.h"

/*disable tcp-transmiter debugger*/
#undef DEBUG_TCP_TRANSMITER

#ifdef DEBUG_TCP_TRANSMITER
static const char *TAG = "TcpTransmiter";
#endif

TcpTransmiter::TcpTransmiter()
{
	mTransmiter = new Transmiter(this);
	ASSERT(mTransmiter != NULL);
}


TcpTransmiter::~TcpTransmiter()
{
	if (NULL != mTransmiter) {
		delete mTransmiter;
		mTransmiter = NULL;
	}

	mDatas.clear();
}

void TcpTransmiter::post(void *data)
{
	ASSERT(data != NULL);

	int *value = (int *)data;
#ifdef DEBUG_TCP_TRANSMITER
	LOGI(TAG, "data wait to transmit.");
#endif

	mDatas.push_back(*value);
	mTransmiter->notify();
}

void TcpTransmiter::transmit()
{
#ifdef DEBUG_TCP_TRANSMITER
	LOGI(TAG, "transmit data.");
#endif

	if (!mSocket.isWritable()) {
#ifdef DEBUG_TCP_TRANSMITER
		LOGI(TAG, "socket is unwritable.");
#endif
		return ;
	}

	if (mDatas.size() <= 0) {
		return ;
	}
	/*fetch data*/
	int data = mDatas.front();
	mDatas.pop_front();
	
	int *buf = (int *)malloc(sizeof(int));
	memcpy(buf, &data, sizeof(int));
	mSocket.write((char *)buf, sizeof(int));
}

status_t TcpTransmiter::Transmiter::readyToRun()
{
	mLoop = uv_loop_new();
	ASSERT(mLoop != NULL);

	flag = 0;
	/*init and start exit async*/
	uv_async_init(mLoop, &mAsyncData, NULL);
	mTcpTransmiter->mSocket.connect(mLoop, "10.10.7.177", 3001);
	
	return NO_ERROR;
}

bool TcpTransmiter::Transmiter::threadLoop()
{
	while (!exitPending()) {
		uv_run(mLoop, UV_RUN_ONCE);
		if (exitPending()) {
			break;
		}

		mTcpTransmiter->transmit();
    }

	MAKE_VALGRIND_HAPPY(mLoop);
	
#ifdef DEBUG_TCP_TRANSMITER
	LOGI(TAG, "exit tcp-transmiter.");
#endif

	return true;
}

