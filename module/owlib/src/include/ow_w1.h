/*
$Id$
W! Announce -- daemon  for showing w1 busmasters using Avahi
Written 2008 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPLv2
Much thanks to Evgeniy Polyakov
*/

#if OW_W1
#ifndef OW_W1_H
#define OW_W1_H

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include "netlink.h"
#include "connector.h"
#include "w1_netlink.h"

#define W1_NLM_LENGTH	16
#define W1_CN_LENGTH	20
#define W1_W1M_LENGTH	12
#define W1_W1C_LENGTH	4

struct connection_in ;

int w1_bind( void ) ;
void w1_unbind( void ) ;

void RemoveW1Bus( int bus_master ) ;
void AddW1Bus( int bus_master ) ;
int W1_send_msg( struct connection_in * in, struct w1_netlink_msg *msg, struct w1_netlink_cmd *cmd) ;
int W1PipeSelect_timeout( int file_descriptor ) ;
int W1PipeSelect_no_timeout( int file_descriptor ) ;
int W1Select_no_timeout( void ) ;
void * W1_Dispatch( void * v ) ;

int W1NLScan( void ) ;
int W1NLList( void ) ;

#define MAKE_NL_SEQ( bus, seq )  ((uint32_t)(( ((bus) & 0xFFFF) << 16 ) | ((seq) & 0xFFFF)))
#define NL_SEQ( seq )  ((uint32_t)((seq) & 0xFFFF))
#define NL_BUS( seq )  ((uint32_t)(((seq) >> 16 ) & 0xFFFF))

int Announce_Control_init( int allocated ) ;

struct netlink_parse {
	struct nlmsghdr *	nlm ;
	struct cn_msg * 	cn ;
	struct w1_netlink_msg *	w1m ;
	struct w1_netlink_cmd *	w1c ;
	unsigned char *		data ;
	int			data_size ;
} ;

void Netlink_Parse_Destroy( struct netlink_parse * nlp ) ;
int Netlink_Parse_Get( struct netlink_parse * nlp ) ;
int Get_and_Parse_Pipe( int file_descriptor, struct netlink_parse * nlp ) ;
void Netlink_Print( struct nlmsghdr * nlm, struct cn_msg * cn, struct w1_netlink_msg * w1m, struct w1_netlink_cmd * w1c, unsigned char * data, int length ) ;

#endif 	/* OW_W1_H */
#endif /* OW_W1 */