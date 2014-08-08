#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sched.h>
# include <sys/resource.h>
#endif

#if defined(HAVE_PRCTL)
#include <sys/prctl.h>
#endif

#include "thread.h"

/*
 * Create and run a new thread.
 *
 * We create it "detached", so it cleans up after itself.
 */

typedef void* (*pthread_entry)(void*);

static pthread_once_t gDoSchedulingGroupOnce = PTHREAD_ONCE_INIT;
static bool gDoSchedulingGroup = true;

static void checkDoSchedulingGroup(void)
{
//	char buf[PROPERTY_VALUE_MAX];
//	int len = property_get("debug.sys.noschedgroups", buf, "");
//	if (len > 0) {
//		int temp;
//		if (sscanf(buf, "%d", &temp) == 1) {
//			gDoSchedulingGroup = temp == 0;
//		}
//	}
}

struct thread_data_t {
	thread_func_t entryFunction;
	void* userData;
    int priority;
    char* threadName;

	// we use this trampoline when we need to set the priority with
	// nice/setpriority.
	static int trampoline(const thread_data_t* t)
	{
		thread_func_t f = t->entryFunction;
		void *u = t->userData;
		int prio = t->priority;
		char *name = t->threadName;
		delete t;
		setpriority(PRIO_PROCESS, 0, prio);
		pthread_once(&gDoSchedulingGroupOnce, checkDoSchedulingGroup);
		
		if (gDoSchedulingGroup) {
			if (prio >= PRIORITY_BACKGROUND) {
				set_sched_policy(getTid(), SP_BACKGROUND);
			} else {
				set_sched_policy(getTid(), SP_FOREGROUND);
			}
		}
	
        if (name) {
#if defined(HAVE_PRCTL)
			// Mac OS doesn't have this, and we build libutil for the host too
			int hasAt = 0;
			int hasDot = 0;
			char *s = name;
			while (*s) {
				if (*s == '.') hasDot = 1;
				else if (*s == '@') hasAt = 1;
				s++;
			}

			int len = s - name;
			if (len < 15 || hasAt || !hasDot) {
				s = name;
			} else {
				s = name + len - 15;
			}

			prctl(PR_SET_NAME, (unsigned long) s, 0, 0, 0);
#endif
			free(name);
		}
		
		return f(u);
	}
};

int createThreadEtc(thread_func_t entryFunction,
                               void *userData,
                               const char* threadName,
                               int32_t threadPriority,
                               size_t threadStackSize,
                               thread_id_t *threadId)
{
	pthread_attr_t attr; 
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (threadPriority != PRIORITY_DEFAULT || threadName != NULL) {
        // We could avoid the trampoline if there was a way to get to the
        // android_thread_id_t (pid) from pthread_t
        thread_data_t* t = new thread_data_t;
        t->priority = threadPriority;
        t->threadName = threadName ? strdup(threadName) : NULL;
        t->entryFunction = entryFunction;
        t->userData = userData;
        entryFunction = (thread_func_t)&thread_data_t::trampoline;
        userData = t;
	}

	if (threadStackSize) {
		pthread_attr_setstacksize(&attr, threadStackSize);
	}

	errno = 0;
	pthread_t thread;
	int result = pthread_create(&thread, &attr, \
		(pthread_entry)entryFunction, userData);

	pthread_attr_destroy(&attr);
	if (result != 0) {
		return 0;
	}

	// Note that *threadID is directly available to the parent only, as it is
	// assigned after the child starts.  Use memory barrier / lock if the child
	// or other threads also need access.
	if (threadId != NULL) {
		*threadId = (thread_id_t)thread; // XXX: this is not portable
	}

	return 1;
}

thread_id_t getThreadId()
{
	return (thread_id_t)pthread_self();
}

pid_t getTid()
{
#ifdef HAVE_GETTID
	return gettid();
#else
	return getpid();
#endif
}


int setThreadSchedulingGroup(pid_t tid, int grp)
{
	if (grp > TGROUP_MAX || grp < 0) {
		return BAD_VALUE;
	}

#if defined(HAVE_PTHREADS)
	pthread_once(&gDoSchedulingGroupOnce, checkDoSchedulingGroup);
	if (gDoSchedulingGroup) {
		// set_sched_policy does not support tid == 0
		if (tid == 0) {
			tid = getTid();
		}

		if (set_sched_policy(tid, (grp == TGROUP_BG_NONINTERACT) ? \
			SP_BACKGROUND : SP_FOREGROUND)) {
			return PERMISSION_DENIED;
		}
	}
#endif

	return NO_ERROR;
}

int setThreadPriority(pid_t tid, int pri)
{
	int rc = 0;

#if defined(HAVE_PTHREADS)
	int lasterr = 0;

	pthread_once(&gDoSchedulingGroupOnce, checkDoSchedulingGroup);
	if (gDoSchedulingGroup) {
		// set_sched_policy does not support tid == 0
		int policy_tid;
		if (tid == 0) {
			policy_tid = getTid();
		} else {
			policy_tid = tid;
		}

		if (pri >= PRIORITY_BACKGROUND) {
			rc = set_sched_policy(policy_tid, SP_BACKGROUND);
		} else if (getpriority(PRIO_PROCESS, tid) >= PRIORITY_BACKGROUND) {
			rc = set_sched_policy(policy_tid, SP_FOREGROUND);
		}
	}

	if (rc) {
		lasterr = errno;
	}

	if (setpriority(PRIO_PROCESS, tid, pri) < 0) {
		rc = INVALID_OPERATION;
	} else {
		errno = lasterr;
	}
#endif

	return rc;
}

int getThreadPriority(pid_t tid) {
#if defined(HAVE_PTHREADS)
    return getpriority(PRIO_PROCESS, tid);
#else
    return PRIORITY_NORMAL;
#endif
}

int getThreadSchedulingGroup(pid_t tid)
{
	int ret = TGROUP_DEFAULT;

#if defined(HAVE_PTHREADS)
	// convention is to not call get/set_sched_policy methods if disabled by property
	pthread_once(&gDoSchedulingGroupOnce, checkDoSchedulingGroup);
	if (gDoSchedulingGroup) {
		SchedPolicy policy = SP_BACKGROUND;
		// get_sched_policy does not support tid == 0
		if (tid == 0) {
			tid = getTid();
		}

		if (get_sched_policy(tid, &policy) < 0) {
			ret = INVALID_OPERATION;
		} else {
			switch (policy) {
			case SP_BACKGROUND:
				ret = TGROUP_BG_NONINTERACT;
				break;
			case SP_FOREGROUND:
				ret = TGROUP_FG_BOOST;
				break;
			default:
				// should not happen, as enum SchedPolicy does not have any other values
				ret = INVALID_OPERATION;
				break;
			}
		}
	}
#endif

	return ret;
}

int set_sched_policy(pid_t tid, const SchedPolicy & policy) {
	//TODO

	return 1;
}

int get_sched_policy(pid_t tid, SchedPolicy* policy) {
	//TODO
	
	return 1;
}


/*
 * This is our thread object!
 */

Thread::Thread()
	: mThread(thread_id_t(-1)),
	mLock("Thread::mLock"),
	mStatus(NO_ERROR),
	mExitPending(false),
	mRunning(false)
{
}

Thread::~Thread()
{
}

status_t Thread::readyToRun()
{
	return NO_ERROR;
}

status_t Thread::run(const char* name, int32_t priority, size_t stack)
{
	Mutex::Autolock _l(mLock);

	if (mRunning) {
		// thread already started
		return INVALID_OPERATION;
	}

	// reset status and exitPending to their default value, so we can
	// try again after an error happened (either below, or in readyToRun())
	mStatus = NO_ERROR;
	mExitPending = false;
	mThread = thread_id_t(-1);
	mRunning = true;
	bool res;
	
    res = createThreadEtc(_threadLoop, \
		this, name, priority, stack, &mThread);
	if (res == false) {
		mStatus = UNKNOWN_ERROR;   // something happened!
		mRunning = false;
		mThread = thread_id_t(-1);

		return UNKNOWN_ERROR;
    }

	// Do not refer to mStatus here: The thread is already running (may, in fact
	// already have exited with a valid mStatus result). The NO_ERROR indication
	// here merely indicates successfully starting the thread and does not
	// imply successful termination/execution.
	return NO_ERROR;

	// Exiting scope of mLock is a memory barrier and allows new thread to run
}

int Thread::_threadLoop(void* user)
{
	Thread* const self = static_cast<Thread*>(user);
	bool first = true;

	bool result;
	if (first) {
		first = false;
		self->mStatus = self->readyToRun();
		result = (self->mStatus == NO_ERROR);

		if (result && !self->exitPending()) {
			// Binder threads (and maybe others) rely on threadLoop
			// running at least once after a successful ::readyToRun()
			// (unless, of course, the thread has already been asked to exit
			// at that point).
			// This is because threads are essentially used like this:
			//   (new ThreadSubclass())->run();
			// The caller therefore does not retain a strong reference to
			// the thread and the thread would simply disappear after the
			// successful ::readyToRun() call instead of entering the
			// threadLoop at least once.
			result = self->threadLoop();
		}
	} else {
		result = self->threadLoop();
	}

    // establish a scope for mLock
    {
    Mutex::Autolock _l(self->mLock);
    if (result == false || self->mExitPending) {
        self->mExitPending = true;
        self->mRunning = false;
        // clear thread ID so that requestExitAndWait() does not exit if
        // called by a new thread using the same thread ID as this one.
        self->mThread = thread_id_t(-1);
        // note that interested observers blocked in requestExitAndWait are
        // awoken by broadcast, but blocked on mLock until break exits scope
        self->mThreadExitedCondition.broadcast();
        return 0;
    }
    }
    
    return 0;
}

void Thread::requestExit()
{
	Mutex::Autolock _l(mLock);
	mExitPending = true;
}

status_t Thread::requestExitAndWait()
{
	Mutex::Autolock _l(mLock);
	if (mThread == getThreadId()) {
		return WOULD_BLOCK;
	}

	mExitPending = true;

	while (mRunning == true) {
		mThreadExitedCondition.wait(mLock);
	}
    // This next line is probably not needed any more, but is being left for
    // historical reference. Note that each interested party will clear flag.
    mExitPending = false;

	return mStatus;
}

status_t Thread::join()
{
	Mutex::Autolock _l(mLock);
	if (mThread == getThreadId()) {
		return WOULD_BLOCK;
	}

	while (mRunning == true) {
		mThreadExitedCondition.wait(mLock);
	}

	return mStatus;
}

bool Thread::exitPending() const
{
	Mutex::Autolock _l(mLock);
	return mExitPending;
}
