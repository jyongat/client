#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "application.h"

#include "tcp_socket.h"

/* disable tcp-socket debugger */
#define DEBUG_TCP_SOCKET

#ifdef DEBUG_TCP_SOCKET
static const char *TAG = "TcpSocket";
#endif

TcpSocket::TcpSocket()
{
	address = NULL;
	mStream = NULL;

	const int bufSize = 4096;
	char *buf = (char *)malloc(bufSize);
	ASSERT(buf != NULL);
	mReadBuffer = uv_buf_init(buf, bufSize);
	
	mConnectTimes = 0;
}

TcpSocket::~TcpSocket()
{
	free(mReadBuffer.base);
	
	if (NULL != address) {
		free(address);
		address = NULL;
	}
}

void TcpSocket::connectThread(void * param)
{
	TcpSocket *self = reinterpret_cast<TcpSocket*>(param);
	ASSERT(self != NULL);
	
#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "ip address = %s, port = %d", self->address, self->port);
#endif
	ASSERT(0 == uv_ip4_addr(self->address, self->port, &self->addr));
	ASSERT(0 == uv_tcp_init(self->loop, &self->mClient));
	ASSERT(0 == uv_tcp_nodelay(&self->mClient, 1));
	ASSERT(0 == uv_tcp_keepalive(&self->mClient, 1, 60));

	self->mConnectRequest.data = self;
	uv_timer_init(self->loop, &self->mTimer);
	self->mTimer.data = self;
	int timeout = (self->mConnectTimes / 10) * 1000;
	if (timeout < 2000) {
		timeout = 2000;
	} else if (timeout > 15 * 1000) {
		timeout = 15 * 1000;
	}
	
	uv_timer_start(&self->mTimer, TcpSocket::onTimer, timeout, 0);
	ASSERT(0 == uv_tcp_connect(&self->mConnectRequest, &self->mClient, \
		(const struct sockaddr *)&self->addr, TcpSocket::onConnect));

#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "exit connect thread.");
#endif 
}

void TcpSocket::connect(uv_loop_t *loop, const char *address, \
	unsigned short port)
{
	ASSERT(loop != NULL);
	ASSERT(address != NULL);

	int len = strlen(address);
	this->address = (char *)malloc(len + 1);
	ASSERT(this->address != NULL);
	
	memcpy(this->address, address, len);
	this->address[len] = '\0';
	this->port = port;
	this->loop = loop;

	ASSERT(0 == uv_thread_create(&mTidConnect, \
		TcpSocket::connectThread, this));
}

void TcpSocket::disconnect()
{	
	if (0 != mTidConnect) {
		uv_thread_join(&mTidConnect);
	}
	
	if (NULL != mStream) {
		ASSERT(0 == uv_read_stop(mStream));
		mStream = NULL;
	}

#if 0
	mShutdownRequest.data = this;
	if (0 != uv_shutdown(&mShutdownRequest, mStream, TcpSocket::onShutDown)) {
		LOGE(TAG, "uv_shutdown error.");
	}
#endif
	uv_timer_stop(&mTimer);
	uv_close((uv_handle_t *)&mTimer, NULL);
	
	uv_close((uv_handle_t *)&mClient, TcpSocket::onClose);
}

int TcpSocket::write(char *buf, ssize_t size)
{
#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "write.");
#endif
	ASSERT(buf != NULL);

	if (NULL == mStream) {
		return 0;
	}
	
	const int pkgSize = 4096;
	const int count = size / pkgSize + size % pkgSize != 0 ? 1 : 0;
	uv_buf_t bufs[count];
	for (int i=0; i<count-1; i++) {
		bufs[i] = uv_buf_init(buf + i * pkgSize, pkgSize);
	}
	
	bufs[count - 1] = uv_buf_init(buf + (count - 1) * pkgSize, size % pkgSize);

#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "buf[%d - 1].size = %d with value = %s", count, bufs[count - 1].len, bufs[count - 1].base);
#endif

	mWriteRequest.data = this;
	int ret = uv_write(&mWriteRequest, mStream, bufs, count, \
		TcpSocket::onWrite);

	free(buf);
	buf = NULL;
	
    if (0 != ret) {
#ifdef DEBUG_TCP_SOCKET
		LOGE(TAG, "ur_write error.");
#endif
		disconnect();
		ASSERT(0 == uv_thread_create(&mTidConnect, \
				TcpSocket::connectThread, this));
	}
	
	return ret;
}

void TcpSocket::onAllocBuffer(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
	TcpSocket *self = reinterpret_cast<TcpSocket*>(handle->data);
	ASSERT(self != NULL);

//	memset(self->mReadBuffer.base, 0, self->mReadBuffer.len);
	*buf = self->mReadBuffer;
}

void TcpSocket::onTimer(uv_timer_t* handle)
{
	TcpSocket *self = reinterpret_cast<TcpSocket*>(handle->data);
	ASSERT(self != NULL);

	
	if (NULL != self->mStream) {
		return ;
	}

	uv_close((uv_handle_t *)handle, NULL);
	self->mConnectTimes++;
	
	/* reconnect */
	uv_close((uv_handle_t *)&self->mClient, NULL);	
	ASSERT(0 == uv_thread_create(&self->mTidConnect, \
		TcpSocket::connectThread, self));
}

void TcpSocket::onConnect(uv_connect_t *req, int status)
{
#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "onConnected.");
#endif
	
	TcpSocket *self = reinterpret_cast<TcpSocket*>(req->data);
	ASSERT(self != NULL);
	
	if (0 != status) {
#ifdef DEBUG_TCP_SOCKET
		LOGE(TAG, "connect error.");
#endif

		if (status == UV_ECANCELED) {
			return;  /* Handle has been closed. */
  		}
		
		return ;
	}	

	uv_timer_stop(&self->mTimer);
	self->mConnectTimes = 0;
	self->mStream = req->handle;
	ASSERT(self->mStream != NULL);
	
	/* Start reading */
	self->mStream->data = self;
  	ASSERT(0 == uv_read_start(self->mStream, TcpSocket::onAllocBuffer, \
		TcpSocket::onRead));
}

void TcpSocket::onShutDown(uv_shutdown_t *req, int status)
{
#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "onShutDown.");
#endif

	ASSERT(req != NULL);
	ASSERT(status == 0);

	uv_tcp_t *tcp = (uv_tcp_t*)(req->handle);

	/* The write buffer should be empty by now. */
	ASSERT(tcp->write_queue_size == 0);
}

void TcpSocket::onClose(uv_handle_t *handle)
{
#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "onClose.");
#endif
}

void TcpSocket::onWrite(uv_write_t *req, int status)
{
#ifdef DEBUG_TCP_SOCKET
	LOGI(TAG, "onWrite.");
#endif
	
	if (status == UV_ECANCELED) {
		return;  /* Handle has been closed. */
	}

	TcpSocket *self = reinterpret_cast<TcpSocket *>(req->data);
	ASSERT(self != NULL);

	if (status) {
#ifdef DEBUG_TCP_SOCKET
		LOGE(TAG, "uv_write error: %s\n", uv_strerror(status));
#endif
		self->disconnect();
		ASSERT(0 == uv_thread_create(&self->mTidConnect, \
				TcpSocket::connectThread, self));
	}
}

void TcpSocket::onRead(uv_stream_t *stream, ssize_t nread, \
	const uv_buf_t *buf)
{
#ifdef DEBUG_TCP_SOCKET
		LOGI(TAG, "onRead.");
#endif
	
	TcpSocket *self = reinterpret_cast<TcpSocket *>(stream->data);
	ASSERT(self != NULL);
	
	if (nread < 0) {
		if (nread == UV_EOF) {
#ifdef DEBUG_TCP_SOCKET
			LOGW(TAG, "server closed normally.");
#endif
			/* reconnect */
			self->disconnect();
			ASSERT(0 == uv_thread_create(&self->mTidConnect, \
				TcpSocket::connectThread, self));
		} else if (nread == UV_ECONNRESET) {
#ifdef DEBUG_TCP_SOCKET
			LOGW(TAG, "server closed imnormally.");
#endif
			/* reconnect */
			self->disconnect();
			ASSERT(0 == uv_thread_create(&self->mTidConnect, \
				TcpSocket::connectThread, self));
		} else {
#ifdef DEBUG_TCP_SOCKET
            LOGW(TAG, "server closed error.");
#endif
			/* reconnect */
			self->disconnect();
			ASSERT(0 == uv_thread_create(&self->mTidConnect, \
				TcpSocket::connectThread, self));
		}
		
		return ;
	} else {	
#ifdef DEBUG_TCP_SOCKET	
		for (int i=0; i<nread/4; i++) {
			LOGI(TAG, "read integer value = %d with nread = %d", *((int *)(buf->base + i * 4)), nread);
		}
#endif
	}
}
	

