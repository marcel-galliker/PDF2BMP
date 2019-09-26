
#ifndef __EVENT_H
#define __EVENT_H

#define EVT_ERROR	1	
#define EVT_WARNING	2	
#define EVT_INFO	4	

opj_bool opj_event_msg(opj_common_ptr cinfo, int event_type, const char *fmt, ...);

#endif 
