/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

int fakes = 0 ;

/* All the rest of the program sees is the Fake_detect and the entry in iroutines */

static int Fake_reset( const struct parsedname * pn ) ;
static int Fake_overdrive( const UINT ov, const struct parsedname * pn ) ;
static int Fake_testoverdrive( const struct parsedname * pn ) ;
static int Fake_ProgramPulse( const struct parsedname * pn ) ;
static int Fake_sendback_bits( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) ;
static void Fake_close( struct connection_in * in ) ;
static int Fake_next_both(struct device_search * ds, const struct parsedname * pn) ;
static const ASCII * namefind( const char * name ) ;

/* Device-specific functions */
/* Note, the "Bad"adapter" ha not function, and returns "-ENOTSUP" (not supported) for most functions */
/* It does call lower level functions for higher ones, which of course is pointless since the lower ones don't work either */
int Fake_detect( struct connection_in * in ) {
    ASCII * newname ;
    ASCII * oldname = in->name ;

    in->fd = fakes ;
    in->iroutines.detect        = Fake_detect ;
    in->Adapter = adapter_Bad ; /* OWFS assigned value */
    in->iroutines.reset         = Fake_reset         ;
    in->iroutines.next_both     = Fake_next_both     ;
    in->iroutines.overdrive     = Fake_overdrive     ;
    in->iroutines.testoverdrive = Fake_testoverdrive ;
    in->iroutines.PowerByte     = NULL               ;
    in->iroutines.ProgramPulse  = Fake_ProgramPulse  ;
    in->iroutines.sendback_data = NULL               ;
    in->iroutines.sendback_bits = Fake_sendback_bits ;
    in->iroutines.select        = NULL               ;
    in->iroutines.reconnect     = NULL               ;
    in->iroutines.close         = Fake_close         ;
    in->iroutines.transaction   = NULL               ;
    in->iroutines.flags         = ADAP_FLAG_2409path ;

    in->connin.fake.devices=0 ;
    in->connin.fake.device=NULL ;
    in->adapter_name = "Simulated" ;
    in->Adapter = adapter_fake ;
    in->AnyDevices = 0 ;
    LEVEL_CONNECT("Setting up Simulated (Fake) Bus Master (%d)\n",fakes) ;
    if ( (newname=(ASCII *)malloc(20)) ) {
        const ASCII * dev ;
        ASCII * rest = in->name ;
        int allocated = 0 ;

        snprintf(newname,18,"fake.%d",fakes) ;
        in->name = newname ;

        while (rest) {
            BYTE sn[8] ;
            for ( dev=strsep( &rest, " ," ) ; dev[0] ; ++dev ) {
                if ( dev[0]!=' ' && dev[0]!=',' ) break ;
            }
            if ( (isxdigit(dev[0])&&isxdigit(dev[1])) || (dev=namefind(dev)) ) {
                sn[0] = string2num(dev) ;
                sn[1] = rand()&0xFF ;
                sn[2] = rand()&0xFF ;
                sn[3] = rand()&0xFF ;
                sn[4] = rand()&0xFF ;
                sn[5] = rand()&0xFF ;
                sn[6] = rand()&0xFF ;
                sn[7] = CRC8compute(sn,7,0) ;
                if ( in->connin.fake.devices >= allocated ) {
                    BYTE * temp = (BYTE *) realloc( in->connin.fake.device, allocated*8+88 ) ;
                    if ( temp ) {
                        allocated += 10 ;
                        in->connin.fake.device = temp ;
                    }
                }
                if ( in->connin.fake.devices < allocated ) {
                    memcpy( &(in->connin.fake.device[in->connin.fake.devices*8]), sn, 8 ) ;
                    in->AnyDevices = 1 ;
                    ++ in->connin.fake.devices ;
                }
            }
        }
        if (oldname) free(oldname) ;
    }
    ++ fakes ;
    return 0 ;
}

static int Fake_reset( const struct parsedname * pn ) {
    (void) pn ;
    return 0 ;
}
static int Fake_overdrive( const UINT ov, const struct parsedname * pn ) {
    (void) ov ;
    (void) pn ;
    return 0 ;
}
static int Fake_testoverdrive( const struct parsedname * pn ) {
    (void) pn ;
    return 0 ;
}
static int Fake_ProgramPulse( const struct parsedname * pn ) {
    (void) pn ;
    return 0 ;
}
static int Fake_sendback_bits( const BYTE * data , BYTE * resp, const size_t len, const struct parsedname * pn ){
    (void) pn ;
    (void) data ;
    (void) resp ;
    (void) len ;
    return 0 ;
}
static void Fake_close( struct connection_in * in ) {
    (void) in ;
}

static int Fake_next_both(struct device_search * ds, const struct parsedname * pn) {
    //printf("Fake_next_both LastDiscrepancy=%d, devices=%d, LastDevice=%d, AnyDevice=%d\n",ds->LastDiscrepancy,pn->in->connin.fake.devices,ds->LastDevice,pn->in->AnyDevices);
    if ( ++ds->LastDiscrepancy >= pn->in->connin.fake.devices 
        ||        
         ! pn->in->AnyDevices 
        ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;
    memcpy( ds->sn, &(pn->in->connin.fake.device[8*ds->LastDiscrepancy]), 8 ) ;
    return 0 ;
}

#ifdef NO_NESTED_FUNCTIONS

#if OW_MT
pthread_mutex_t Namefindmutex = PTHREAD_MUTEX_INITIALIZER ;
#endif /* OW_MT */
ASCII * Namefindret = NULL;
char * Namefindname = NULL;

void Namefindaction(const void *nodep, const VISIT which, const int depth) {
    const struct device * p = *(struct device * const *) nodep ;
    (void) depth ;
    //printf("Comparing %s|%s with %s\n",p->name ,p->code , Namefindname ) ;
    switch(which) {
    case leaf:
    case postorder:
        if ( strcasecmp(p->name, Namefindname)==0 || strcasecmp(p->code, Namefindname)==0 ) {
            Namefindret = p->code ;
        }
    case preorder:
    case endorder:
        break;
    }
}

static const ASCII * namefind( const char * name ) {
#if OW_MT
    pthread_mutex_lock(&Namefindmutex) ;
#endif /* OW_MT */

    Namefindname = name;
    twalk( Tree[pn_real], Namefindaction ) ;

#if OW_MT
    pthread_mutex_lock(&Namefindmutex) ;
#endif /* OW_MT */
    return Namefindret;
}

#else /* NO_NESTED_FUNCTIONS */

static const ASCII * namefind( const char * name ) {
    const ASCII * ret = NULL ;
    /* Embedded function */
    void action(const void *nodep, const VISIT which, const int depth) {
        const struct device * p = *(struct device * const *) nodep ;
        (void) depth ;
        //printf("Comparing %s|%s with %s\n",p->name ,p->code , name ) ;
        switch(which) {
        case leaf:
        case postorder:
            if ( strcasecmp(p->name,name)==0 || strcasecmp(p->code,name)==0 ) {
                ret = p->code ;
            }
        case preorder:
        case endorder:
            break;
        }
    }

    twalk( Tree[pn_real], action ) ;
    return ret ;
}

#endif /* NO_NESTED_FUNCTIONS */
