#include <stdio.h>
#include <unistd.h>

#include "task.h"
#include "application.h"
#include "logger.h"

/*disalble main debugger*/
#define DEBUG_MAIN

#ifdef DEBUG_MAIN
#define TAG "TAG_MAIN"
#endif

static void signal_cb(uv_signal_t* handle, int signum)
{
	if (SIGINT == signum) {
#ifdef DEBUG_MAIN
		LOGI(TAG, "SIGINT trigered.");
#endif
		theApp.mContext.exit = true;
		uv_close((uv_handle_t*)handle, NULL);
	}
}

static void init()
{
	uv_signal_t signal;
	ASSERT(0 == uv_signal_init(uv_default_loop(), &signal));
	ASSERT(0 == uv_signal_start(&signal, signal_cb, SIGINT));

	theApp.init();
}

static void myloop()
{
	/* async evnet loop */
	do {
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	} while(0);
	
	MAKE_VALGRIND_HAPPY(uv_default_loop());
}

static void uninit()
{
	theApp.uninit();
}


int main(int argc, char *argv[])
{		
	init();
	
	myloop();

	uninit();

#ifdef DEBUG_MAIN
	LOGI(TAG, "main exited.");
#endif

	return 0;
}

