
/*****************************************************************************
globus_gram_client.c

Description:
    Resource Managemant Client API's

    This file contains the Resource Management Client API funtion
    calls.  The resource management API provides functions for 
    submitting a job request to a RM, for asking when a job
    (submitted or not) might run, for cancelling a request,
    for requesting notification of state changes for a request,
    and for checking for pending notifications.

CVS Information:

    $Source$
    $Date$
    $Revision$
    $Author$
******************************************************************************/

/*****************************************************************************
                             Include header files
******************************************************************************/

#include "globus_config.h"
#include "globus_gram_client.h"
#include "globus_gram_protocol.h"
#include "globus_rsl.h"

#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/param.h>
#include <sys/time.h>
#include <globus_io.h>

#if defined(TARGET_ARCH_SOLARIS)
#include <netdb.h>
#endif

typedef
enum
{
    GLOBUS_GRAM_CLIENT_JOB_REQUEST,
    GLOBUS_GRAM_CLIENT_PING,
    GLOBUS_GRAM_CLIENT_STATUS,
    GLOBUS_GRAM_CLIENT_SIGNAL,
    GLOBUS_GRAM_CLIENT_CANCEL,
    GLOBUS_GRAM_CLIENT_CALLBACK_REGISTER,
    GLOBUS_GRAM_CLIENT_CALLBACK_UNREGISTER
}
globus_l_gram_client_callback_type_t;

typedef struct
{
    globus_mutex_t			mutex;
    globus_cond_t			cond;
    globus_gram_protocol_handle_t	handle;
    globus_l_gram_client_callback_type_t
					type;
    volatile globus_bool_t		done;
    volatile int			errorcode;

    /* For job request only */
    char *				contact;

    /* For job request / status */
    int					status;

    /* For job status only */
    int					job_failure_code;

    /* For register_job_request */
    globus_gram_client_callback_func_t	callback;
    void *				callback_arg;
} globus_l_gram_client_monitor_t;

typedef struct
{
    globus_gram_client_callback_func_t	callback;
    void *				callback_arg;
    char *				callback_contact;
}
globus_l_gram_client_callback_info_t;
/******************************************************************************
                          Module specific prototypes
******************************************************************************/

static
int
globus_l_gram_client_parse_gatekeeper_contact(
    const char *			contact_string,
    const char *			service_prefix,
    char **				gatekeeper_url,
    char **				gatekeeper_dn);

static int 
globus_l_gram_client_setup_attr_t(
    globus_io_attr_t *                     attrp,
    globus_io_secure_delegation_mode_t     delegation_mode,
    char *                                 gatekeeper_dn );

static
int
globus_l_gram_client_job_request(
    const char *			resource_manager_contact,
    const char *			description,
    int					job_state_mask,
    const char *			callback_contact,
    globus_l_gram_client_monitor_t *	monitor,
    globus_gram_client_callback_func_t	register_callback,
    void *				register_callback_arg);

static
void
globus_l_gram_client_callback(
    void *				arg,
    globus_gram_protocol_handle_t	handle,
    globus_byte_t *			buf,
    globus_size_t			nbytes,
    int					errorcode);

static
void
globus_l_gram_client_monitor_callback(
    void *				user_arg,
    globus_gram_protocol_handle_t	handle,
    globus_byte_t *			message,
    globus_size_t			msgsize,
    int					errorcode);

static
void
globus_l_gram_client_register_job_request_callback(
    void *				user_arg,
    globus_gram_protocol_handle_t	handle,
    globus_byte_t *			message,
    globus_size_t			msgsize,
    int					errorcode);

static
int
globus_l_gram_client_monitor_init(
    globus_l_gram_client_monitor_t *	monitor);

static
int
globus_l_gram_client_monitor_destroy(
    globus_l_gram_client_monitor_t *	monitor);

/******************************************************************************
                       Define module specific variables
******************************************************************************/

globus_module_descriptor_t globus_gram_client_module = 
{
    "globus_gram_client",
    globus_i_gram_client_activate,
    globus_i_gram_client_deactivate,
    GLOBUS_NULL
};

FILE *					globus_l_print_fp;
static globus_mutex_t			globus_l_mutex;
static int				globus_l_is_initialized = 0;
static globus_hashtable_t		globus_l_gram_client_contacts;

#define GLOBUS_L_CHECK_IF_INITIALIZED assert(globus_l_is_initialized==1)

/******************************************************************************
Function:	globus_i_gram_client_activate()
Description:	Initialize variables
		Call authorization routine for password entry.
Parameters:
Returns:
******************************************************************************/
int
globus_i_gram_client_activate(void)
{
    int rc;
    
    rc = globus_module_activate(GLOBUS_POLL_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
	return(rc);
    }

    rc = globus_module_activate(GLOBUS_IO_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
	return(rc);
    }
    rc = globus_module_activate(GLOBUS_GRAM_PROTOCOL_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
	return(rc);
    }

    
    if ( globus_l_is_initialized == 0 )
    {
	/* initialize mutex which makes the client thread-safe */
	int err;
	err = globus_mutex_init (&globus_l_mutex, NULL);
	assert (!err);
	globus_l_is_initialized = 1;

    }
    
    globus_l_print_fp = NULL;
    globus_hashtable_init(&globus_l_gram_client_contacts,
	                  16,
			  globus_hashtable_string_hash,
			  globus_hashtable_string_keyeq);

    /* globus_gram_client_debug(); */

    return 0;
} /* globus_i_gram_client_activate() */


/******************************************************************************
Function:	globus_i_gram_client_deactivate()
Description:
Parameters:
Returns:
******************************************************************************/
int
globus_i_gram_client_deactivate(void)
{
    int rc;

    if ( globus_l_is_initialized == 0 )
    {
	return(GLOBUS_FAILURE);
    }
    else
    {
	globus_l_is_initialized = 0;
    }

    /* 
     * this will free any allocated space, but not malloc any new
     */
    globus_gram_protocol_error_7_hack_replace_message((const char*) GLOBUS_NULL);
    
    rc = globus_module_deactivate(GLOBUS_GRAM_PROTOCOL_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
	return(rc);
    }

    rc = globus_module_deactivate(GLOBUS_IO_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
	return(rc);
    }

    rc = globus_module_deactivate(GLOBUS_POLL_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
	return(rc);
    }
    globus_hashtable_destroy(&globus_l_gram_client_contacts);

    return (GLOBUS_SUCCESS);
} /* globus_i_gram_client_deactivate() */


/******************************************************************************
Function:	globus_gram_client_debug()
Description:
Parameters:
Returns:
******************************************************************************/
void
globus_gram_client_debug(void)
{
    globus_l_print_fp = stdout;
    globus_libc_fprintf(globus_l_print_fp,
		  "globus_gram_client: debug messages will be printed.\n");
} /* globus_gram_client_debug() */


/******************************************************************************
Function:	globus_l_gram_client_parse_gatekeeper_url()
Description:
Parameters:
Returns:
******************************************************************************/
static
int
globus_l_gram_client_parse_gatekeeper_contact(
    const char *			contact_string,
    const char *			service_prefix,
    char **				gatekeeper_url,
    char **				gatekeeper_dn)
{
    char *				duplicate;
    char *				host = GLOBUS_NULL;
    char *				port = GLOBUS_NULL;
    char *				dn = GLOBUS_NULL;
    char *				service;
    int					got_port = 0;
    int					got_service = 0;
    char *				ptr;
    unsigned short			iport;
    globus_url_t			some_struct;

    /*
     *  the gatekeeper contact format: [https://]<host>:<port>[/<service>]:<dn>
     */    

    service = "jobmanager";
    iport = 2119;

    if ((duplicate = globus_libc_strdup(contact_string)))
    {
        host = duplicate;

        if (strncmp(duplicate,"https://", 8) == 0)
            host += 8;

        dn = host;

        for (ptr = duplicate; *ptr != '\0'; ptr++)
        {
            if ( *ptr == ':' )
            {
                got_port = 1;
                *ptr++ = '\0';
                port = ptr;
                break;
            }
            if ( *ptr == '/' )
            {
                got_service = 1;
                *ptr++ = '\0';
                service = ptr;
                break;
            }
        }

        if (got_port || got_service) 
        {
	    if ((dn = strchr(ptr, ':')))
	    {
		*dn++ = '\0';
	    }

            if (got_port)
            {
	        if (service = strchr(port,'/'))
                {
                    if ((service - port) > 1)
                    {
	                iport = (unsigned short) atoi(port);
                    }
                    *service++ = '\0';
                }
                else
                {
                    service = "jobmanager";
	            if (strlen(port) > 0)
	               iport = (unsigned short) atoi(port);
                }
            }
        }
        else
        {
            dn = GLOBUS_NULL;
        }
    } 
    else 
    {
	if(globus_l_print_fp)
	{
	    globus_libc_fprintf(globus_l_print_fp,
		                "strdup failed for contact_string\n");
	}
        return(GLOBUS_GRAM_PROTOCOL_ERROR_BAD_GATEKEEPER_CONTACT);
    }
    
    if (! *host)
    {
        globus_libc_free(duplicate);
	if(globus_l_print_fp)
	{
	    globus_libc_fprintf(globus_l_print_fp,
		                "empty host value in contact_string\n");
	}
       return(GLOBUS_GRAM_PROTOCOL_ERROR_BAD_GATEKEEPER_CONTACT);
    }

    (*gatekeeper_url) = globus_libc_malloc(11 /* https://:/\0 */ +
					   strlen(host) +
					   5 + /*unsigned short*/
					   strlen(service) +
					   ((service_prefix != GLOBUS_NULL)
					       ? strlen(service_prefix)
					       : 0));

    globus_libc_sprintf((*gatekeeper_url),
	                "https://%s:%hu%s/%s",
			host,
			(unsigned short) iport,
			((service_prefix != GLOBUS_NULL) ? service_prefix : ""),
			service);

    if (globus_url_parse(*gatekeeper_url, &some_struct) != GLOBUS_SUCCESS)
    {
       globus_libc_free(*gatekeeper_url);
       globus_libc_free(duplicate);
       return(GLOBUS_GRAM_PROTOCOL_ERROR_BAD_GATEKEEPER_CONTACT);
    }
    globus_url_destroy(&some_struct);

    if ((dn) && (*dn))
    {
   	*gatekeeper_dn = globus_libc_strdup(dn);
    }
    else
    {
   	*gatekeeper_dn = NULL;
    }
    globus_libc_free(duplicate);

    return GLOBUS_SUCCESS;
}


/******************************************************************************
Function:	globus_l_gram_client_setup_attr_t()
Description:
Parameters:
Returns:
******************************************************************************/
static int 
globus_l_gram_client_setup_attr_t(
    globus_io_attr_t *                     attrp,
    globus_io_secure_delegation_mode_t     delegation_mode,
    char *                                 gatekeeper_dn )
{
    globus_result_t                        res;
    globus_io_secure_authorization_data_t  auth_data;

    if ( (res = globus_io_tcpattr_init(attrp))
	 || (res = globus_io_secure_authorization_data_initialize(
	     &auth_data))
	 || (res = globus_io_attr_set_secure_authentication_mode(
	     attrp,
	     GLOBUS_IO_SECURE_AUTHENTICATION_MODE_MUTUAL,
	     globus_i_gram_protocol_credential))
	 ||  (gatekeeper_dn ? (res = globus_io_secure_authorization_data_set_identity(
	     &auth_data,
	     gatekeeper_dn)) : 0)
	 || (res = globus_io_attr_set_secure_authorization_mode(
	     attrp,
	     gatekeeper_dn ? GLOBUS_IO_SECURE_AUTHORIZATION_MODE_IDENTITY : GLOBUS_IO_SECURE_AUTHORIZATION_MODE_HOST,
	     &auth_data))
	 || (res = globus_io_attr_set_secure_delegation_mode(
	     attrp,
	     delegation_mode))
	 || (res = globus_io_attr_set_secure_channel_mode(
	     attrp,
	     GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP)) )
    {
	globus_object_t *  err = globus_error_get(res);
	
	if(globus_l_print_fp)
	{
	    globus_libc_fprintf(globus_l_print_fp, 
				"setting up IO attributes failed\n");
	}
	
	/* TODO: interrogate 'err' to choose the correct error code */
	
	globus_object_free(err);
	return GLOBUS_GRAM_PROTOCOL_ERROR_PROTOCOL_FAILED;
    }

    return GLOBUS_SUCCESS;
} /* globus_l_gram_client_setup_attr_t() */


/******************************************************************************
Function:	globus_gram_client_version()
Description:
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_version(void)
{
    return(GLOBUS_GRAM_PROTOCOL_VERSION);

} /* globus_gram_client_version() */


/******************************************************************************
Function:	globus_gram_client_ping()
Description:
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_ping(
    char *				gatekeeper_contact)
{
    int					rc;
    globus_io_attr_t			attr;
    globus_l_gram_client_monitor_t	monitor;
    char *				url;
    char *				dn;

    globus_mutex_init(&monitor.mutex, (globus_mutexattr_t *) NULL);
    globus_cond_init(&monitor.cond, (globus_condattr_t *) NULL);
    monitor.done = GLOBUS_FALSE;

    rc = globus_l_gram_client_parse_gatekeeper_contact(
	gatekeeper_contact,
	"ping",
	&url,
	&dn );

    if (rc != GLOBUS_SUCCESS)
	goto globus_gram_client_ping_parse_failed;

    rc = globus_l_gram_client_setup_attr_t( 
	&attr,
	GLOBUS_IO_SECURE_DELEGATION_MODE_NONE,
	dn );
    if (rc != GLOBUS_SUCCESS)
	goto globus_gram_client_ping_attr_failed;

    globus_mutex_lock(&monitor.mutex);
    monitor.type = GLOBUS_GRAM_CLIENT_PING;

    rc = globus_gram_protocol_post(
	         url,
		 &monitor.handle,
		 &attr,
		 GLOBUS_NULL,
		 0,
		 &globus_l_gram_client_monitor_callback,
		 &monitor);

    if (rc != GLOBUS_SUCCESS)
    {
	globus_mutex_unlock(&monitor.mutex);
	goto globus_gram_client_ping_post_failed;
    }
    while (!monitor.done)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }
    rc = monitor.errorcode;

    globus_mutex_unlock(&monitor.mutex);

globus_gram_client_ping_post_failed:
    globus_io_tcpattr_destroy (&attr);

globus_gram_client_ping_attr_failed:
    globus_libc_free(url);
    if (dn)
        globus_libc_free(dn);

globus_gram_client_ping_parse_failed:
    globus_mutex_destroy(&monitor.mutex);
    globus_cond_destroy(&monitor.cond);
    globus_io_tcpattr_destroy (&attr);
    return rc;
} /* globus_gram_client_ping() */


int
globus_gram_client_register_job_request(
    const char *			resource_manager_contact,
    const char *			description,
    int					job_state_mask,
    const char *			callback_contact,
    globus_gram_client_callback_func_t	register_callback,
    void *				register_callback_arg)
{
    globus_l_gram_client_monitor_t *	monitor;
    int					rc;

    monitor = globus_libc_malloc(sizeof(globus_l_gram_client_monitor_t));
    if(!monitor)
    {
	return GLOBUS_GRAM_PROTOCOL_ERROR_MALLOC_FAILED;
    }

    globus_l_gram_client_monitor_init(monitor);
    rc = globus_l_gram_client_job_request(resource_manager_contact,
	                                  description,
				          job_state_mask,
				          callback_contact,
				          monitor,
				          register_callback,
				          register_callback_arg);
    return rc;
}
/* globus_gram_client_register_job_request() */

/******************************************************************************
Function:	globus_gram_client_job_request()
Description:
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_request(
    char *				gatekeeper_contact,
    const char *			description,
    const int				job_state_mask,
    const char *			callback_url,
    char **				job_contact)
{
    int					rc;
    globus_l_gram_client_monitor_t	monitor;

    if(job_contact)
    {
	*job_contact = GLOBUS_NULL;
    }

    globus_l_gram_client_monitor_init(&monitor);

    rc = globus_l_gram_client_job_request(gatekeeper_contact,
	                                  description,
					  job_state_mask,
					  callback_url,
					  &monitor,
					  GLOBUS_NULL,
					  GLOBUS_NULL);
    if(rc != GLOBUS_SUCCESS)
    {
	globus_l_gram_client_monitor_destroy(&monitor);

	return rc;
    }

    globus_mutex_lock(&monitor.mutex);
    while (!monitor.done)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }
    rc = monitor.errorcode;
    if(job_contact)
    {
	*job_contact = monitor.contact;
    }
    globus_mutex_unlock(&monitor.mutex);

    globus_l_gram_client_monitor_destroy(&monitor);

    return rc;
}
/* globus_gram_client_job_request() */


/******************************************************************************
Function:	globus_gram_client_job_check()
Description:
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_check(char * gatekeeper_url,
               const char * description,
               float required_confidence,
               globus_gram_client_time_t * estimate,
               globus_gram_client_time_t * interval_size)
{
    return(0);
} /* globus_gram_client_job_check() */


const char *
globus_gram_client_error_string(int error_code)
{
    return globus_gram_protocol_error_string(error_code);
}

/******************************************************************************
Function:	globus_l_gram_client_to_jobmanager()
Description:	packing/sending to jobmanager URL/waiting/unpacking 
Parameters:
Returns:
******************************************************************************/
int
globus_l_gram_client_to_jobmanager(
    const char *			job_contact,
    const char *			request,
    globus_l_gram_client_callback_type_t
    					request_type,
    int *				job_status,
    int *				failure_code )
{
    int					rc;
    int					job_failure_code;
    globus_byte_t *			query = GLOBUS_NULL; 
    globus_size_t			querysize;
    globus_l_gram_client_monitor_t	monitor;

    globus_mutex_init(&monitor.mutex, (globus_mutexattr_t *) NULL);
    globus_cond_init(&monitor.cond, (globus_condattr_t *) NULL);
    monitor.done = GLOBUS_FALSE;

    rc = globus_gram_protocol_pack_status_request(
	      request,
	      &query,
	      &querysize);

    if (rc!=GLOBUS_SUCCESS)
	goto globus_l_gram_client_to_jobmanager_pack_failed;
    
    globus_mutex_lock(&monitor.mutex);
    monitor.type = request_type;

    rc = globus_gram_protocol_post(
	         job_contact,
		 &monitor.handle,
		 GLOBUS_NULL,
		 query,
		 querysize,
		 globus_l_gram_client_monitor_callback,
		 &monitor);

    if (rc!=GLOBUS_SUCCESS)
    {
	globus_mutex_unlock(&monitor.mutex);
	goto globus_l_gram_client_to_jobmanager_http_failed;
    }

    while (!monitor.done)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }
    rc = monitor.errorcode;
    globus_mutex_unlock(&monitor.mutex);

    if (rc == GLOBUS_SUCCESS)
    {
	rc = monitor.status;
	if ( failure_code )
	{
	    *failure_code = monitor.job_failure_code;
	}
    }

globus_l_gram_client_to_jobmanager_http_failed:
    if (rc != GLOBUS_SUCCESS)
    {
        if (rc == GLOBUS_GRAM_PROTOCOL_ERROR_CONNECTION_FAILED)
        {
            rc = GLOBUS_GRAM_PROTOCOL_ERROR_CONTACTING_JOB_MANAGER;
            *failure_code = GLOBUS_GRAM_PROTOCOL_ERROR_CONTACTING_JOB_MANAGER;
        }
        else
        {
            *failure_code = rc;
        }
    }
    else
    {
	if (*failure_code != GLOBUS_SUCCESS)
	{
	    rc = *failure_code;
	}
	if (job_failure_code != 0)
	{
	    *failure_code = job_failure_code;
	}
    }

    globus_libc_free(query);
    
globus_l_gram_client_to_jobmanager_pack_failed:
    globus_mutex_destroy(&monitor.mutex);
    globus_cond_destroy(&monitor.cond);
    
    return rc;
}
/* globus_l_gram_client_to_jobmanager() */




/******************************************************************************
Function:	globus_gram_client_job_cancel()
Description:	sending cancel request to job manager
Parameters:
Returns:
******************************************************************************/
int
globus_gram_client_job_cancel(char * job_contact)
{
    int                           rc;
    int                           job_state;
    int                           failure_code;
    char *                        request = "cancel";

    GLOBUS_L_CHECK_IF_INITIALIZED;

    rc = globus_l_gram_client_to_jobmanager( job_contact,
					     request,
					     GLOBUS_GRAM_CLIENT_CANCEL,
					     &job_state,
					     &failure_code );

    return rc;
}


/******************************************************************************
Function:	globus_gram_client_job_signal()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_signal(char * job_contact,
                              globus_gram_protocol_job_signal_t signal,
                              char * signal_arg,
			      int  * job_status,
                              int * failure_code)
{
    int       rc;
    char  *   request;

    GLOBUS_L_CHECK_IF_INITIALIZED;

    if (signal_arg != NULL)
    {
	/* 'signal' = 6, allow 10-digit integer, 2 spaces and null  */
	request = (char *) globus_libc_malloc( strlen(signal_arg)
					       + 6 + 10 + 2 + 1 );

	globus_libc_sprintf(request,
			    "signal %d %s",
			    signal,
			    signal_arg);
    }
    else
    {
	/* 'signal' = 6, allow 10-digit integer, 1 space and null  */
	request = (char *) globus_libc_malloc( 6 + 10 + 1 + 1 );

	globus_libc_sprintf(request,
			    "signal %d",
			    signal);
    }

    rc = globus_l_gram_client_to_jobmanager( job_contact,
					     request,
					     GLOBUS_GRAM_CLIENT_SIGNAL,
					     job_status,
					     failure_code );

    globus_libc_free(request);

    return rc;
}


/******************************************************************************
Function:       globus_gram_client_job_status()
Description:    sending cancel request to job manager
Parameters:
Returns:
******************************************************************************/
int
globus_gram_client_job_status(char * job_contact,
			      int  * job_status,
			      int  * failure_code)
{
    int       rc;
    char *    request = "status";

    GLOBUS_L_CHECK_IF_INITIALIZED;

    rc = globus_l_gram_client_to_jobmanager( job_contact,
					     request,
					     GLOBUS_GRAM_CLIENT_STATUS,
					     job_status,
					     failure_code );

    return rc;
}



/******************************************************************************
Function:	globus_gram_client_job_callback_register()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_callback_register(char * job_contact,
					 const int job_state_mask,
					 const char * callback_contact,
					 int * job_status,
					 int * failure_code)
{
    int       rc;
    char  *   request;

    GLOBUS_L_CHECK_IF_INITIALIZED;

    /* 'register' = 8, allow 10-digit integer, 2 spaces and null  */
    request = (char *) globus_libc_malloc( 
	                  strlen(callback_contact)
			  + 8 + 10 + 2 + 1 );

    globus_libc_sprintf(request,
			"register %d %s",
			job_state_mask,
			callback_contact);

    rc = globus_l_gram_client_to_jobmanager( job_contact,
					     request,
					     GLOBUS_GRAM_CLIENT_CALLBACK_REGISTER,
					     job_status,
					     failure_code );

    globus_libc_free(request);

    return rc;
}


/******************************************************************************
Function:	globus_gram_client_job_callback_unregister()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_callback_unregister(char *         job_contact,
					   const char *   callback_contact,
					   int *          job_status,
					   int *          failure_code)
{
    int       rc;
    char  *   request;

    GLOBUS_L_CHECK_IF_INITIALIZED;

    /* 'unregister' = 10, a space and null  */
    request = (char *) globus_libc_malloc( 
	                  strlen(callback_contact)
			  + 10 + 1 + 1 );

    globus_libc_sprintf(request,
			"unregister %s",
			callback_contact);

    rc = globus_l_gram_client_to_jobmanager(
	    job_contact,
	    request,
	    GLOBUS_GRAM_CLIENT_CALLBACK_UNREGISTER,
	    job_status,
	    failure_code );

    globus_libc_free(request);

    return rc;
}

/******************************************************************************
Function:	globus_gram_client_callback_allow()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_callback_allow(
    globus_gram_client_callback_func_t callback_func,
    void * user_callback_arg,
    char ** callback_contact)
{
    int					rc;
    globus_l_gram_client_callback_info_t *
					callback_info;

    GLOBUS_L_CHECK_IF_INITIALIZED;

    callback_info = globus_libc_malloc(
	                sizeof(globus_l_gram_client_callback_info_t));

    callback_info->callback = callback_func;
    callback_info->callback_arg = user_callback_arg;

    rc = globus_gram_protocol_allow_attach(
	    &callback_info->callback_contact,
	    globus_l_gram_client_callback,
	    callback_info);

    globus_mutex_lock(&globus_l_mutex);
    globus_hashtable_insert(&globus_l_gram_client_contacts,
	                    callback_info->callback_contact,
	                    callback_info);
    globus_mutex_unlock(&globus_l_mutex);

    if (rc==GLOBUS_SUCCESS && callback_contact)
    {
	*callback_contact = globus_libc_strdup(callback_info->callback_contact);
    }
    return rc;
} /* globus_gram_client_callback_allow() */


/******************************************************************************
Function:	globus_gram_client_callback_disallow()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_callback_disallow(char * callback_contact)
{
    int					rc;
    globus_l_gram_client_callback_info_t *
					callback_info;

    globus_mutex_lock(&globus_l_mutex);

    callback_info = globus_hashtable_remove(
	    &globus_l_gram_client_contacts,
	    callback_contact);

    globus_mutex_unlock(&globus_l_mutex);

    if(callback_info != GLOBUS_NULL)
    {
	rc = globus_gram_protocol_callback_disallow(callback_contact);

	globus_libc_free(callback_info->callback_contact);
	globus_libc_free(callback_info);
    }
    else
    {
	rc = GLOBUS_GRAM_PROTOCOL_ERROR_CALLBACK_NOT_FOUND;
    }

    return rc;
} /* globus_gram_client_callback_allow() */


/******************************************************************************
Function:	globus_gram_client_job_start_time()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_start_time(char * job_contact,
                    float required_confidence,
                    globus_gram_client_time_t * estimate,
                    globus_gram_client_time_t * interval_size)
{
    if(globus_l_print_fp)
    {
	globus_libc_fprintf(globus_l_print_fp,
			    "in globus_gram_client_job_start_time()\n");
    }

    return GLOBUS_SUCCESS;
} /* globus_gram_client_job_start_time() */



/******************************************************************************
Function:	globus_gram_client_job_contact_free()
Description:	
Parameters:
Returns:
******************************************************************************/
int 
globus_gram_client_job_contact_free(char * job_contact)
{
    if(globus_l_print_fp)
    {
	globus_libc_fprintf(globus_l_print_fp,
		      "in globus_gram_client_job_contact_free()\n");
    }

    globus_free(job_contact);

    return (0);
} /* globus_gram_client_job_contact_free() */

static
int
globus_l_gram_client_job_request(
    const char *			resource_manager_contact,
    const char *			description,
    int					job_state_mask,
    const char *			callback_contact,
    globus_l_gram_client_monitor_t *	monitor,
    globus_gram_client_callback_func_t	register_callback,
    void *				register_callback_arg)
{
    int					rc;
    globus_byte_t *			query = GLOBUS_NULL;
    globus_size_t			querysize; 
    globus_io_attr_t			attr;
    char *				url;
    char *				dn;

    monitor->callback = register_callback;
    monitor->callback_arg = register_callback_arg;

    if ((rc = globus_l_gram_client_parse_gatekeeper_contact(
	             resource_manager_contact,
		     GLOBUS_NULL,
		     &url,
		     &dn )) != GLOBUS_SUCCESS)
    {
	goto globus_gram_client_job_request_parse_failed;
    }

    if ((rc = globus_l_gram_client_setup_attr_t( 
	             &attr,
		     GLOBUS_IO_SECURE_DELEGATION_MODE_LIMITED_PROXY,
		     dn )) 

	|| (rc = globus_gram_protocol_pack_job_request(
	             job_state_mask,
		     callback_contact,
		     description,
		     &query,
		     &querysize)) )
    {
	goto globus_gram_client_job_request_pack_failed;
    }

    globus_mutex_lock(&monitor->mutex);
    monitor->type = GLOBUS_GRAM_CLIENT_JOB_REQUEST;
    rc = globus_gram_protocol_post(
	         url,
		 &monitor->handle,
		 &attr,
		 query,
		 querysize,
		 globus_l_gram_client_monitor_callback,
		 monitor);
    globus_mutex_unlock(&monitor->mutex);

    if (query)
	globus_libc_free(query);

    if(rc == GLOBUS_SUCCESS)
    {
	return rc;
    }

globus_gram_client_job_request_pack_failed:
    globus_io_tcpattr_destroy (&attr);
    globus_libc_free(url);
    if (dn)
        globus_libc_free(dn);

globus_gram_client_job_request_parse_failed:
    return rc;
}
/* globus_l_gram_client_job_request() */

static
void
globus_l_gram_client_callback(
    void *				arg,
    globus_gram_protocol_handle_t	handle,
    globus_byte_t *			buf,
    globus_size_t			nbytes,
    int					errorcode)
{
    globus_l_gram_client_callback_info_t *
					info;
    globus_gram_client_callback_func_t	userfunc;
    globus_byte_t *			reply;
    globus_size_t			replysize;
    char *				url;
    int					job_status;
    int					failure_code;
    int					rc;

    info = arg;

    rc = errorcode;

    if (rc != GLOBUS_SUCCESS || nbytes <= 0)
    {
        job_status   = GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED;
        failure_code = rc;
    }
    else
    {
        rc = globus_gram_protocol_unpack_status_update_message(
            buf,
            nbytes,
            &url,
            &job_status,
            &failure_code);
    }

    rc = globus_gram_protocol_reply(handle,
	                            200,
				    GLOBUS_NULL,
				    0);
    
    info->callback(info->callback_arg,
	           url,
		   job_status,
		   failure_code);

    globus_libc_free(url);
}

static
void
globus_l_gram_client_monitor_callback(
    void *				user_arg,
    globus_gram_protocol_handle_t	handle,
    globus_byte_t *			message,
    globus_size_t			msgsize,
    int					errorcode)
{
    globus_l_gram_client_monitor_t *	monitor;
    int					rc;

    monitor = user_arg;

    globus_mutex_lock(&monitor->mutex);

    monitor->errorcode = errorcode;
    monitor->done = GLOBUS_TRUE;

    switch(monitor->type)
    {
      case GLOBUS_GRAM_CLIENT_JOB_REQUEST:
	rc = globus_gram_protocol_unpack_job_request_reply(
		message,
		msgsize,
		&monitor->status,
		&monitor->contact);
	if(rc != GLOBUS_SUCCESS)
	{
	    monitor->errorcode = rc;
	}
	break;

      case GLOBUS_GRAM_CLIENT_PING:
	break;
      case GLOBUS_GRAM_CLIENT_STATUS:
      case GLOBUS_GRAM_CLIENT_SIGNAL:
      case GLOBUS_GRAM_CLIENT_CANCEL:
      case GLOBUS_GRAM_CLIENT_CALLBACK_REGISTER:
      case GLOBUS_GRAM_CLIENT_CALLBACK_UNREGISTER:
	rc = globus_gram_protocol_unpack_status_reply(
		message,
		msgsize,
		&monitor->status,
		&monitor->errorcode,
		&monitor->job_failure_code);
	if(rc != GLOBUS_SUCCESS)
	{
	    monitor->errorcode = rc;
	}
	break;
    }
    globus_cond_signal(&monitor->cond);
    globus_mutex_unlock(&monitor->mutex);
}
/* globus_l_gram_client_monitor_callback() */

static
void
globus_l_gram_client_register_job_request_callback(
    void *				user_arg,
    globus_gram_protocol_handle_t	handle,
    globus_byte_t *			message,
    globus_size_t			msgsize,
    int					errorcode)
{
    globus_l_gram_client_monitor_t *	monitor;
    int					rc;

    monitor = user_arg;

    globus_mutex_lock(&monitor->mutex);

    monitor->errorcode = errorcode;
    monitor->done = GLOBUS_TRUE;

    rc = globus_gram_protocol_unpack_job_request_reply(
	    message,
	    msgsize,
	    &monitor->status,
	    &monitor->contact);
    if(rc != GLOBUS_SUCCESS)
    {
	monitor->errorcode = rc;
    }
    globus_mutex_unlock(&monitor);

    monitor->callback(monitor->callback_arg,
	              monitor->contact,
		      monitor->status,
		      monitor->errorcode);
    monitor->contact = GLOBUS_NULL;

    globus_l_gram_client_monitor_destroy(monitor);
    globus_libc_free(monitor);
}
/* globus_l_gram_client_register_job_request_callback() */

static
int
globus_l_gram_client_monitor_init(
    globus_l_gram_client_monitor_t *	monitor)
{
    memset(monitor, '\0', sizeof(globus_l_gram_client_monitor_t));

    globus_mutex_init(&monitor->mutex, GLOBUS_NULL);
    globus_cond_init(&monitor->cond, GLOBUS_NULL);
    monitor->done = GLOBUS_FALSE;

    return GLOBUS_SUCCESS;
}
/* globus_l_gram_client_monitor_init() */

static
int
globus_l_gram_client_monitor_destroy(
    globus_l_gram_client_monitor_t *	monitor)
{
    globus_mutex_destroy(&monitor->mutex);
    globus_cond_destroy(&monitor->cond);

    return GLOBUS_SUCCESS;
}
/* globus_l_gram_client_monitor_destroy() */
