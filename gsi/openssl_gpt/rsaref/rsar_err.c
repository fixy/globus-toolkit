/* rsaref/rsar_err.c */
/* ====================================================================
 * Copyright (c) 1999 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

/* NOTE: this file was auto generated by the mkerr.pl script: any changes
 * made to it will be overwritten when the script next updates this file,
 * only reason strings will be preserved.
 */

#include <stdio.h>
#include <openssl/err.h>
#include <openssl/rsaref.h>

/* BEGIN ERROR CODES */
#ifndef NO_ERR
static ERR_STRING_DATA RSAREF_str_functs[]=
	{
{ERR_PACK(0,RSAREF_F_BN_REF_MOD_EXP,0),	"BN_REF_MOD_EXP"},
{ERR_PACK(0,RSAREF_F_RSAREF_BN2BIN,0),	"RSAREF_BN2BIN"},
{ERR_PACK(0,RSAREF_F_RSA_BN2BIN,0),	"RSA_BN2BIN"},
{ERR_PACK(0,RSAREF_F_RSA_PRIVATE_DECRYPT,0),	"RSA_private_decrypt"},
{ERR_PACK(0,RSAREF_F_RSA_PRIVATE_ENCRYPT,0),	"RSA_private_encrypt"},
{ERR_PACK(0,RSAREF_F_RSA_PUBLIC_DECRYPT,0),	"RSA_public_decrypt"},
{ERR_PACK(0,RSAREF_F_RSA_PUBLIC_ENCRYPT,0),	"RSA_public_encrypt"},
{ERR_PACK(0,RSAREF_F_RSA_REF_BN2BIN,0),	"RSA_REF_BN2BIN"},
{ERR_PACK(0,RSAREF_F_RSA_REF_MOD_EXP,0),	"RSA_REF_MOD_EXP"},
{ERR_PACK(0,RSAREF_F_RSA_REF_PRIVATE_DECRYPT,0),	"RSA_REF_PRIVATE_DECRYPT"},
{ERR_PACK(0,RSAREF_F_RSA_REF_PRIVATE_ENCRYPT,0),	"RSA_REF_PRIVATE_ENCRYPT"},
{ERR_PACK(0,RSAREF_F_RSA_REF_PUBLIC_DECRYPT,0),	"RSA_REF_PUBLIC_DECRYPT"},
{ERR_PACK(0,RSAREF_F_RSA_REF_PUBLIC_ENCRYPT,0),	"RSA_REF_PUBLIC_ENCRYPT"},
{0,NULL}
	};

static ERR_STRING_DATA RSAREF_str_reasons[]=
	{
{RSAREF_R_CONTENT_ENCODING               ,"content encoding"},
{RSAREF_R_DATA                           ,"data"},
{RSAREF_R_DIGEST_ALGORITHM               ,"digest algorithm"},
{RSAREF_R_ENCODING                       ,"encoding"},
{RSAREF_R_ENCRYPTION_ALGORITHM           ,"encryption algorithm"},
{RSAREF_R_KEY                            ,"key"},
{RSAREF_R_KEY_ENCODING                   ,"key encoding"},
{RSAREF_R_LEN                            ,"len"},
{RSAREF_R_MODULUS_LEN                    ,"modulus len"},
{RSAREF_R_NEED_RANDOM                    ,"need random"},
{RSAREF_R_PRIVATE_KEY                    ,"private key"},
{RSAREF_R_PUBLIC_KEY                     ,"public key"},
{RSAREF_R_SIGNATURE                      ,"signature"},
{RSAREF_R_SIGNATURE_ENCODING             ,"signature encoding"},
{0,NULL}
	};

#endif

void ERR_load_RSAREF_strings(void)
	{
	static int init=1;

	if (init)
		{
		init=0;
#ifndef NO_ERR
		ERR_load_strings(ERR_LIB_RSAREF,RSAREF_str_functs);
		ERR_load_strings(ERR_LIB_RSAREF,RSAREF_str_reasons);
#endif

		}
	}