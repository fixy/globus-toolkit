/*
 * Copyright 1999-2006 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined GRIDFTP_REGISTRY_H
#define GRIDFTP_REGISTRY_H 1

#include "globus_soap_message.h"
#include "globus_wsrf_resource.h"
#include "globus_service_engine.h"
#include "globus_operation_provider.h"
#include "globus_wsrf_core_tools.h"
#include "globus_ws_addressing.h"

#include "globus_gridftp_server.h"
#include "globus_service_engine.h"

#include "FrontendStats.h"
#include "FrontendStatsType.h"
#include "BackendPool.h"

#define ELEMENT_NAME "GridFTPAdmin"
#define RESOURCE_NAME "GridFTPAdmin"
#define GRIDFTP_ADMIN_SERVICE_NAMESPACE "http://www.globus.org/namespaces/2005/09/GridFTPAdmin"


void
gridftpA_l_fe_get_cb(
    void *                              arg,
    const xsd_QName *                   qname,
    void **                             property);

globus_bool_t
gridftpA_l_fe_set_cb(
    void *                              arg,
    const xsd_QName *                   qname,
    void *                              property);

void
gridftpA_l_fe_change_cb(
    const char *                        opt_name,
    const char *                        val,
    void *                              user_arg);

void
gridftpA_l_backend_change_cb(
    const char *                        opt_name,
    const char *                        val,
    void *                              user_arg);

void
gridftpA_l_backend_get_cb(
    void *                              arg,
    const xsd_QName *                   qname,
    void **                             property);

globus_bool_t
gridftpA_l_backend_set_cb(
    void *                              arg,
    const xsd_QName *                   qname,
    void *                              property);
#endif