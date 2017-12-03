#pragma once

// Remember to bump utils.version too
#define JSP_VERSION_NUMBER "1.3.2.1"
//#define JSP_VERSION_TEST "Beta 1"

#ifdef JSP_VERSION_TEST
#	define JSP_VERSION_TEST_PREFIX " "
#else
#	define JSP_VERSION_TEST ""
#	define JSP_VERSION_TEST_PREFIX ""
#endif

#if defined(DEBUG) || defined(_DEBUG)
#	define JSP_VERSION_DEBUG_SUFFIX " (Debug)"
#else
#	define JSP_VERSION_DEBUG_SUFFIX ""
#endif

#define JSP_VERSION JSP_VERSION_NUMBER JSP_VERSION_TEST_PREFIX JSP_VERSION_TEST JSP_VERSION_DEBUG_SUFFIX
