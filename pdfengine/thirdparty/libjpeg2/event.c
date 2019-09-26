

#include "opj_includes.h"

#ifdef OPJ_CODE_NOT_USED
#ifndef _WIN32
static char*
i2a(unsigned i, char *a, unsigned r) {
	if (i/r > 0) a = i2a(i/r,a,r);
	*a = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i%r];
	return a+1;
}

static char *
_itoa(int i, char *a, int r) {
	r = ((r < 2) || (r > 36)) ? 10 : r;
	if(i < 0) {
		*a = '-';
		*i2a(-i, a+1, r) = 0;
	}
	else *i2a(i, a, r) = 0;
	return a;
}

#endif 
#endif

opj_event_mgr_t* OPJ_CALLCONV opj_set_event_mgr(opj_common_ptr cinfo, opj_event_mgr_t *event_mgr, void *context) {
	if(cinfo) {
		opj_event_mgr_t *previous = cinfo->event_mgr;
		cinfo->event_mgr = event_mgr;
		cinfo->client_data = context;
		return previous;
	}

	return NULL;
}

opj_bool opj_event_msg(opj_common_ptr cinfo, int event_type, const char *fmt, ...) {
#define MSG_SIZE 512 
	opj_msg_callback msg_handler = NULL;

	opj_event_mgr_t *event_mgr = cinfo->event_mgr;
	if(event_mgr != NULL) {
		switch(event_type) {
			case EVT_ERROR:
				msg_handler = event_mgr->error_handler;
				break;
			case EVT_WARNING:
				msg_handler = event_mgr->warning_handler;
				break;
			case EVT_INFO:
				msg_handler = event_mgr->info_handler;
				break;
			default:
				break;
		}
		if(msg_handler == NULL) {
			return OPJ_FALSE;
		}
	} else {
		return OPJ_FALSE;
	}

	if ((fmt != NULL) && (event_mgr != NULL)) {
		va_list arg;
		int str_length; 
		char message[MSG_SIZE];
		memset(message, 0, MSG_SIZE);
		
		va_start(arg, fmt);
		
		str_length = (strlen(fmt) > MSG_SIZE) ? MSG_SIZE : strlen(fmt);
		
		vsprintf(message, fmt, arg); 
		
		va_end(arg);

		
		msg_handler(message, cinfo->client_data);
	}

	return OPJ_TRUE;
}

