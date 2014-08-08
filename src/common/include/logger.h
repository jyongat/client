#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H

#define LOGGER_LEVEL_STDERR            0
#define LOGGER_LEVEL_EMERG             1
#define LOGGER_LEVEL_ALERT             2
#define LOGGER_LEVEL_CRIT              3
#define LOGGER_LEVEL_ERR               4
#define LOGGER_LEVEL_WARN              5
#define LOGGER_LEVEL_NOTICE            6
#define LOGGER_LEVEL_INFO              7
#define LOGGER_LEVEL_DEBUG             8

extern void log(int level, const char *tag, const char *file, \
	const char *function, int line, const char *fmt, ...);
extern void setLogLevel(int level);

#ifdef DEBUG
#define LOGD(tag, fmt, args...) \
	log(LOGGER_LEVEL_DEBUG, tag, __FILE__, __FUNCTION__, __LINE__, fmt, ##args)
#define LOGI(tag, fmt, args...) \
	log(LOGGER_LEVEL_INFO, tag, __FILE__, __FUNCTION__, __LINE__, fmt, ##args)
#define LOGW(tag, fmt, args...) \
	log(LOGGER_LEVEL_WARN, tag, __FILE__, __FUNCTION__, __LINE__, fmt, ##args)
#define LOGE(tag, fmt, args...) \
	log(LOGGER_LEVEL_ERR, tag, __FILE__, __FUNCTION__, __LINE__, fmt, ##args)
#else
#define LOGD(tag, fmt, args...)
#define LOGI(tag, fmt, args...)
#define LOGW(tag, fmt, args...)
#define LOGE(tag, fmt, args...)
#endif

#endif // COMMON_LOGGER_H