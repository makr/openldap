/* time.c - deal with time subsystem */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*
 * Copyright 2001, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 * 
 * This work has beed deveolped for the OpenLDAP Foundation 
 * in the hope that it may be useful to the Open Source community, 
 * but WITHOUT ANY WARRANTY.
 * 
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 * 
 * 1. The author and SysNet s.n.c. are not responsible for the consequences
 *    of use of this software, no matter how awful, even if they arise from
 *    flaws in it.
 * 
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 * 
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 *    SysNet s.n.c. cannot be responsible for the consequences of the
 *    alterations.
 * 
 * 4. This notice may not be removed or altered.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/time.h>


#include "slap.h"
#include <lutil.h>
#include "proto-slap.h"
#include "back-monitor.h"

int
monitor_subsys_time_init(
	BackendDB		*be
)
{
	struct monitorinfo	*mi;
	
	Entry			*e, *e_tmp, *e_time;
	struct monitorentrypriv	*mp;
	char			buf[ BACKMONITOR_BUFSIZE ];

	assert( be != NULL );

	mi = ( struct monitorinfo * )be->be_private;

	if ( monitor_cache_get( mi,
			&monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn, &e_time ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT,
			"monitor_subsys_time_init: "
			"unable to get entry '%s'\n",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to get entry '%s'\n%s%s",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val, 
			"", "" );
#endif
		return( -1 );
	}

	e_tmp = NULL;

	snprintf( buf, sizeof( buf ),
			"dn: cn=Start,%s\n"
			"objectClass: %s\n"
			"structuralObjectClass: %s\n"
			"cn: Start\n"
			"%s: %s\n"
			"createTimestamp: %s\n"
			"modifyTimestamp: %s\n", 
			monitor_subsys[SLAPD_MONITOR_TIME].mss_dn.bv_val,
			mi->mi_oc_monitoredObject->soc_cname.bv_val,
			mi->mi_oc_monitoredObject->soc_cname.bv_val,
			mi->mi_ad_monitorTimestamp->ad_cname.bv_val,
			mi->mi_startTime.bv_val,
			mi->mi_startTime.bv_val,
			mi->mi_startTime.bv_val );

	e = str2entry( buf );
	if ( e == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT,
			"monitor_subsys_time_init: "
			"unable to create entry 'cn=Start,%s'\n",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to create entry 'cn=Start,%s'\n%s%s",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val,
			"", "" );
#endif
		return( -1 );
	}
	
	mp = ( struct monitorentrypriv * )ch_calloc( sizeof( struct monitorentrypriv ), 1 );
	e->e_private = ( void * )mp;
	mp->mp_next = e_tmp;
	mp->mp_children = NULL;
	mp->mp_info = &monitor_subsys[SLAPD_MONITOR_TIME];
	mp->mp_flags = monitor_subsys[SLAPD_MONITOR_TIME].mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;

	if ( monitor_cache_add( mi, e ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT,
			"monitor_subsys_time_init: "
			"unable to add entry 'cn=Start,%s'\n",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to add entry 'cn=Start,%s'\n%s%s",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val,
			"", "" );
#endif
		return( -1 );
	}
	
	e_tmp = e;

	/*
	 * Current
	 */
	snprintf( buf, sizeof( buf ),
			"dn: cn=Current,%s\n"
			"objectClass: %s\n"
			"structuralObjectClass: %s\n"
			"cn: Current\n"
			"%s: %s\n"
			"createTimestamp: %s\n"
			"modifyTimestamp: %s\n",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_dn.bv_val,
			mi->mi_oc_monitoredObject->soc_cname.bv_val,
			mi->mi_oc_monitoredObject->soc_cname.bv_val,
			mi->mi_ad_monitorTimestamp->ad_cname.bv_val,
			mi->mi_startTime.bv_val,
			mi->mi_startTime.bv_val,
			mi->mi_startTime.bv_val );

	e = str2entry( buf );
	if ( e == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT,
			"monitor_subsys_time_init: "
			"unable to create entry 'cn=Current,%s'\n",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to create entry 'cn=Current,%s'\n%s%s",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val,
			"", "" );
#endif
		return( -1 );
	}

	mp = ( struct monitorentrypriv * )ch_calloc( sizeof( struct monitorentrypriv ), 1 );
	e->e_private = ( void * )mp;
	mp->mp_next = e_tmp;
	mp->mp_children = NULL;
	mp->mp_info = &monitor_subsys[SLAPD_MONITOR_TIME];
	mp->mp_flags = monitor_subsys[SLAPD_MONITOR_TIME].mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;

	if ( monitor_cache_add( mi, e ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT,
			"monitor_subsys_time_init: "
			"unable to add entry 'cn=Current,%s'\n",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to add entry 'cn=Current,%s'\n%s%s",
			monitor_subsys[SLAPD_MONITOR_TIME].mss_ndn.bv_val,
			"", "" );
#endif
		return( -1 );
	}
	
	e_tmp = e;

	mp = ( struct monitorentrypriv * )e_time->e_private;
	mp->mp_children = e_tmp;

	monitor_cache_release( mi, e_time );

	return( 0 );
}

int
monitor_subsys_time_update(
	Operation		*op,
	Entry                   *e
)
{
	struct monitorinfo *mi = (struct monitorinfo *)op->o_bd->be_private;

	assert( mi );
	assert( e );
	
	if ( strncmp( e->e_nname.bv_val, "cn=current",
				sizeof("cn=current") - 1 ) == 0 ) {
		struct tm	*tm;
		char		tmbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		Attribute	*a;
		ber_len_t	len;
		time_t		currtime;

		currtime = slap_get_time();

		ldap_pvt_thread_mutex_lock( &gmtime_mutex );
#ifdef HACK_LOCAL_TIME
		tm = localtime( &currtime );
		lutil_localtime( tmbuf, sizeof( tmbuf ), tm, -timezone );
#else /* !HACK_LOCAL_TIME */
		tm = gmtime( &currtime );
		lutil_gentime( tmbuf, sizeof( tmbuf ), tm );
#endif /* !HACK_LOCAL_TIME */
		ldap_pvt_thread_mutex_unlock( &gmtime_mutex );

		len = strlen( tmbuf );

		a = attr_find( e->e_attrs, mi->mi_ad_monitorTimestamp );
		if ( a == NULL ) {
			return( -1 );
		}

		assert( len == a->a_vals[0].bv_len );
		AC_MEMCPY( a->a_vals[0].bv_val, tmbuf, len );
	}

	return( 0 );
}

