/*
 * URL download support library
 * Copyright (C) 2015-2019, Wazuh Inc.
 * October 26, 2018.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "shared.h"
#include "../config/config.h"
#include "../config/global-config.h"

// Returns 1 if the node is a worker, 0 if it is not and -1 if error.
int w_is_worker(void) {

    XML_NODE node = NULL;
    OS_XML xml;
    const char * xmlf[] = {"ossec_config", "cluster", NULL};
    const char * xmlf2[] = {"ossec_config", "cluster", "node_type", NULL};
    const char * xmlf3[] = {"ossec_config", "cluster", "disabled", NULL};
    const char *cfgfile = DEFAULTCPATH;
    int modules = 0;
    int is_worker = 0;
    _Config cfg;
    memset(&cfg, 0, sizeof(_Config));

    modules |= CCLUSTER;

    node = OS_GetElementsbyNode(&xml, NULL);

    if (ReadConfig(modules, cfgfile, &cfg, NULL) < 0) {
        return (OS_INVALID);
    }

    if (Read_Cluster(node, &cfg, NULL) < 0){
        return(OS_INVALID);
    }

    if (OS_ReadXML(cfgfile, &xml) < 0) {
        mdebug1(XML_ERROR, cfgfile, xml.err, xml.err_line);
    } else {
        char * cl_config = OS_GetOneContentforElement(&xml, xmlf);
        if (cl_config && cl_config[0] != '\0') {
            char * cl_type = OS_GetOneContentforElement(&xml, xmlf2);
                if (cl_type && cl_type[0] != '\0') {
                    char * cl_status = OS_GetOneContentforElement(&xml, xmlf3);
                    if(cl_status && cl_status[0] != '\0'){
                	    if (!strcmp(cl_status, "no")) {
                            if (!strcmp(cl_type, "client") || !strcmp(cl_type, "worker")) {
                                is_worker = 1;
                            } else {
                                is_worker = 0;
                            }
                        } else {
                            is_worker = 0;
                        }
                	} else {
                        if (!strcmp(cl_type, "client") || !strcmp(cl_type, "worker")) {
                            is_worker = 1;
                        } else {
                        	is_worker = 0;
                        }
                	}
                    free(cl_status);
                    free(cl_type);
                }
            free(cl_config);
        } else {
            is_worker = 0;
        }
    }
    OS_ClearXML(&xml);
    config_free(&cfg);

    return is_worker;
}


char *get_master_node(void) {

    OS_XML xml;
    const char * xmlf[] = {"ossec_config", "cluster", "nodes", "node", NULL};
    const char *cfgfile = DEFAULTCPATH;
    _Config cfg;
    int modules = 0;
    char *master_node = NULL;
    memset(&cfg, 0, sizeof(_Config));

    modules |= CCLUSTER;

    if (ReadConfig(modules, cfgfile, &cfg, NULL) < 0) {
        master_node = strdup("undefined");
    }

    if (OS_ReadXML(cfgfile, &xml) < 0) {
        mdebug1(XML_ERROR, cfgfile, xml.err, xml.err_line);
    } else {
        os_free(master_node);
        master_node = OS_GetOneContentforElement(&xml, xmlf);
    }

    OS_ClearXML(&xml);

    if (!master_node) {
        master_node = strdup("undefined");
    }

    config_free(&cfg);

    return master_node;
}

// Get cluster node name from configuration or environment variables
char * wm_node_name() {
    const char *(xml_node[]) = {"ossec_config", "cluster", "node_name", NULL};
    char * node_name;

    OS_XML xml;

    if (OS_ReadXML(DEFAULTCPATH, &xml) < 0){
        merror_exit(XML_ERROR, DEFAULTCPATH, xml.err, xml.err_line);
    }

    node_name = OS_GetOneContentforElement(&xml, xml_node);
    OS_ClearXML(&xml);

    if (!node_name) {
        return NULL;
    }

    // Get environment variables

    if (strcmp(node_name, "$NODE_NAME") == 0) {
        free(node_name);
        node_name = getenv("NODE_NAME");

        if (node_name) {
            return strdup(node_name);
        } else {
            mwarn("Cannot find environment variable 'NODE_NAME'");
            return NULL;
        }
    } else if (strcmp(node_name, "$HOSTNAME") == 0) {
        char hostname[512];

        free(node_name);

        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strncpy(hostname, "localhost", sizeof(hostname));
        }

        hostname[sizeof(hostname) - 1] = '\0';
        return strdup(hostname);
    } else {
        return node_name;
    }
}