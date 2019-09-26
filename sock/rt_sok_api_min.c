
// Please keep this library portable, damn it !
/* **************************************************************************** */
/*																				*/
/*    Real Time API																*/
/*						C O M M U N I C A T I O N								*/
/*						-------------------------								*/
/* ****************************************************************************	*/
/*																				*/
/*    Copyright 1998 Graph-Tech AG, Switzerland. All rights reserved.			*/
/*    Written by Markus Portmann.												*/
/*																				*/
/*	  rt_sok: InterProcessCommunication 										*/
/*			  communication between process in the same or different			*/
/*			  computer based on sockets. NON-BLOCKING messages.					*/
/*			  Does not work in RTX (no sockets there yet)						*/
/*																				*/
/*			  The protocol is GT-specific: 										*/
/*					4 bytes: length of message in bytes							*/
/*					rest: message itself										*/
/*																				*/
/*			  rt_sok_init														*/
/*			  rt_sok_close														*/
/*			  rt_sok_send														*/
/*			  rt_sok_receive													*/
/*			  rt_sok_msg_send_bj												*/
/*			  rt_sok_msg_receive_bj												*/
/*																				*/
/* ****************************************************************************	*/


/* --- Version ---------------------------------------------------------------- */


/* --- Includes ---------------------------------------------------------------	*/
#include <stdio.h>	
#include <stdlib.h>
#include <string.h>

#include "rt_sok_api_min.h"

/* --- Makros ----------------------------------------------------------------- */



/* --- Defines ---------------------------------------------------------------- */
 
#define traciSOK 1



/* --- Structures -------------------------------------------------------------	*/




/* --- Prototypes ------------------------------------------------------------- */

/* ---------------------------------------------------------------------------- */



/* **************************************************************************** */
/*    G E N E R A L   F U N C T I O N S											*/
/* **************************************************************************** */


/*******************************************************************************
rt_sok_init: initializes sockets, either for a client or a server 

	client		TRUE: it is a client that tries to connect to an existing end-point
				FALSE: it is a server that creates an end-point
	host		name of host. default "localhost" 
	port		number of port. default "10051" 

  return:		0=OK
	
	socki		pointer to socket handle

*******************************************************************************/
int	rt_sok_init			( int client, char *host, char *port, SOCKET *socki)
{
	WORD version;						
	WSADATA	data;
	int accessPoint;
	struct sockaddr_in baseAddr;
	struct hostent *hostInfo;
	int ret=0;
	int init=TRUE;

	if ( traciSOK) 
		printf("client=%d  host [%s]  port [%s]\n", client, host, port);

	if ( client){
		/* --- Startup Winsockets --- */
		if ( traciSOK) printf("Startup Winsockets\n");
		version= MAKEWORD (2,0);
		ret= WSAStartup (version, &data);
		if (ret) {
			printf("<<<<<<<<<<<<<<<<< version 1\n");
			version= MAKEWORD (1,1);
			ret= WSAStartup (version, &data);
		}
		if ( ret)
			return( 1);

		/* --- Get socket --- */
		if ( traciSOK) printf("Get socket\n");
		*socki = socket( AF_INET, SOCK_STREAM, 0);
		if ( *socki == INVALID_SOCKET){
			return(2);
		}
		/* --- Socket options --- */
		if (setsockopt (*socki, IPPROTO_TCP, TCP_NODELAY, (char *) &init, sizeof (BOOL)))
			return(3);

		if (isalpha(host[0])){       
			/* --- Resolve host name --- */  
			hostInfo = gethostbyname ((char*) host);
			if ( hostInfo==NULL)
				return(3);
			memcpy((char *) &(baseAddr.sin_addr), (char *) hostInfo->h_addr, hostInfo->h_length);
		}
		else{
			/* Convert nnn.nnn address to a usable one */
			baseAddr.sin_addr.S_un.S_addr = inet_addr( host); // do not go over DNS when IP-Address is already known! (much quicker!)
		}

		/* --- Get port Number --- */
		if ( traciSOK) printf("get port for >>>%s<<< port=%d\n", host, (int) atoi( port));
	    baseAddr.sin_family = AF_INET;       
	    accessPoint = (int) atoi( port);
		baseAddr.sin_port = htons( (short) accessPoint);

		/* --- Try to connect to socket --- */
		if ( traciSOK) printf("Try to connect socket\n");
		if ( connect( *socki, (struct sockaddr *) &baseAddr, sizeof (baseAddr) ) != 0) {
			if ( traciSOK){
				ret = WSAGetLastError();
//				RtPrintf ("Error is >>>>%d<<<<\n", ret);
			}
			return(5);
		}
		if ( traciSOK) printf("Socket connected\n");
	}
	else{
		/* --- Startup Winsockets --- */
		if ( traciSOK) printf("Startup Winsockets\n");
		version= MAKEWORD (2,0);
		ret= WSAStartup (version, &data);
		if (ret) {
			version= MAKEWORD (1,1);
			ret= WSAStartup (version, &data);
		}
		if ( ret)
			return(6);

		/* --- Get socket --- */
		if ( traciSOK) printf("Get socket\n");
		*socki = socket( AF_INET, SOCK_STREAM, 0);	
		if ( *socki == INVALID_SOCKET)
			return(7);

		/* --- init socket: Non-blocking mode --- */
		if ( traciSOK) printf("init socket: Non-blocking mode\n");
	
		/* --- Get port Number --- */
		if ( traciSOK) printf("get port for >>>%s<<< port=%d\n", host, (int) atoi( port));
	    baseAddr.sin_addr.s_addr = INADDR_ANY;
	    baseAddr.sin_family = AF_INET;       
	    accessPoint = (int) atoi( port);
		baseAddr.sin_port = htons( (short) accessPoint);

		/* --- Socket options --- */
		if (setsockopt (*socki, IPPROTO_TCP, TCP_NODELAY, (char*) &init, sizeof (BOOL)))
			return(8);

		/* --- Try to bind socket --- */
		if ( traciSOK) printf("Try to bind socket\n");
		if ( bind( *socki, (struct sockaddr *) &baseAddr, sizeof (baseAddr) ) != 0)
			return(9);
	}
	return (0);
}


/*******************************************************************************
rt_sok_close: close sockets, either for a client or a server 

	client		TRUE: it is a client
				FALSE: it is a server 

*******************************************************************************/
void	rt_sok_close		( int clean, SOCKET *socki)
{
	if ( traciSOK) printf("rt_sok_close\n");

	closesocket( *socki);
	Sleep(10); // this must be !!!
	if (clean)
	{
		WSACleanup ();
		if ( traciSOK) printf("clean\n");
	}
	*socki= INVALID_SOCKET;
}


/*******************************************************************************
rt_sok_msg_send: sends a message though a socket. You stay blocked until the 
				 message is out.

	socki		pointer to socket where msg_send is to be sent
	msg_send	pointer to structure (buffer, struct, ..) to be sent
	send_size	size of msg_send

  
	return:		0=NOK
				n  = length

*******************************************************************************/
int		rt_sok_msg_send		( SOCKET *socki, void *msg_send, int send_size)
{
	int ret;

	/* --- send message length --- */
	if ( ret=send( *socki, (char *) &send_size, sizeof( send_size), 0) <= 0){
//		ret = WSAGetLastError();
//		#ifndef _AFXDLL
//		RtPrintf ("Send 11 Return is %d", ret);
//		#endif
		return( 0);
	}

	/* --- send message --- */
	if ( ret=send( *socki, (char*) msg_send, send_size, 0) <= 0){
//		ret = WSAGetLastError();
//		#ifndef _AFXDLL
//		RtPrintf ("Send 22 Return is %d", ret);
//		#endif
		return( 0);
	}

	return (send_size);
}


/*******************************************************************************
rt_sok_msg_receive: receives a message from a socket. You stay blocked until a 
					message	comes.

	socki			pointer to socket where msg is goint to come_send is to be sent
	msg_send		pointer to structure (buffer, struct, ..) to be sent
	max_rec_size	maximum size of message to be received

  return	0,<0  = NOK: socket closed
			n     = length

*******************************************************************************/
int		rt_sok_msg_receive	( SOCKET *socki, void *msg_rec, int max_rec_size)
{
//	int ret;
	static char buffi[ 8];
	static int leni, pos;
	static int *lenp;
	static char *pMsg;

	/* --- get message length --- */
	leni = recv( *socki, buffi, sizeof( lenp), 0);
	if ( leni==0) {
		return(0);
	}
	if ( leni < 0){
//		ret = WSAGetLastError();
//		#ifndef _AFXDLL
//		RtPrintf ("Rec 11 Error is %d\n", ret);
//		#endif
		return( -1);
	}
	lenp = (int*) buffi;

	if ( *lenp > max_rec_size){
		return( -2);
	}
	/* --- get message --- */
	pMsg= (char*) msg_rec;
	pos = *lenp;
	for (;;) {
		leni = recv( *socki, pMsg, pos, 0);
		if ( leni < 0){
//			ret = WSAGetLastError();
//			#ifndef _AFXDLL
//			RtPrintf ("Rec 22 Error is %d\n", ret);
//			#endif
			return( -5);
		}
		pos -= leni;
		if ( pos <= 0) break;
		pMsg += leni;
	}
	if ( pos )
		return( -3);
	return (*lenp);
}

/*******************************************************************************
rt_sok_msg_send_bj: 
		sends a message though a socket to a bitjet. 
		You stay blocked until the message is out.

	socki		pointer to socket where msg_send is to be sent
	msg_send	pointer to structure (buffer, struct, ..) to be sent
	send_size	size of msg_send

  
	return:		0=NOK
				n  = length

*******************************************************************************/
int		rt_sok_msg_send_bj	( SOCKET *socki, void *msg_send, int send_size)
{
	int sizi;

	/* --- send message length --- */
	sizi=send_size+4;	// the protocol wants the length of the length too
	if ( send( *socki, (char *) &sizi, sizeof( sizi), 0) <= 0)
		return( 0);

	/* --- send message --- */
	if ( send( *socki, (char*) msg_send, send_size, 0) <= 0)
		return( 0);

	return (send_size);
}


/*******************************************************************************
rt_sok_msg_receive_bj: 
		receives a message from a socket. You stay blocked until a 
		message	comes.

	socki			pointer to socket where msg is goint to come_send is to be sent
	msg_send		pointer to structure (buffer, struct, ..) to be sent
	max_rec_size	maximum size of message to be received

  return	0  = NOK: socket closed
			n  = length

*******************************************************************************/
int		rt_sok_msg_receive_bj( SOCKET *socki, void *msg_rec, int max_rec_size)
{
	static char buffi[ 8];
	static int leni, pos;
	static int *lenp;
	static char *pMsg;

	/* --- get message length --- */
	leni = recv( *socki, buffi, sizeof( lenp), 0);
	if ( leni==0) 
		return(0);

	if ( leni < 0){
		return( -1);
	}
	lenp = (int*) buffi;
	*lenp = *lenp-4;		// total length of packet, including length

	if ( *lenp > max_rec_size)
		return( -2);

	/* --- get message --- */
	/* header length (int), 0
	   version (int), 4
	   engine (char), 8
	   packet type (char), 9
	   type identifier (char), 10
	   data []
	*/

	pMsg=(char*) msg_rec;
	pos = *lenp;
	for (;;){
		leni = recv( *socki, pMsg, pos, 0);
		if ( leni==0) 
			return(0);

		if ( leni < 0){
			return( -1);
		}
		pos -= leni;
		if ( pos <= 0) break;
		pMsg += leni;
	}

	if ( pos )
		return( -3);
	if ( leni <= 0)
		return( -4);

	return (*lenp);
}


/*******************************************************************************
rt_sok_msg_receive: receives a message from a socket. Non blocking mode.

	socki			pointer to socket where msg is goint to come_send is to be sent
	msg_send		pointer to structure (buffer, struct, ..) to be sent
	max_rec_size	maximum size of message to be received

  return	0,<0  = NOK: socket closed
			n     = length

*******************************************************************************/
int		rt_sok_msg_receive_nb	( SOCKET *socki, void *msg_rec, int max_rec_size)
{
	int leni, rec, ret;
	static char buffi[ 8];
	static int *lenp;
	static char *pMsg;
	fd_set readSock;
	struct timeval timeout;

	timeout.tv_sec=0;
	timeout.tv_usec=50000;
	FD_ZERO (&readSock);
	FD_SET (*socki, &readSock);

	/* --- get message length --- */
	rec=0;
	while (1) {
		leni=recv (*socki, &buffi [rec], sizeof (lenp)-rec, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, &readSock, NULL, NULL, NULL);
				if (ret==SOCKET_ERROR) {
//					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Rcv 1 Return is %d\n", ret);
//					#endif
					return (-1);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Rcv 2 Return is %d\n", ret);
//				#endif
				return (-2);
			}
		}
		else if (!leni) {
			return (0);
		}
		else {
			rec+=leni;
			if (rec==sizeof (lenp))
				break;
		}
	}
	lenp=(int*) buffi;
	if (*lenp>max_rec_size)
		return( -2);

	/* --- get message --- */
	rec=0;
	while (1) {
		leni=recv (*socki, (char *) msg_rec+rec, *lenp-rec, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, &readSock, NULL, NULL, NULL);
				if (ret==SOCKET_ERROR) {
//					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Rcv 3 Return is %d\n", ret);
//					#endif
					return (-3);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Rcv 4 Return is %d\n", ret);
//				#endif
				return (-4);
			}
		}
		else if (!leni) {
			return (0);
		}
		else {
			rec+=leni;
			if (rec>=*lenp)
				break;
		}
	}
	if (rec!=*lenp)
		return (-5);
	return (*lenp);
}

/*******************************************************************************
rt_sok_msg_receive_nb_bj: receives a message from a socket. Non blocking mode.

	socki			pointer to socket where msg is goint to come_send is to be sent
	msg_send		pointer to structure (buffer, struct, ..) to be sent
	max_rec_size	maximum size of message to be received

  return	0,<0  = NOK: socket closed
			n     = length

*******************************************************************************/
int		rt_sok_msg_receive_nb_bj	( SOCKET *socki, void *msg_rec, int max_rec_size, int size)
{
	int leni, rec, ret;
	static char buffi[ 8];
	static int *lenp;
	static char *pMsg;
	fd_set readSock;
	struct timeval timeout;

	timeout.tv_sec=0;
	timeout.tv_usec=50000;
	FD_ZERO (&readSock);
	FD_SET (*socki, &readSock);

	/* --- get message length --- */
//RtPrintf("rt_sok_msg_receive_nb_bj: socki=%X msg_rec=%X\n", socki, msg_rec);
	rec=0;
	while (!size) {
		leni=recv (*socki, &buffi [rec], sizeof (lenp)-rec, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, &readSock, NULL, NULL, NULL);
				if (ret==SOCKET_ERROR) {
					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Rcv 1 Return is %d\n", ret);
//					#endif
					return (-1);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Rcv 2 Return is %d\n", ret);
//				#endif
				return (-2);
			}
		}
		else if (!leni) {
			return (0);
		}
		else {
			rec+=leni;
			if (rec==sizeof (lenp))
				break;
		}
	}
	if (!size) {
		lenp=(int*) buffi;
		*lenp = *lenp-4;		// total length of packet, including length
		if (*lenp>max_rec_size)
			return( -2);
	}
	else
		*lenp=size;
	/* --- get message --- */
	rec=0;
	while (1) {
		leni=recv (*socki, (char *) msg_rec+rec, *lenp-rec, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, &readSock, NULL, NULL, NULL);
				if (ret==SOCKET_ERROR) {
					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Rcv 3 Return is %d\n", ret);
//					#endif
					return (-3);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Rcv 4 Return is %d\n", ret);
//				#endif
				return (-4);
			}
		}
		else if (!leni) {
			return (0);
		}
		else {
			rec+=leni;
			if (rec>=*lenp)
				break;
		}
	}
//	if (*lenp!=max_rec_size)
//		return (-5);
	return (*lenp);
}


/*******************************************************************************
rt_sok_msg_send: sends a message though a socket. Non blocking mode.

	socki		pointer to socket where msg_send is to be sent
	msg_send	pointer to structure (buffer, struct, ..) to be sent
	send_size	size of msg_send

  
	return:		< 0=NOK
				n  = length

*******************************************************************************/
int rt_sok_msg_send_nb (SOCKET *socki, void *msg_send, int send_size)
{
	int leni, sent, ret;
	fd_set writeSock;
	struct timeval timeout;

	timeout.tv_sec=0;
	timeout.tv_usec=50000;
	FD_ZERO (&writeSock);
	FD_SET (*socki, &writeSock);

	/* --- send message length --- */
	sent=0;
	while (1) {
		leni=send (*socki, (char *) (&send_size)+sent, sizeof (send_size)-sent, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, NULL, &writeSock, NULL, NULL);
				if (ret==SOCKET_ERROR) {
//					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Send 1 Return is %d\n", ret);
//					#endif
					return (-1);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Send 2 Return is %d\n", ret);
//				#endif
				return (-2);
			}
		}
		else if (!leni) {
//			#ifndef _AFXDLL
//			RtPrintf ("Send 3 Return is %d\n", ret);
//			#endif
			return (-3);
		}
		else {
			sent+=leni;
			if (sent==sizeof (send_size))
				break;
		}
	}
	/* --- send message --- */
	sent=0;
	while (1) {
		leni=send (*socki, (char *) msg_send+sent, send_size-sent, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, NULL, &writeSock, NULL, NULL);
				if (ret==SOCKET_ERROR) {
//					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Send 4 Return is %d\n", ret);
//					#endif
					return (-4);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Send 5 Return is %d\n", ret);
//				#endif
				return (-5);
			}
		}
		else if (!leni) {
//			#ifndef _AFXDLL
//			RtPrintf ("Send 6 Return is %d\n", ret);
//			#endif
			return (-6);
		}
		else {
			sent+=leni;
			if (sent==send_size)
				break;
		}
	}
	return (send_size);
}

/*******************************************************************************
rt_sok_msg_send_nb_bj: sends a message though a socket. Non blocking mode.

	socki		pointer to socket where msg_send is to be sent
	msg_send	pointer to structure (buffer, struct, ..) to be sent
	send_size	size of msg_send

  
	return:		< 0=NOK
				n  = length

*******************************************************************************/
int rt_sok_msg_send_nb_bj (SOCKET *socki, void *msg_send, int send_size, int size)
{
	int leni, sent, ret;
	fd_set writeSock;
	struct timeval timeout;
	int sizi;

	timeout.tv_sec=0;
	timeout.tv_usec=50000;
	FD_ZERO (&writeSock);
	FD_SET (*socki, &writeSock);

	/* --- send message length --- */
	sizi=send_size+4;	// the protocol wants the length of the length too
	sent=0;
	while (!size) {
		leni=send (*socki, (char *) (&sizi)+sent, sizeof (sizi)-sent, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, NULL, &writeSock, NULL, NULL);
				if (ret==SOCKET_ERROR) {
//					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Send 1 Return is %d\n", ret);
//					#endif
					return (-1);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Send 2 Return is %d\n", ret);
//				#endif
				return (-2);
			}
		}
		else if (!leni) {
//			#ifndef _AFXDLL
//			RtPrintf ("Send 3 Return is %d\n", ret);
//			#endif
			return (-3);
		}
		else {
			sent+=leni;
			if (sent==sizeof (sizi))
				break;
		}
	}
	/* --- send message --- */
	sent=0;
	while (1) {
		leni=send (*socki, (char *) msg_send+sent, send_size-sent, 0);
		if (leni==SOCKET_ERROR) {
			ret=WSAGetLastError();
			if (ret==WSAEWOULDBLOCK) {
				ret=select (0, NULL, &writeSock, NULL, NULL);
				if (ret==SOCKET_ERROR) {
//					ret=WSAGetLastError();
//					#ifndef _AFXDLL
//					RtPrintf ("Send 4 Return is %d\n", ret);
//					#endif
					return (-4);
				}
				continue;
			}
			else {
//				#ifndef _AFXDLL
//				RtPrintf ("Send 5 Return is %d\n", ret);
//				#endif
				return (-5);
			}
		}
		else if (!leni) {
//			#ifndef _AFXDLL
//			RtPrintf ("Send 6 Return is %d\n", ret);
//			#endif
			return (-6);
		}
		else {
			sent+=leni;
			if (sent==send_size)
				break;
		}
	}
	return (send_size);
}


/*******************************************************************************
rt_sok_set_nb: Sets the socket to none blocking mode

	socki		pointer to socket where msg_send is to be sent
	return:		0=OK, else error

*******************************************************************************/
int	rt_sok_set_nb (SOCKET *socki)
{
	unsigned long l=1;

	if (ioctlsocket (*socki, FIONBIO, &l))
		return (1);
	return (0);
}

/*******************************************************************************
rt_sok_send_nb: Sends a message thriugh the socket in none blocking mode, no length control

	socki		pointer to socket where msg_send is to be sent
	msg_send	pointer to structure (buffer, struct, ..) to be sent
	send_size	size of msg_send

  
	return:		< 0=NOK
				n  = length

*******************************************************************************/
int	rt_sok_send_nb (SOCKET *socki, void *msg_send, int send_size)
{
	int l, sent, err;
	fd_set writeSock;

	FD_ZERO (&writeSock);
	FD_SET ( *socki, &writeSock);

	/* --- send message length --- */
	sent=0;
	while (1) {
		l = send ( *socki, (char *) msg_send+sent, send_size-sent, MSG_DONTROUTE);

		if (l==SOCKET_ERROR) {
			err = WSAGetLastError();
			if (err==WSAEWOULDBLOCK) {
				if (select (0, NULL, &writeSock, NULL, NULL)==SOCKET_ERROR) 
					return (-1);
				else continue;
			}
			else {
				return (0-err);
			}
		}
		else if (l==0) return (-3);
		else {
			sent+=l;
			if (sent==send_size) {
				return(sent);
			}
		}
	}
}



/*******************************************************************************
rt_sok_receive_nb: Receives message the socket to none blocking mode

	socki			pointer to socket where msg is goint to come_send is to be sent
	msg_send		pointer to structure (buffer, struct, ..) to be sent
	max_rec_size	maximum size of message to be received

  return	0,<0  = NOK: socket closed
			n     = length

*******************************************************************************/
int	rt_sok_receive_nb (SOCKET *socki, void *msg_rec, int max_rec_size, int size)
{
	int l;
	int rec;
	int err;
	fd_set readSock;

	FD_ZERO(&readSock);
	FD_SET( *socki, &readSock);

	rec=0;
	while(1) {
		l=recv( *socki, (char*) msg_rec+rec, max_rec_size-rec, 0);
		if (l==SOCKET_ERROR) {
			err = WSAGetLastError();
			if (err==WSAEWOULDBLOCK) {
				if (select (0, &readSock, NULL, NULL, NULL)==SOCKET_ERROR) 
					return (-1);
				else {
					continue;
				}
			}
			else {
//				s->errCode=err;
//				printf("Receive Error=%d\n", err);
				return (-2);
			}
		}
//		else if (s->type==SOCK_DGRAM) return(l);
		else if (l<=0) return (l);
		else {
			size=l;
			return( l);		// SOCK_DGRAM. B.S. but that is what it is
//			rec += l;
//			if (rec>=max_rec_size) {
//				return(rec);
//			}
		}
	}
}



/*******************************************************************************
rt_sok_msg_receive_gj108: receives a message from a socket. Non blocking mode.

	socki			pointer to socket where msg is goint to come_send is to be sent
	msg_send		pointer to structure (buffer, struct, ..) to be sent
	max_rec_size	maximum size of message to be received

  return	0,<0  = NOK: socket closed
			n     = length

*******************************************************************************/
int	rt_sok_msg_receive_gj108 (SOCKET *socki, void *msg_rec, int max_rec_size)
{
	int l;
	char *p_str;
	static char buffi[ 8];
	static int *lenp;

	p_str=msg_rec;
	// --- get length ---
	l = recv_nb108 (socki, p_str, sizeof ( int), 0);
	if (l==0) return (l);
	if (l<0)  return (l-3);

	lenp = (int*) msg_rec;

	if ( *lenp > max_rec_size){
		return( -2);
	}

	l = recv_nb108 (socki, p_str+sizeof( int), *lenp-sizeof( int), 0);

	if (l==0) return (l);
	if (l<0)  return (l-3);

	return ( *lenp);
}




/*******************************************************************************
rt_sok_msg_send_gj108: sends a message though a socket. Non blocking mode.

	socki		pointer to socket where msg_send is to be sent
	msg_send	pointer to structure (buffer, struct, ..) to be sent
	send_size	size of msg_send

  
	return:		< 0=NOK
				n  = length

*******************************************************************************/
int rt_sok_msg_send_gj108(SOCKET *socki, void *msg_send, int send_size)
{
	int len;

printf("rt_sok_msg_send_gj108: len=%d\n", send_size);
	// --- send message ---
	if (send_size) len = send_nb108 (socki, msg_send, send_size, MSG_DONTROUTE);
	else return 0;

	if (len==0) return (len);
	if (len<0)  return (len-3);

	return (len);
}


int recv_nb108(SOCKET *s, void *buf, int len, int flags)
{
	int l;
	int rec;
	int err;
	fd_set readSock;

	FD_ZERO (&readSock);
	FD_SET ( *s, &readSock);

	rec=0;
	while(1) {
		l=recv ( *s, &((char*)buf)[rec], len-rec, flags);
		if (l==SOCKET_ERROR) {
			err = WSAGetLastError();
			if (err==WSAEWOULDBLOCK) {
				if (select (0, &readSock, NULL, NULL, NULL)==SOCKET_ERROR) 
					return (-1);
				else {
					continue;
				}
			}
			else {
//				s->errCode=err;
//				printf("Receive Error=%d\n", err);
				return (-2);
			}
		}
//		else if (s->type==SOCK_DGRAM) return(l);
		else if (l<=0) return (l);
		else {
			rec += l;
			if (rec>=len) {
				return(rec);
			}
		}
	}
}


/*******************************************************************************

*******************************************************************************/
int send_nb108(SOCKET *s, const void *buf, int len, int flags)
{
	int l, sent, err;
	fd_set writeSock;

	FD_ZERO (&writeSock);
	FD_SET (*s, &writeSock);

	sent=0;
	while (1) {
//		if (s->type==SOCK_DGRAM)
//			l = sendto (*s, &(((char*)buf)[sent]), len-sent, flags|MSG_DONTROUTE, (struct sockaddr *)&s->ipAddr, sizeof(s->ipAddr));
//		else
			l = send (*s, &(((char*)buf)[sent]), len-sent, flags);

		if (l==SOCKET_ERROR) {
			err = WSAGetLastError();
			if (err==WSAEWOULDBLOCK) {
				if (select (0, NULL, &writeSock, NULL, NULL)==SOCKET_ERROR) 
					return (-1);
				else continue;
			}
			else {
//				s->errCode = err;
				return (-2);
			}
		}
		else if (l==0) return (-3);
		else {
			sent+=l;
			if (sent==len) {
				return(sent);
			}
		}
	}
}




/* ---------------------------------------------------------------------------- */


