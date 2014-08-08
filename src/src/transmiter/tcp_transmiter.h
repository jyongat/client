#ifndef TRANSIMITER_TCP_TRANSMITER_H
#define TRANSIMITER_TCP_TRANSMITER_H

#include <list>
#include "thread.h"
#include "tcp_socket.h"

#include "task.h"

class TcpTransmiter {
public:
	TcpTransmiter();
	virtual ~TcpTransmiter();

	class Transmiter : public Thread {
    public:
		Transmiter(TcpTransmiter *transmiter) : Thread(), mTcpTransmiter(transmiter) {}

		bool start()
		{			
			if (run("TCP Transmiter", PRIORITY_NORMAL) != NO_ERROR) {
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
		unsigned int flag;
	private:
		uv_loop_t *mLoop;
		uv_async_t mAsyncData;
		
    private:
        TcpTransmiter *mTcpTransmiter;
	
	private:
		status_t readyToRun();
        bool threadLoop();
    };
	
public:
	void post(void *data);

private:
	void transmit();

private:
	TcpSocket mSocket;
	
private:
	std::list<int> mDatas;
	
public:
	Transmiter *mTransmiter;
};

#endif // TRANSIMITER_TCP_TRANSMITER_H
