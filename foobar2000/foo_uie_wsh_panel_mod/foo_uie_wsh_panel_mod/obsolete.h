#pragma once


static inline void print_obsolete_message(const char * message)
{
	console::formatter() << WSPM_NAME ": Warning: Obsolete: " << message;
}

static inline void print_obsolete_message_once(bool& reported, const char * message)
{
	if (reported) return;
	print_obsolete_message(message);
	reported = true;
}

#define PRINT_OBSOLETE_MESSAGE_ONCE(message) \
	do {\
		static bool ___obsolete_message_reported__  = false;\
		print_obsolete_message_once(___obsolete_message_reported__, message);\
	} while(0)
