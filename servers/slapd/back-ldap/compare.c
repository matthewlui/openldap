/* compare.c - ldap backend compare function */
/* $OpenLDAP$ */
/*
 * Copyright 1998-2000 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* This is an altered version */
/*
 * Copyright 1999, Howard Chu, All rights reserved. <hyc@highlandsun.com>
 * 
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 * 
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 * 
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 * 
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 * 
 * 4. This notice may not be removed or altered.
 *
 *
 *
 * Copyright 2000, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 * 
 * This software is being modified by Pierangelo Masarati.
 * The previously reported conditions apply to the modified code as well.
 * Changes in the original code are highlighted where required.
 * Credits for the original code go to the author, Howard Chu.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-ldap.h"

int
ldap_back_compare(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    const char	*dn,
    const char	*ndn,
	AttributeAssertion *ava
)
{
	struct ldapinfo	*li = (struct ldapinfo *) be->be_private;
	struct ldapconn *lc;
	char *mdn, *mapped_oc, *mapped_at;

	lc = ldap_back_getconn(li, conn, op);
	if (!lc || !ldap_back_dobind( lc, op ) ) {
		return( -1 );
	}

	/*
	 * Rewrite the compare dn, if needed
	 */
#ifdef ENABLE_REWRITE
	switch ( rewrite_session( li->rwinfo, "compareDn", dn, conn, &mdn ) ) {
	case REWRITE_REGEXEC_OK:
		if ( mdn == NULL ) {
			mdn = ( char * )dn;
		}
		Debug( LDAP_DEBUG_ARGS, "rw> compareDn: \"%s\" -> \"%s\"\n%s",
				dn, mdn, "" );
		break;
		
	case REWRITE_REGEXEC_UNWILLING:
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM,
				NULL, "Unwilling to perform", NULL, NULL );
		
	case REWRITE_REGEXEC_ERR:
		return( -1 );
	}
#else /* !ENABLE_REWRITE */
	mdn = ldap_back_dn_massage( li, ch_strdup( dn ), 0 );
 	if ( mdn == NULL ) {
 		return -1;
	}
#endif /* !ENABLE_REWRITE */

	mapped_oc = ldap_back_map(&li->oc_map, ava->aa_desc->ad_cname->bv_val, 0);
	if (mapped_oc == NULL)
		return( -1 );

	mapped_at = ldap_back_map(&li->at_map, ava->aa_value->bv_val, 0);
	if (mapped_at == NULL)
		return( -1 );

	ldap_compare_s( lc->ld, mdn, mapped_oc, mapped_at );

#ifdef ENABLE_REWRITE
	if ( mdn != dn ) {
#endif /* ENABLE_REWRITE */
		free( mdn );
#ifdef ENABLE_REWRITE
	}
#endif /* ENABLE_REWRITE */
	
	return( ldap_back_op_result( lc, op ) );
}
