/*
 * Copyright 1998-2000 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* Adapted for inclusion into OpenLDAP by Kurt D. Zeilenga */
/*---
 * Copyright (C) 1999, 2000 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES. USE, MODIFICATION, AND REDISTRIBUTION OF THIS WORK IS SUBJECT
 * TO VERSION 2.0.1 OF THE OPENLDAP PUBLIC LICENSE, A COPY OF WHICH IS
 * AVAILABLE AT HTTP://WWW.OPENLDAP.ORG/LICENSE.HTML OR IN THE FILE "LICENSE"
 * IN THE TOP-LEVEL DIRECTORY OF THE DISTRIBUTION. ANY USE OR EXPLOITATION
 * OF THIS WORK OTHER THAN AS AUTHORIZED IN VERSION 2.0.1 OF THE OPENLDAP
 * PUBLIC LICENSE, OR OTHER PRIOR WRITTEN CONSENT FROM NOVELL, COULD SUBJECT
 * THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.
 *
 *---*/

#include "portable.h"

#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

#define LDAP_MATCHRULE_IDENTIFIER      0x80L
#define LDAP_REVERSEORDER_IDENTIFIER   0x81L
#define LDAP_ATTRTYPES_IDENTIFIER      0x80L



/* ---------------------------------------------------------------------------
   countKeys
   
   Internal function to determine the number of keys in the string.
   
   keyString  (IN) String of items separated by whitespace.
   ---------------------------------------------------------------------------*/

static int countKeys(char *keyString)
{
	char *p = keyString;
	int count = 0;

	for (;;)
	{
		while (LDAP_SPACE(*p))		 /* Skip leading whitespace */
			p++;

		if (*p == '\0')			/* End of string? */
			return count;

		count++;				/* Found start of a key */

		while (!LDAP_SPACE(*p))	/* Skip till next space or end of string. */
			if (*p++ == '\0')
				return count;
	}
}


/* ---------------------------------------------------------------------------
   readNextKey
   
   Internal function to parse the next sort key in the string.
   Allocate an LDAPSortKey structure and initialize it with
   attribute name, reverse flag, and matching rule OID.

   Each sort key in the string has the format:
	  [whitespace][-]attribute[:[OID]]

   pNextKey    (IN/OUT) Points to the next key in the sortkey string to parse.
						The pointer is updated to point to the next character
						after the sortkey being parsed.
						
   key         (OUT)    Points to the address of an LDAPSortKey stucture
						which has been allocated by this routine and
						initialized with information from the next sortkey.                        
   ---------------------------------------------------------------------------*/

static int readNextKey( char **pNextKey, LDAPSortKey **key)
{
	char *p = *pNextKey;
	int rev = 0;
	char *attrStart;
	int attrLen;
	char *oidStart = NULL;
	int oidLen = 0;

	/* Skip leading white space. */
	while (LDAP_SPACE(*p))
		p++;

	if (*p == '-')		 /* Check if the reverse flag is present. */
	{
		rev=1;
		p++;
	}

	/* We're now positioned at the start of the attribute. */
	attrStart = p;

	/* Get the length of the attribute until the next whitespace or ":". */
	attrLen = strcspn(p, " \t:");
	p += attrLen;

	if (attrLen == 0)	 /* If no attribute name was present, quit. */
		return LDAP_PARAM_ERROR;

	if (*p == ':')
	{
		oidStart = ++p;				 /* Start of the OID, after the colon */
		oidLen = strcspn(p, " \t");	 /* Get length of OID till next whitespace */
		p += oidLen;
	}

	*pNextKey = p;		 /* Update argument to point to next key */

	/* Allocate an LDAPSortKey structure */
	*key = LDAP_MALLOC(sizeof(LDAPSortKey));
	if (*key == NULL) return LDAP_NO_MEMORY;

	/* Allocate memory for the attribute and copy to it. */
	(*key)->attributeType = LDAP_MALLOC(attrLen+1);
	if ((*key)->attributeType == NULL) {
		LDAP_FREE(*key);
		return LDAP_NO_MEMORY;
	}

	strncpy((*key)->attributeType, attrStart, attrLen);
	(*key)->attributeType[attrLen] = 0;

	/* If present, allocate memory for the OID and copy to it. */
	if (oidLen) {
		(*key)->orderingRule = LDAP_MALLOC(oidLen+1);
		if ((*key)->orderingRule == NULL) {
			LDAP_FREE((*key)->attributeType);
			LDAP_FREE(*key);
			return LDAP_NO_MEMORY;
		}
		strncpy((*key)->orderingRule, oidStart, oidLen);
		(*key)->orderingRule[oidLen] = 0;

	} else {
		(*key)->orderingRule = NULL;
	}

	(*key)->reverseOrder = rev;

	return LDAP_SUCCESS;
}


/* ---------------------------------------------------------------------------
   ldap_create_sort_keylist
   
   Create an array of pointers to LDAPSortKey structures, containing the
   information specified by the string representation of one or more
   sort keys.
   
   sortKeyList    (OUT) Points to a null-terminated array of pointers to
						LDAPSortKey structures allocated by this routine.
						This memory SHOULD be freed by the calling program
						using ldap_free_sort_keylist().
						
   keyString      (IN)  Points to a string of one or more sort keys.                      
   
   ---------------------------------------------------------------------------*/

int
ldap_create_sort_keylist ( LDAPSortKey ***sortKeyList, char *keyString )
{
	int         numKeys, rc, i;
	char        *nextKey;
	LDAPSortKey **keyList = NULL;

	if (( sortKeyList == NULL ) || ( keyString == NULL )) {
		return LDAP_PARAM_ERROR;
	}
	*sortKeyList = NULL;

	/* Determine the number of sort keys so we can allocate memory. */
	if (( numKeys = countKeys(keyString)) == 0) {
		return LDAP_PARAM_ERROR;
	}

	/* Allocate the array of pointers.  Initialize to NULL. */
	keyList=(LDAPSortKey**)LBER_CALLOC(numKeys+1, sizeof(LDAPSortKey*));
	if ( keyList == NULL) return LDAP_NO_MEMORY;

	/* For each sort key in the string, create an LDAPSortKey structure
	   and add it to the list.
	*/
	nextKey = keyString;		  /* Points to the next key in the string */
	for (i=0; i < numKeys; i++) {
		rc = readNextKey(&nextKey, &keyList[i]);

		if (rc != LDAP_SUCCESS) {
			ldap_free_sort_keylist(keyList);
			return rc;
		}
	}

	*sortKeyList = keyList;
	return LDAP_SUCCESS;
}


/* ---------------------------------------------------------------------------
   ldap_free_sort_keylist
   
   Frees the sort key structures created by ldap_create_sort_keylist().
   Frees the memory referenced by the LDAPSortKey structures,
   the LDAPSortKey structures themselves, and the array of pointers
   to the structures.
   
   keyList     (IN) Points to an array of pointers to LDAPSortKey structures.
   ---------------------------------------------------------------------------*/

void
ldap_free_sort_keylist ( LDAPSortKey **keyList )
{
	int i;
	LDAPSortKey *nextKeyp;

	if (keyList == NULL) return;

	i=0;
	while ( 0 != (nextKeyp = keyList[i++]) ) {
		if (nextKeyp->attributeType) {
			LBER_FREE(nextKeyp->attributeType);
		}

		if (nextKeyp->orderingRule != NULL) {
			LBER_FREE(nextKeyp->orderingRule);
		}

		LBER_FREE(nextKeyp);
	}

	LBER_FREE(keyList);
}


/* ---------------------------------------------------------------------------
   ldap_create_sort_control
   
   Create and encode the server-side sort control.
   
   ld          (IN) An LDAP session handle, as obtained from a call to
					ldap_init().

   keyList     (IN) Points to a null-terminated array of pointers to
					LDAPSortKey structures, containing a description of
					each of the sort keys to be used.  The description
					consists of an attribute name, ascending/descending flag,
					and an optional matching rule (OID) to use.
			   
   isCritical  (IN) 0 - Indicates the control is not critical to the operation.
					non-zero - The control is critical to the operation.
					 
   ctrlp      (OUT) Returns a pointer to the LDAPControl created.  This control
					SHOULD be freed by calling ldap_control_free() when done.
   
   
   Ber encoding
   
   SortKeyList ::= SEQUENCE OF SEQUENCE {
		   attributeType   AttributeDescription,
		   orderingRule    [0] MatchingRuleId OPTIONAL,
		   reverseOrder    [1] BOOLEAN DEFAULT FALSE }
   
   ---------------------------------------------------------------------------*/

int
ldap_create_sort_control (
	LDAP *ld,
	LDAPSortKey **keyList,
	int isCritical,
	LDAPControl **ctrlp )
{
	int         i;
	BerElement  *ber;
	ber_tag_t tag;


	if ( (ld == NULL) || (keyList == NULL) || (ctrlp == NULL) ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return(ld->ld_errno);
	}

	if ((ber = ldap_alloc_ber_with_options(ld)) == NULL) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return( ld->ld_errno );
	}

	tag = ber_printf(ber, "{" /*}*/);
	if (tag == LBER_ERROR) goto exit;

	for (i = 0; keyList[i] != NULL; i++) {
		tag = ber_printf(ber, "{s" /*}*/, (keyList[i])->attributeType);
		if (tag == LBER_ERROR) goto exit;

		if ((keyList[i])->orderingRule != NULL) {
			tag = ber_printf( ber, "ts",
				LDAP_MATCHRULE_IDENTIFIER,
				(keyList[i])->orderingRule );

			if( tag == LBER_ERROR ) goto exit;
		}

		if ((keyList[i])->reverseOrder) {
			tag = ber_printf(ber, "tb",
				LDAP_REVERSEORDER_IDENTIFIER,
				(keyList[i])->reverseOrder );

			if( tag == LBER_ERROR ) goto exit;
		}

		tag = ber_printf(ber, /*{*/ "N}");
		if( tag == LBER_ERROR ) goto exit;
	}

	tag = ber_printf(ber, /*{*/ "N}");
	if( tag == LBER_ERROR ) goto exit;

	ld->ld_errno = ldap_create_control( LDAP_CONTROL_SORTREQUEST,
		ber, isCritical, ctrlp);

	ber_free(ber, 1);

	return(ld->ld_errno);

exit:
	ber_free(ber, 1);
	ld->ld_errno =  LDAP_ENCODING_ERROR;
	return(ld->ld_errno);
}


/* ---------------------------------------------------------------------------
   ldap_parse_sort_control
   
   Decode the server-side sort control return information.

   ld          (IN) An LDAP session handle, as obtained from a call to
					ldap_init().

   ctrls       (IN) The address of a NULL-terminated array of LDAPControl
					structures, typically obtained by a call to
					ldap_parse_result().
				  
   returnCode (OUT) This result parameter is filled in with the sort control
					result code.  This parameter MUST not be NULL.
				  
   attribute  (OUT) If an error occured the server may return a string
					indicating the first attribute in the sortkey list
					that was in error.  If a string is returned, the memory
					should be freed with ldap_memfree.  If this parameter is
					NULL, no string is returned.
   
			   
   Ber encoding for sort control
	 
	 SortResult ::= SEQUENCE {
		sortResult  ENUMERATED {
			success                   (0), -- results are sorted
			operationsError           (1), -- server internal failure
			timeLimitExceeded         (3), -- timelimit reached before
										   -- sorting was completed
			strongAuthRequired        (8), -- refused to return sorted
										   -- results via insecure
										   -- protocol
			adminLimitExceeded       (11), -- too many matching entries
										   -- for the server to sort
			noSuchAttribute          (16), -- unrecognized attribute
										   -- type in sort key
			inappropriateMatching    (18), -- unrecognized or inappro-
										   -- priate matching rule in
										   -- sort key
			insufficientAccessRights (50), -- refused to return sorted
										   -- results to this client
			busy                     (51), -- too busy to process
			unwillingToPerform       (53), -- unable to sort
			other                    (80)
			},
	  attributeType [0] AttributeDescription OPTIONAL }
   ---------------------------------------------------------------------------*/

int
ldap_parse_sort_control(
	LDAP           *ld,
	LDAPControl    **ctrls,
	unsigned long  *returnCode,
	char           **attribute )
{
	BerElement *ber;
	LDAPControl *pControl;
	int i;
	ber_tag_t tag, berTag;
	ber_len_t berLen;

	if (ld == NULL) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return(ld->ld_errno);
	}

	if (ctrls == NULL) {
		ld->ld_errno =  LDAP_CONTROL_NOT_FOUND;
		return(ld->ld_errno);
	}

	if (attribute) {
		*attribute = NULL;
	}

	/* Search the list of control responses for a sort control. */
	for (i=0; ctrls[i]; i++) {
		pControl = ctrls[i];
		if (!strcmp(LDAP_CONTROL_SORTRESPONSE, pControl->ldctl_oid))
			goto foundSortControl;
	}

	/* No sort control was found. */
	ld->ld_errno = LDAP_CONTROL_NOT_FOUND;
	return(ld->ld_errno);

foundSortControl:
	/* Create a BerElement from the berval returned in the control. */
	ber = ber_init(&pControl->ldctl_value);

	if (ber == NULL) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return(ld->ld_errno);
	}

	/* Extract the result code from the control. */
	tag = ber_scanf(ber, "{e" /*}*/, returnCode);

	if( tag == LBER_ERROR ) {
		ber_free(ber, 1);
		ld->ld_errno = LDAP_DECODING_ERROR;
		return(ld->ld_errno);
	}

	/* If caller wants the attribute name, and if it's present in the control,
	   extract the attribute name which caused the error. */
	if (attribute && (LDAP_ATTRTYPES_IDENTIFIER == ber_peek_tag(ber, &berLen)))
	{
		tag = ber_scanf(ber, "ta", &berTag, attribute);

		if (tag == LBER_ERROR ) {
			ber_free(ber, 1);
			ld->ld_errno = LDAP_DECODING_ERROR;
			return(ld->ld_errno);
		}
	}

	ber_free(ber,1);

	ld->ld_errno = LDAP_SUCCESS;
	return(ld->ld_errno);
}
