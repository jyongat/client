#ifndef TRANSMITER_TCP_SOCKET_H
#define TRANSMITER_TCP_SOCKET_H

#include "task.h"

class TcpSocket {
public:
	TcpSocket();
	virtual ~TcpSocket();

public:
	void connect(uv_loop_t *loop, const char *address, unsigned short port);
	void disconnect();
	bool isWritable();
	int write(char *buf, ssize_t size);

private:
	uv_loop_t *loop;
	uv_thread_t mTidConnect;
	uv_stream_t *mStream;
	uv_tcp_t mClient;
	uv_connect_t mConnectRequest;
	uv_shutdown_t mShutdownRequest;
	uv_write_t mWriteRequest;
	uv_buf_t mReadBuffer;
	uv_timer_t mTimer;
	int mConnectTimes;

private:
	static void connectThread(void * param);

private:
	static void onTimer(uv_timer_t* handle);
	static void onConnect(uv_connect_t *req, int status);
	static void onShutDown(uv_shutdown_t *req, int status);
	static void onClose(uv_handle_t *handle);
	static void onAllocBuffer(uv_handle_t *handle, size_t size, uv_buf_t *buf);
	static void onWrite(uv_write_t *req, int status);
	static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	
private:
	char *address;
	unsigned short port;
	struct sockaddr_in addr;
};

inline bool TcpSocket::isWritable()
{
	return mStream != NULL;
}

#endif // TRANSMITER_TCP_SOCKET_H
