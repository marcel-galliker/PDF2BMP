#ifndef RT_SOK_API_MIN
#define RT_SOK_API_MIN

/* **************************************************************************** */
/*																				*/
/*    REAL TIME SOCKET COMMUNICATION API										*/
/*																				*/
/* ****************************************************************************	*/
/*																				*/
/*    Copyright 1998 Graph-Tech AG, Switzerland. All rights reserved.			*/
/*    Written by Markus Portmann.				               					*/
/*																				*/
/* ****************************************************************************	*/

/* --- Includes --------------------------------------------------------------- */
#include "Winsock.h"
#include <stdio.h>                        


/* --- Macros ----------------------------------------------------------------- */


/* --- Defines ---------------------------------------------------------------- */


/* --- Structures ------------------------------------------------------------- */
										/* socket initialization */



/* --- Prototypes ------------------------------------------------------------- */

int		rt_sok_init				(int client, char *host, char *port, SOCKET *socki);
void	rt_sok_close			(int clean, SOCKET *socki);
int		rt_sok_msg_send			(SOCKET *socki, void *msg_send, int send_size);
int		rt_sok_msg_receive		(SOCKET *socki, void *msg_rec, int max_rec_size);
int		rt_sok_msg_send_bj		(SOCKET *socki, void *msg_send, int send_size);
int		rt_sok_msg_receive_bj	(SOCKET *socki, void *msg_rec, int max_rec_size);

int		rt_sok_msg_send_nb		(SOCKET *socki, void *msg_send, int send_size);
int		rt_sok_msg_receive_nb	(SOCKET *socki, void *msg_rec, int max_rec_size);
int		rt_sok_msg_send_nb_bj	(SOCKET *socki, void *msg_send, int send_size, int size);
int		rt_sok_msg_receive_nb_bj(SOCKET *socki, void *msg_rec, int max_rec_size, int size);
int		rt_sok_set_nb			(SOCKET *socki);
int		rt_sok_send_nb			(SOCKET *socki, void *msg_send, int send_size);
int		rt_sok_receive_nb		(SOCKET *socki, void *msg_rec, int max_rec_size, int size);

int		rt_sok_msg_receive_gj108(SOCKET *socki, void *msg_rec, int max_rec_size);
int		rt_sok_msg_send_gj108	(SOCKET *socki, void *msg_send, int send_size);
int		recv_nb108				(SOCKET *s, void *buf, int len, int flags);
int		send_nb108				(SOCKET *s, const void *buf, int len, int flags);



/* ----------------------------------------------------------------------------*/

#endif

