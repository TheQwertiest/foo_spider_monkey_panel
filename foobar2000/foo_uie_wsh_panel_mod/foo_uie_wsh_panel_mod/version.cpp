#include "stdafx.h"
#include "version.h"

#if WSPM_TESTING == 1
#include <time.h>

/* NOTE: Assume that date is following this format: "Jan 28 2010" */
bool is_expired(const char * date)
{
	char s_month[4] = {0};
	int month, day, year;
	tm t = {0};
	const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

	sscanf_s(date, "%3s %2d %4d", s_month, _countof(s_month), &day, &year);

	const char * month_pos = strstr(month_names, s_month);
	month = (month_pos - month_names) / 3;

	t.tm_mon = month;
	t.tm_mday = day;
	t.tm_year = year - 1900;
	t.tm_isdst = FALSE;

	time_t start = mktime(&t);
	time_t end = time(NULL);

	// expire in ~32 days
	const double secs = 32 * 60 * 60 * 24;
	return (difftime(end, start) > secs);
}
#endif
