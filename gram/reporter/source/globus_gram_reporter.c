/******************************************************************************
globus_gram_scheduler.c

Description:
    Globus GRAM scheduler wrapper program

CVS Information:
    $Source$
    $Date$
    $Revision$
    $Author$
******************************************************************************/

/******************************************************************************
                             Include header files
******************************************************************************/

#include "globus_common.h"
#include <stdio.h>
#include "globus_rsl.h"
#include "globus_gss_assist.h"
#include "globus_gram_scheduler.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "strings.h"
#include "ctype.h"
#include "fcntl.h"
#include "time.h"

/******************************************************************************
                       Define module specific variables
******************************************************************************/

typedef struct globus_l_gram_conf_values_s
{
    char *         curr_gmt_time;
    char *         type;
    char *         condor_arch;
    char *         condor_os;
    char *         home_dir;
    char *         rdn;
    char *         dmdn;
    char *         host_dn;
    char *         org_dn;
    char *         gate_host;
    char *         gate_port;
    char *         gate_subject;
    char *         conf_file;
    char *         cldif_file;
    FILE *         cldif_fp;
    char *         osname;
    char *         osversion;
    char *         cputype;
    char *         manufacturer;
    char *         machine_type;
    globus_bool_t  publish_jobs;
} globus_l_gram_conf_values_t;

static int verbose=0;

#define GRAM_JOB_MANAGER_STATUS_FILE_SECONDS 600

/******************************************************************************
                          Module specific prototypes
******************************************************************************/

static void
print_usage();

static int
globus_l_gram_show_conf_data(globus_l_gram_conf_values_t * vals);

static int
globus_l_gram_write_gram_cldif_file(globus_l_gram_conf_values_t * vals,
                                    globus_list_t * q_list);

static void
globus_l_gram_check_globus_jobs(globus_l_gram_conf_values_t * vals,
                                globus_list_t * q_list);

static void
globus_l_gram_add_to_job_entries( globus_l_gram_conf_values_t * vals,
                                  globus_list_t * q_list,
                                  char * job_status_filename,
                                  char * job_status_filepath );

static char *
globus_l_gram_parse_job_status_filename(char * job_status_filename,
                                        char * rdn);

static int
globus_l_gram_get_conf_values(globus_l_gram_conf_values_t * vals);

static int
globus_l_gram_conf_values_init(globus_l_gram_conf_values_t * vals);

static void
globus_l_gram_conf_values_free(globus_l_gram_conf_values_t * vals);

static int
globus_l_gram_gridmap_file_has_changed(globus_l_gram_conf_values_t * vals,
                                         char * lastupdate_map_file);

/******************************************************************************
Function:       print_usage()
Description:
Parameters:
Returns:
******************************************************************************/
static void
print_usage()
{
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s %s %s %s %s %s %s %s\n",
            "globus-gram-scheduler",
            "[-condor-arch archetecture] [-condor-os operating system]",
            "[-conf job manager configuration file]",
            "[-cldif cldif file to append information]",
            "[-rdn service part of the gram contact]",
            "[-dmdn directory manager DN]",
            "[-type the type of the gram]",
            "[-dont-publish-jobs] [-verbose]"
           );
} /* print_usage() */


/******************************************************************************
Function:       globus_l_gram_show_conf_data()
Description:
Parameters:
Returns:
******************************************************************************/
static int
globus_l_gram_show_conf_data(globus_l_gram_conf_values_t * vals)
{
    if (vals->curr_gmt_time)
        printf("curr gmt time= %s\n", vals->curr_gmt_time);
    else
        printf("curr gmt time= NULL\n");
       
    if (vals->type)
        printf("type = %s\n", vals->type);
    else
        printf("type = NULL\n");
       
    if (vals->condor_arch)
        printf("condor_arch = %s\n", vals->condor_arch);
    else
        printf("condor_arch = NULL\n");
       
    if (vals->condor_os)
        printf("condor_os = %s\n", vals->condor_os);
    else
        printf("condor_os = NULL\n");
       
    if (vals->home_dir)
        printf("home_dir = %s\n", vals->home_dir);
    else
        printf("home_dir = NULL\n");
       
    if (vals->dmdn)
        printf("dmdn = %s\n", vals->dmdn);
    else
        printf("dmdn = NULL\n");
       
    if (vals->rdn)
        printf("rdn = %s\n", vals->rdn);
    else
        printf("rdn = NULL\n");
       
    if (vals->host_dn)
        printf("host_dn = %s\n", vals->host_dn);
    else
        printf("host_dn = NULL\n");
       
    if (vals->org_dn)
        printf("org_dn = %s\n", vals->org_dn);
    else
        printf("org_dn = NULL\n");
       
    if (vals->gate_host)
        printf("gate_host = %s\n", vals->gate_host);
    else
        printf("gate_host = NULL\n");
       
    if (vals->gate_port)
        printf("gate_port = %s\n", vals->gate_port);
    else
        printf("gate_port = NULL\n");
       
    if (vals->gate_subject)
        printf("gate_subject = %s\n", vals->gate_subject);
    else
        printf("gate_subject = NULL\n");
       
    if (vals->cputype)
        printf("cputype = %s\n", vals->cputype);
    else
        printf("cputype = NULL\n");
       
    if (vals->osname)
        printf("osname = %s\n", vals->osname);
    else
        printf("osname = NULL\n");
       
    if (vals->osversion)
        printf("osversion = %s\n", vals->osversion);
    else
        printf("osversion = NULL\n");
       
    if (vals->manufacturer)
        printf("manufacturer = %s\n", vals->manufacturer);
    else
        printf("manufacturer = NULL\n");
       
    if (vals->conf_file)
        printf("conf_file = %s\n", vals->conf_file);
    else
        printf("conf_file = NULL\n");
       
    if (vals->cldif_file)
        printf("cldif_file = %s\n", vals->cldif_file);
    else
        printf("cldif_file = NULL\n");
       
    if (vals->machine_type)
        printf("machinetype = %s\n", vals->machine_type);
    else
        printf("machinetype = NULL\n");
       
    printf("publish jobs = %d\n", vals->publish_jobs);

    return(GLOBUS_SUCCESS);

} /* globus_l_gram_show_conf_data() */



/******************************************************************************
Function:       globus_l_gram_write_gram_cldif_file()
Description:
Parameters:
Returns:
******************************************************************************/
static int
globus_l_gram_write_gram_cldif_file(globus_l_gram_conf_values_t * vals,
                                    globus_list_t * q_list)
{
    globus_list_t * q_entry_list = GLOBUS_NULL;
    globus_gram_scheduler_t *  q_node;
    globus_gram_scheduler_entry_t *  q_entry_node;

    if (q_list == GLOBUS_NULL)
        return(1);

    /* begin: write the resource manager object */    
    fprintf(vals->cldif_fp, "dn: service=%s, %s\n",
                vals->rdn,
                vals->host_dn);
    fprintf(vals->cldif_fp, "objectclass: GlobusTop\n");
    fprintf(vals->cldif_fp, "objectclass: GlobusDaemon\n");
    fprintf(vals->cldif_fp, "objectclass: GlobusService\n");
    fprintf(vals->cldif_fp, "objectclass: GlobusServiceJobManager\n");
    fprintf(vals->cldif_fp, "objectName: service=%s, %s\n",
                vals->rdn,
                vals->host_dn);
    fprintf(vals->cldif_fp, "service: %s\n", vals->rdn);
    fprintf(vals->cldif_fp, "hn: %s\n", vals->gate_host);
    fprintf(vals->cldif_fp, "contact: %s:%s/%s:%s\n",
                vals->gate_host,
                vals->gate_port,
                vals->rdn,
                vals->gate_subject);
    fprintf(vals->cldif_fp, "schedulerVersion: 0.1\n");
    fprintf(vals->cldif_fp, "schedulertype: %s\n", vals->type);
    fprintf(vals->cldif_fp, "deploydir: %s\n", vals->home_dir);
    fprintf(vals->cldif_fp, "cputype: %s\n", vals->cputype);
    fprintf(vals->cldif_fp, "osversion: %s\n", vals->osversion);
    fprintf(vals->cldif_fp, "osname: %s\n", vals->osname);
    fprintf(vals->cldif_fp, "manufacturer: %s\n", vals->manufacturer);
    fprintf(vals->cldif_fp, "machinetype: %s\n", vals->machine_type);
    /* fprintf(vals->cldif_fp, "gramVersion: %s\n", GRAM_VERSION); */
    /* fprintf(vals->cldif_fp, "gramVersionDate: %s\n", GRAM_VERSION_DATE); */
    /* fprintf(vals->cldif_fp, "gramSecurity: %s\n", GRAM_SECURITY); */
    fprintf(vals->cldif_fp, "lastupdate: %s\n", vals->curr_gmt_time);
    fprintf(vals->cldif_fp, "ttl: 30\n");
    fprintf(vals->cldif_fp, "\n");
    /* end: write the resource manager object */    

    while (! globus_list_empty(q_list))
    {
        q_node = (globus_gram_scheduler_t *) globus_list_first
            (q_list);

        q_list = globus_list_rest(q_list);

        if ( ! q_node->queuename)
           q_node->queuename = (char *) globus_libc_strdup("default");

        fprintf(vals->cldif_fp, "dn: queue=%s, service=%s, %s\n", 
                    q_node->queuename,
                    vals->rdn,
                    vals->host_dn);
        fprintf(vals->cldif_fp, "objectclass: GlobusTop\n");
        fprintf(vals->cldif_fp, "objectclass: GlobusQueue\n");
        fprintf(vals->cldif_fp, "queue: %s\n", q_node->queuename);
        fprintf(vals->cldif_fp, "maxtime: %d\n", q_node->maxtime);
        fprintf(vals->cldif_fp, "maxcputime: %d\n", q_node->maxcputime);
        fprintf(vals->cldif_fp, "maxcount: %d\n", q_node->maxcount);
        fprintf(vals->cldif_fp, "maxrunningjobs: %d\n", q_node->maxrunningjobs);
        fprintf(vals->cldif_fp, "maxjobsinqueue: %d\n", q_node->maxjobsinqueue);
        fprintf(vals->cldif_fp, "maxtotalmemory: %d\n", q_node->maxtotalmemory);
        fprintf(vals->cldif_fp, "maxsinglememory: %d\n",
                                                       q_node->maxsinglememory);
        fprintf(vals->cldif_fp, "totalnodes: %d\n", q_node->totalnodes);
        fprintf(vals->cldif_fp, "freenodes: %d\n", q_node->freenodes);
        if (q_node->whenactive)
            fprintf(vals->cldif_fp, "whenactive: %s\n", q_node->whenactive);
        else
            fprintf(vals->cldif_fp, "whenactive: NULL\n");

        if (q_node->status)
            fprintf(vals->cldif_fp, "status: %s\n", q_node->status);
        else
            fprintf(vals->cldif_fp, "status: NULL\n");

        if (q_node->dispatchtype)
            fprintf(vals->cldif_fp, "dispatchtype: %s\n", q_node->dispatchtype);
        else
            fprintf(vals->cldif_fp, "dispatchtype: NULL\n");

        if (q_node->priority)
            fprintf(vals->cldif_fp, "priority: %s\n", q_node->priority);
        else
            fprintf(vals->cldif_fp, "priority: NULL\n");

        if (q_node->jobwait)
            fprintf(vals->cldif_fp, "jobwait: %s\n", q_node->jobwait);
        else
            fprintf(vals->cldif_fp, "jobwait: NULL\n");

        if (q_node->schedulerspecific)
            fprintf(vals->cldif_fp, "schedulerspecific: %s\n",
                    q_node->schedulerspecific);
        else
            fprintf(vals->cldif_fp, "schedulerspecific: NULL\n");

        /* if we are not publishing the job entries then skip the rest!! */
        if (!vals->publish_jobs)
        {
            fprintf(vals->cldif_fp, "ttl: 1\n");
            fprintf(vals->cldif_fp, "lastupdate: %s\n", vals->curr_gmt_time);
            fprintf(vals->cldif_fp, "\n");
            continue;
        }


        fprintf(vals->cldif_fp, "ttl: 1\n");
        fprintf(vals->cldif_fp, "lastupdate: %s\n", vals->curr_gmt_time);
        fprintf(vals->cldif_fp, "\n");

        q_entry_list = q_node->entry_list;

        /* begin: write the queue entry object(s) */    
        while (! globus_list_empty(q_entry_list))
        {
            q_entry_node = (globus_gram_scheduler_entry_t *) globus_list_first
                (q_entry_list);
            q_entry_list = globus_list_rest(q_entry_list);

            if (!q_entry_node->local_job_id)
               continue;

            fprintf(vals->cldif_fp, "dn: jobid=%s, queue=%s, service=%s, %s\n",
                         q_entry_node->local_job_id,
                         q_node->queuename,
                         vals->rdn,
                         vals->host_dn);
            fprintf(vals->cldif_fp, "jobid: %s\n", q_entry_node->local_job_id);
            fprintf(vals->cldif_fp, "objectclass: GlobusTop\n");
            fprintf(vals->cldif_fp, "objectclass: GlobusQueueEntry\n");

            fprintf(vals->cldif_fp, "localjobid: %s\n",
                q_entry_node->local_job_id);

            if (q_entry_node->local_user_name)
                fprintf(vals->cldif_fp, "localusername: %s\n",
                    q_entry_node->local_user_name);
            else
                fprintf(vals->cldif_fp, "localusername: NULL\n");

            if (q_entry_node->global_user_name)
                fprintf(vals->cldif_fp, "globalusername: %s\n",
                    q_entry_node->global_user_name);
            else
                fprintf(vals->cldif_fp, "globalusername: NULL\n");
        
            if (q_entry_node->specification)
                fprintf(vals->cldif_fp, "specification: %s\n",
                    q_entry_node->specification);
            else
                fprintf(vals->cldif_fp, "specification: NULL\n");

            if (q_entry_node->global_job_id)
                fprintf(vals->cldif_fp, "globaljobid: %s\n",
                    q_entry_node->global_job_id);
            else
                fprintf(vals->cldif_fp, "globaljobid: NULL\n");

            fprintf(vals->cldif_fp, "count: %d\n", q_entry_node->count);

            if (q_entry_node->status)
                fprintf(vals->cldif_fp, "status: %s\n", q_entry_node->status);
            else
                fprintf(vals->cldif_fp, "status: NULL\n");

            fprintf(vals->cldif_fp, "start time: %lu\n",
                    q_entry_node->start_time);

            if (q_entry_node->schedulerspecific)
                fprintf(vals->cldif_fp, "schedspecific: %s\n",
                    q_entry_node->schedulerspecific);
            else
                fprintf(vals->cldif_fp, "schedspecific: NULL\n");

            fprintf(vals->cldif_fp, "ttl: 1\n");
            fprintf(vals->cldif_fp, "lastupdate: %s\n", vals->curr_gmt_time);

            fprintf(vals->cldif_fp, "\n");
        }
        /* end: write the queue entry object(s) */    
    }
    /* end: write the queue object(s) */    

    return(0);

} /* globus_l_gram_write_gram_cldif_file() */


/******************************************************************************
Function:       globus_l_gram_get_globus_values()
Description:
Parameters:
Returns:
******************************************************************************/
static void
globus_l_gram_check_globus_jobs( globus_l_gram_conf_values_t * vals,
                                 globus_list_t * q_list )
{
    globus_gram_scheduler_t *  q_node;
    char stat_file_path[256];
    char jobmanager_status_dir[256];
    DIR * status_dir;
    struct stat statbuf;
    struct dirent * dir_entry;
    char match_str[56];
    unsigned long now;

    if (q_list == GLOBUS_NULL)
       return;

    if (globus_list_empty(q_list))
       return;
    
    if (!vals->publish_jobs)
       return;

    sprintf(jobmanager_status_dir, "%s/var", vals->home_dir);

    status_dir = globus_libc_opendir(jobmanager_status_dir);
    if(status_dir == GLOBUS_NULL)
    {
        fprintf(stderr, 
            "ERROR: unable to open jobmanager status directory.\n");
        fprintf(stderr, 
            "     directory = %s.\n", jobmanager_status_dir);
        return;
    }

    now = (unsigned long) time(NULL);
    sprintf(match_str, "%s_", vals->rdn);

    for(globus_libc_readdir_r(status_dir, &dir_entry);
        dir_entry != GLOBUS_NULL;
        globus_libc_readdir_r(status_dir, &dir_entry))
    {
        if ( strstr(dir_entry->d_name, match_str) )
        {
            sprintf(stat_file_path, "%s/%s", jobmanager_status_dir,
                                      dir_entry->d_name);

            if (stat(stat_file_path, &statbuf) == 0)
            {
                if ( (now - (unsigned long) statbuf.st_mtime) <
                      GRAM_JOB_MANAGER_STATUS_FILE_SECONDS )
                {
                    if (verbose)
                        fprintf( stderr,
                            "processing status file %s\n", stat_file_path);

                    globus_l_gram_add_to_job_entries(vals,
                                                     q_list,
                                                     dir_entry->d_name,
                                                     stat_file_path);
                }
            }
        }
    }

    if (strcasecmp(vals->type, "fork") == 0)
    {
        q_node = (globus_gram_scheduler_t *) globus_list_first (q_list);
        q_node->entry_list = globus_list_copy_reverse(q_node->entry_list);
    }

    globus_libc_closedir(status_dir);

} /* globus_l_gram_check_globus_jobs() */


/******************************************************************************
Function:       globus_l_gram_add_to_job_entries()
Description:
Parameters:
Returns:
******************************************************************************/
static void
globus_l_gram_add_to_job_entries( globus_l_gram_conf_values_t * vals,
                                  globus_list_t * q_list,
                                  char * job_status_filename,
                                  char * job_status_filepath )
{
    globus_list_t * q_entry_list = GLOBUS_NULL;
    globus_list_t * q_list_head = GLOBUS_NULL;
    globus_gram_scheduler_t *  q_node;
    globus_gram_scheduler_entry_t *  q_entry_node;
    char ** tmp_param;
    char * ptr;
    globus_rsl_t * rsl_tree = GLOBUS_NULL;
    char rsl_spec[2048];
    char job_contact[128];
    char job_id[128];
    char job_status[64];
    char globus_id[512];
    FILE * fp;


    if (!vals->publish_jobs)
       return;

    if (q_list == GLOBUS_NULL)
       return;

    if (globus_list_empty(q_list))
       return;
    
    if (job_status_filename == GLOBUS_NULL)
       return;

    if (job_status_filepath == GLOBUS_NULL)
       return;

    /* get info from the job status file */
    if ((fp = fopen(job_status_filepath, "r")) == NULL)
        return;

    *rsl_spec    = '\0';
    *job_contact = '\0';
    *job_id      = '\0';
    *globus_id   = '\0';
    *job_status  = '\0';

    if (fgets(job_status, sizeof(job_status), fp) != NULL)
    {
        /* the job status will be 10 chars long padded with spaces after the
         * value.  There will always be at least 1 space making the following
         * command safe.
         */
        ptr = (char *) strchr(job_status, ' ');
        *ptr='\0';
    }

    if (fgets(rsl_spec, sizeof(rsl_spec), fp) != NULL)
        rsl_spec[strlen(rsl_spec) - 1] = '\0';

    if (fgets(job_contact, sizeof(job_contact), fp) != NULL)
        job_contact[strlen(job_contact) - 1] = '\0';

    if (fgets(job_id, sizeof(job_id), fp) != NULL)
        job_id[strlen(job_id) - 1] = '\0';

    if (fgets(globus_id, sizeof(globus_id), fp) != NULL)
        globus_id[strlen(globus_id) - 1] = '\0';

    fclose(fp);

    if ((strlen(rsl_spec)    == 0) ||
        (strlen(job_contact) == 0) ||
        (strlen(job_id)      == 0) ||
        (strlen(globus_id)   == 0) ||
        (strlen(job_status)  == 0))
    {
        return;
    }

    q_list_head = q_list;

    /* The values have been obtained from the job status file.  Now find the
     * queue entry matching this job id.  If it is found, add to the entry any
     * additional info available.  If it is not found, create a new entry
     * with the information available.
     */
    while (! globus_list_empty(q_list))
    {
        q_node = (globus_gram_scheduler_t *) globus_list_first (q_list);

        q_list = globus_list_rest(q_list);

        q_entry_list = q_node->entry_list;

        while ( ! globus_list_empty(q_entry_list) )
        {
            q_entry_node = (globus_gram_scheduler_entry_t *) 
                globus_list_first (q_entry_list);
            q_entry_list = globus_list_rest(q_entry_list);

            if (strcasecmp(q_entry_node->local_job_id, job_id) == 0)
            {

                if ( ! q_entry_node->local_user_name )
                {
                     q_entry_node->local_user_name =
                          globus_l_gram_parse_job_status_filename(
                                   job_status_filename,
                                   vals->rdn);
                }

                if ( ! q_entry_node->global_user_name )
                {
                    q_entry_node->global_user_name = 
                         (char *) globus_libc_strdup(globus_id);
/*
 *                  globus_gss_assist_map_local_user(
 *                        q_entry_node->local_user_name,
 *                        &q_entry_node->global_user_name);
 */
                }

                if ( ! q_entry_node->specification )
                    q_entry_node->specification =
                         (char *) globus_libc_strdup(rsl_spec);
        
                if ( ! q_entry_node->global_job_id )
                {
                    q_entry_node->global_job_id =
                         (char *) globus_libc_strdup(job_contact);
                }

                return;

            } /* if job_id */
        } /* while job entries */
    } /* while queues */

    /* 
     * if we reach this point then we found a jobmanager status file and there
     * was no matching queue entry.
     *
     * Note:  We are only going to add the entry for fork jobmanagers or
     *        if the add entries flag hsa been set.
     */
    if ((strcasecmp(vals->type, "fork") != 0) && (! q_node->add_entries_flag))
        return;
    
    q_entry_node = (globus_gram_scheduler_entry_t *)
                    globus_libc_malloc(sizeof(globus_gram_scheduler_entry_t));
    globus_i_gram_q_entry_init(q_entry_node);

    q_entry_node->local_job_id     = (char *) globus_libc_strdup(job_id);
    q_entry_node->global_job_id    = (char *) globus_libc_strdup(job_contact);
    q_entry_node->global_user_name = (char *) globus_libc_strdup(globus_id);
    q_entry_node->specification    = (char *) globus_libc_strdup(rsl_spec);
    q_entry_node->status           = (char *) globus_libc_strdup(job_status);
    q_entry_node->local_user_name = 
        globus_l_gram_parse_job_status_filename(job_status_filename,vals->rdn);
 
    if ((rsl_tree = globus_rsl_parse(rsl_spec)) == GLOBUS_NULL)
    {
        q_entry_node->count = 1;
    }
    else
    {
        if (globus_rsl_param_get(rsl_tree,
                                 GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                                 "count",
                                 &tmp_param) == 0)
        {
            if (tmp_param[0])
            {
                q_entry_node->count = atoi ((tmp_param)[0]);
            }
        }

        if ( q_entry_node->count == 0 )
        {
            q_entry_node->count = 1;
        }
    }

    q_node = (globus_gram_scheduler_t *) globus_list_first (q_list_head);

    globus_list_insert(&(q_node->entry_list), (void *) q_entry_node);

    return;

} /* globus_l_gram_add_to_job_entries() */


/******************************************************************************
Function:       globus_l_gram_parse_job_status_filename()
Description:
                Expecting job status file format of <rdn>_<username>.<jobid> 
Parameters:
Returns:
******************************************************************************/
static char *
globus_l_gram_parse_job_status_filename(char * job_status_filename,
                                        char * rdn)
{
    int rdn_len;
    int username_len;
    char * username;
    char * dot_pos;

    rdn_len=strlen(rdn);
    rdn_len++;

    if (strlen(job_status_filename) <= rdn_len ||
        job_status_filename[rdn_len-1] != '_')
    {
        return(GLOBUS_NULL);
    }

    dot_pos = (char *) strchr(&job_status_filename[rdn_len], '.');
    if (dot_pos == NULL)
    {
        return(GLOBUS_NULL);
    }
    username_len = dot_pos - &job_status_filename[rdn_len];
    username = (char *) globus_libc_malloc(sizeof(char *) * username_len + 1);
    strncpy(username, &job_status_filename[rdn_len], username_len);
    username[username_len] = '\0';

    return(username);

} /* globus_l_gram_parse_job_status_filename() */


/******************************************************************************
Function:       globus_l_gram_conf_values_init()
Description:
Parameters:
Returns:
******************************************************************************/
static int
globus_l_gram_conf_values_init(globus_l_gram_conf_values_t * vals)
{
    if (vals == GLOBUS_NULL)
        return(1);

    vals->type = GLOBUS_NULL;
    vals->curr_gmt_time = GLOBUS_NULL;
    vals->home_dir = GLOBUS_NULL;
    vals->host_dn = GLOBUS_NULL;
    vals->org_dn = GLOBUS_NULL;
    vals->gate_host = GLOBUS_NULL;
    vals->gate_port = GLOBUS_NULL;
    vals->gate_subject = GLOBUS_NULL;
    vals->rdn = GLOBUS_NULL;
    vals->dmdn = GLOBUS_NULL;
    vals->cputype = GLOBUS_NULL;
    vals->manufacturer = GLOBUS_NULL;
    vals->osname = GLOBUS_NULL;
    vals->osversion = GLOBUS_NULL;
    vals->condor_arch = GLOBUS_NULL;
    vals->condor_os = GLOBUS_NULL;
    vals->conf_file = GLOBUS_NULL;
    vals->cldif_file = GLOBUS_NULL;
    vals->machine_type = GLOBUS_NULL;
    vals->publish_jobs = GLOBUS_TRUE;
    return(0);
}

/******************************************************************************
Function:       globus_l_gram_conf_values_free()
Description:
Parameters:
Returns:
******************************************************************************/
static void
globus_l_gram_conf_values_free(globus_l_gram_conf_values_t * vals)
{
    if (vals == GLOBUS_NULL)
       return;

    globus_libc_free(vals->curr_gmt_time);
    return;
}

/******************************************************************************
Function:       globus_l_gram_get_conf_values()
Description:
Parameters:
Returns:
******************************************************************************/
static int
globus_l_gram_get_conf_values(globus_l_gram_conf_values_t * vals)
{
    int n = 52, i, x, z=0;
    char * cp;
    char * cp2;
    char ** arg;
    char * tmp_str = NULL;
    char * conf_contents;
    int  pfd;
    struct timeval time;
    time_t curr_time;
    struct tm tm;
    struct tm * tm_ptr = NULL;
    char tmp_date_str[128];
    char date_fmt[] = "%a %b %d %T GMT %Y";
    char * t;

    if ( !vals )
    {
        fprintf(stderr, "Configuration file not specified.\n");
        return(1);
    }
    if (! vals->conf_file )
    {
        fprintf(stderr, "Configuration file not specified.\n");
        return(1);
    }

    /* read the contents out of the conf file */
    conf_contents = (char *) malloc(BUFSIZ);  /* dont free */
    pfd = open(vals->conf_file,O_RDONLY);
    if (pfd == -1)
    {
        fprintf(stderr, "Failed to open the configuration file - %s\n",
                vals->conf_file);
        return(1);
    }
    i = read(pfd, conf_contents, BUFSIZ-1);
    if (i < 0)
    {
        fprintf(stderr, "Unable to read parameters from configuration "
                        "file\n");
        return(1);
    }
    conf_contents[i] = '\0';
    close(pfd);

    arg = (char**) malloc(n * sizeof(char *));

    /* parse the contents into an argument list */
    for (cp = (char *) strtok(conf_contents, " \t\n"); cp != 0; )
    {
        if ( cp[0] == '\'' && cp[strlen(cp) - 1] != '\'' )
        {
           cp2 = (char *) strtok(NULL, "'\n");
           tmp_str = malloc(sizeof(char *) * (strlen(cp) + strlen(cp2) + 2));
           sprintf(tmp_str, "%s %s", &cp[1], cp2);
        }
        else if ( cp[0] == '"' && cp[strlen(cp) - 1] != '"' )
        {
           cp2 = (char *) strtok(NULL, "\"\n");
           tmp_str = malloc(sizeof(char *) * (strlen(cp) + strlen(cp2) + 2));
           sprintf(tmp_str, "%s %s", &cp[1], cp2);
        }
        else
        {
           if (( cp[0] == '"' && cp[strlen(cp) - 1] == '"' ) ||
               ( cp[0] == '\'' && cp[strlen(cp) - 1] == '\'' ))
           {
               tmp_str = malloc(sizeof(char *) * strlen(cp));
               x = strlen(cp)-2;
               strncpy(tmp_str, &cp[1], x);
               tmp_str[x] = '\0';
           }
           else
           {
               tmp_str = cp;
           }
        }

        arg[z] = tmp_str;
        z++;
        if (z >= 52)
            return(1);
        cp = (char *) strtok(NULL, " \t\n");
    }

    arg[z] = (char *) 0;

    /* find the parameters the gram-scheduler cares about in the argument list
     */
    for (i = 0; i < z; i++)
    {
        if ((strcasecmp(arg[i], "-home") == 0)
             && (i + 1 < z))
        {
            vals->home_dir = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-org-dn") == 0)
                 && (i + 1 < z))
        {
            vals->org_dn = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-host-dn") == 0)
                 && (i + 1 < z))
        {
            vals->host_dn = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-gatekeeper-host") == 0)
                 && (i + 1 < z))
        {
            vals->gate_host= arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-gatekeeper-port") == 0)
                 && (i + 1 < z))
        {
            vals->gate_port= arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-gatekeeper-subject") == 0)
                 && (i + 1 < z))
        {
            vals->gate_subject= arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-host-manufacturer") == 0)
                 && (i + 1 < z))
        {
            vals->manufacturer = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-host-osname") == 0)
                 && (i + 1 < z))
        {
            vals->osname = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-host-osversion") == 0)
                 && (i + 1 < z))
        {
            vals->osversion = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-globus-host-cputype") == 0)
                 && (i + 1 < z))
        {
            vals->cputype = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-machine-type") == 0)
                 && (i + 1 < z))
        {
            vals->machine_type = arg[i+1];
        }
        else if ((strcasecmp(arg[i], "-condor-arch") == 0)
                 && (i + 1 < z))
        {
            vals->condor_arch = arg[i+1];
            for (t=vals->condor_arch; *t!='\0'; t++)
                *t = tolower(*t);
        }
        else if ((strcasecmp(arg[i], "-condor-os") == 0)
                 && (i + 1 < z))
        {
            vals->condor_os = arg[i+1];
            for (t=vals->condor_os; *t!='\0'; t++)
                *t = tolower(*t);
        }
        else if (strcasecmp(arg[i], "-dont-publish-jobs") == 0)
        {
            vals->publish_jobs = GLOBUS_FALSE;
        }
        /* printf("conf arg[%d] = %s\n", i, arg[i]); */
    }

    /* get the current GMT time in the format required by the MDS */
    gettimeofday( &time, NULL );
    curr_time = (time_t) time.tv_sec;

#ifdef HAVE_GMTIME_R
    gmtime_r(&curr_time, &tm);
    tm_ptr = &tm;
#else
    tm_ptr = gmtime(&curr_time);
#endif

    strftime(tmp_date_str, 127, date_fmt, tm_ptr);
    vals->curr_gmt_time = (char *) globus_libc_malloc (sizeof(char *) *
                              strlen(tmp_date_str) + 1);
    strcpy(vals->curr_gmt_time, tmp_date_str);

    if ( !vals->home_dir ||
         !vals->host_dn         ||
         !vals->gate_host       ||
         !vals->gate_port       ||
         !vals->gate_subject    ||
         !vals->cputype         ||
         !vals->osname          ||
         !vals->osversion       ||
         !vals->manufacturer    ||
         !vals->machine_type    ||
         !vals->org_dn )
    {
        fprintf(stderr, "Failed to find all required parameters from the "
                        "configuration file - %s\n", vals->conf_file);
        return(1);
    }

    return(0);

} /* globus_l_gram_get_conf_values() */


/******************************************************************************
Function:       main()
Description:
Parameters:
Returns: 
******************************************************************************/
int main (int argc, char **argv)
{
    globus_list_t *  q_list = NULL;
    char script_cmd[512];
    char lastupdate_map_file[512];
    globus_l_gram_conf_values_t conf_values;
    int show_conf_data = 0;
    int i;
    char * t;
    int fd;
    int rc;
    
    /* Initialize modules that I use */
    rc = globus_module_activate(GLOBUS_COMMON_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "common module activation failed with rc=%d\n", rc);
        exit(1);
    }

    globus_l_gram_conf_values_init(&conf_values);

    for (i = 1; i < argc; i++)
    {
        if ((strcasecmp(argv[i], "-condor-arch") == 0)
                 && (i + 1 < argc))
        {
            conf_values.condor_arch = argv[i+1];
            for (t=conf_values.condor_arch; *t!='\0'; t++)
                *t = tolower(*t);
            i++;
        }
        else if ((strcasecmp(argv[i], "-condor-os") == 0)
                 && (i + 1 < argc))
        {
            conf_values.condor_os = argv[i+1];
            for (t=conf_values.condor_os; *t!='\0'; t++)
                *t = tolower(*t);
            i++;
        }
        else if ((strcasecmp(argv[i], "-type") == 0)
                 && (i + 1 < argc))
        {
            conf_values.type = argv[i+1];
            i++;
        }
        else if ((strcasecmp(argv[i], "-rdn") == 0)
                 && (i + 1 < argc))
        {
            conf_values.rdn = argv[i+1];
            i++;
        }
        else if ((strcasecmp(argv[i], "-dmdn") == 0)
                 && (i + 1 < argc))
        {
            conf_values.dmdn = argv[i+1];
            i++;
        }
        else if ((strcasecmp(argv[i], "-conf") == 0)
                 && (i + 1 < argc))
        {
            conf_values.conf_file = argv[i+1];
            i++;
        }
        else if ((strcasecmp(argv[i], "-cldif") == 0)
                 && (i + 1 < argc))
        {
            conf_values.cldif_file = argv[i+1];
            i++;
        }
        else if ((strcasecmp(argv[i], "-machine-type") == 0)
                 && (i + 1 < argc))
        {
            conf_values.machine_type = argv[i+1];
            i++;
        }
        else if (strcasecmp(argv[i], "-verbose") == 0)
        {
            verbose = 1;
        }
        else if (strcasecmp(argv[i], "-dont-publish-jobs") == 0)
        {
            conf_values.publish_jobs = GLOBUS_FALSE;
        }
        else if (strcasecmp(argv[i], "-show-conf-data") == 0)
        {
            show_conf_data = 1;
        }
        else if (strcasecmp(argv[i], "-help") == 0)
        {
            print_usage();
            exit(1);
        }
        else
        {
            fprintf(stderr, "Unknown argument %s\n", argv[i]);
        }
    }

        
    if ( conf_values.conf_file == NULL )
    {
        fprintf(stderr, "Error: -conf parameter is required.\n");
        print_usage();
        exit(1);
    }
        
    if ( conf_values.rdn == NULL )
    {
        fprintf(stderr, "Error: -rdn parameter is required.\n");
        print_usage();
        exit(1);
    }
        
    if ( conf_values.dmdn == NULL )
    {
        fprintf(stderr, "Error: -dmdn parameter is required.\n");
        print_usage();
        exit(1);
    }
        
    if ( conf_values.type == NULL )
    {
        fprintf(stderr, "Error: -type parameter is required.\n");
        print_usage();
        exit(1);
    }
        
    if (verbose)
        fprintf(stdout, "reading configuration file...............\n");

    if (globus_l_gram_get_conf_values(&conf_values) != 0)
    {
        exit(1);
    }

    if ( strcasecmp(conf_values.type, "condor") == 0)
    {
        if ( conf_values.condor_arch == NULL )
        {
            fprintf(stderr, "Error: -condor_arch parameter is required "
                            "when job manager type is condor.\n");
            print_usage();
            exit(1);
        }
        
        if ( conf_values.condor_os == NULL )
        {
            fprintf(stderr, "Error: -condor_os parameter is required "
                            "when job manager type is condor.\n");
            print_usage();
            exit(1);
        }
        sprintf(script_cmd, "%s/libexec/globus-script-condor-queue %s %s\n",
                             conf_values.home_dir,
                             conf_values.condor_arch,
                             conf_values.condor_os);

    }
    else
    {
        sprintf(script_cmd, "%s/libexec/globus-script-%s-queue\n",
                             conf_values.home_dir,
                             conf_values.type);
    }

    /* open cldif file or use stdout as default */
    if ( conf_values.cldif_file == NULL )
    {
        conf_values.cldif_fp = stdout;
    }
    else
    {
        if ((conf_values.cldif_fp = fopen(conf_values.cldif_file, "w")) == NULL)
        {
            fprintf(stderr, "Failed to open cldif file for writing\n");
            fprintf(stderr, "  filename --> %s\n", conf_values.cldif_file);
            return(1);
        }
    }

    if (verbose)
        fprintf(stdout, "getting queue info.......................\n");

    if (globus_gram_scheduler_queue_list_get(script_cmd, &q_list) != 0)
    {
        fprintf(stderr,"Failed getting queue information.\n");
        return(1);
    }
    else
    {
        if (verbose)
            fprintf(stdout, "Adding globus info to queue info.........\n");

        globus_l_gram_check_globus_jobs(&conf_values, q_list);

        if (show_conf_data)
            globus_l_gram_show_conf_data(&conf_values);

        if (verbose)
        {
            if (conf_values.publish_jobs)
                fprintf(stdout, "Writing gram info including job entries to "
                                "the cldif file.....\n");
            else
                fprintf(stdout, "Writing gram info excluding job entries to "
                                "the cldif file.....\n");
        }

        if (globus_l_gram_write_gram_cldif_file(&conf_values, q_list) !=0 )
        {
            fprintf(stderr, "Failed writing the gram information "
                            "to the cldif file.\n");
            return(GLOBUS_FAILURE);
        }

        /* close cldif file pointer unless it is pointing to stdout */
        if ( conf_values.cldif_file != NULL )
            if (conf_values.cldif_fp) fclose(conf_values.cldif_fp);

        if (verbose)
            printf("Freeing internal configuration values.\n");

        globus_l_gram_conf_values_free(&conf_values);

        if (verbose)
            fprintf(stdout, "Freeing the queue info memory............\n");

        if (globus_gram_scheduler_queue_list_free(q_list) != 0)
            printf("Error: globus_gram_scheduler_queue_list_free failed.\n");
    }

    rc = globus_module_deactivate(GLOBUS_COMMON_MODULE);
    if (rc != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "common deactivation failed with rc=%d\n", rc);
        exit(1);
    }

    return(0);
}
