#pragma once

// Remember to bump utils.version too
#define JSP_VERSION_NUMBER "1.2.3.3"
#define JSP_WILL_NOT_EXPIRE
//#define JSP_VERSION_TEST "Beta 1"

#ifdef JSP_VERSION_TEST
#	define JSP_TESTING 1
#	define JSP_VERSION_TEST_PREFIX " "
#else
#	define JSP_TESTING 0
#	define JSP_VERSION_TEST ""
#	define JSP_VERSION_TEST_PREFIX ""
#endif

#if defined(DEBUG) || defined(_DEBUG)
#	define JSP_VERSION_DEBUG_SUFFIX " (Debug)"
#else
#	define JSP_VERSION_DEBUG_SUFFIX ""
#endif

#define JSP_VERSION JSP_VERSION_NUMBER JSP_VERSION_TEST_PREFIX JSP_VERSION_TEST JSP_VERSION_DEBUG_SUFFIX

#if JSP_TESTING == 1 && !defined(JSP_WILL_NOT_EXPIRE)
/* NOTE: Assume that date is following this format: "Jan 28 2010" */
bool is_expired(const char * date);
#	define IS_EXPIRED(date) (is_expired(__DATE__))
#else
#	define IS_EXPIRED(date) (false)
#endif
