#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include "send-graphite.h"

#include "include/nagios/objects.h"
#include "include/nagios/nagios.h"
#include "include/nagios/nebstructs.h"
#include "include/nagios/broker.h"
#include "include/nagios/nebmodules.h"
#include "include/nagios/nebcallbacks.h"

#define DEFAULT_GRAPHITE_PORT 2003
#define DEFAULT_GRAPHITE_HOST "localhost"
#define DEFAULT_GRAPHITE_PREFIX "nagios"
#define DEFAULT_GRAPHITE_DEBUG 0

#define LG_INFO 262144
#define LG_WARN  LOG_INFO
#define LG_ERR   LOG_INFO
#define LG_CRIT  LOG_INFO
#define LG_DEBUG LOG_INFO
#define LG_ALERT LOG_INFO
#define MAX_MESSAGE 1024*1024

void *g_nagios_graphite_handle = NULL;
void *g_nagios_graphite_context;
int   g_nagios_graphite_port = DEFAULT_GRAPHITE_PORT;
char *g_nagios_graphite_host = DEFAULT_GRAPHITE_HOST;
char *g_nagios_graphite_prefix = DEFAULT_GRAPHITE_PREFIX;
int   g_nagios_graphite_debug = DEFAULT_GRAPHITE_DEBUG;

NEB_API_VERSION(CURRENT_NEB_API_VERSION)

  /*
     It's important to namespace functions and variables, otherwise they might conflict with other NEB's functions

     Information about the different NEB structs can be found at:
     -  http://nagios.svn.sourceforge.net/viewvc/nagios/nagioscore/trunk/include/nebstructs.h?&view=markup

     The different callbacks can be found at:
     - http://nagios.svn.sourceforge.net/viewvc/nagios/nagioscore/trunk/include/nebcallbacks.h?&view=markup

*/

void nagios_graphite_logger(int priority, const char *loginfo, ...)
{
  char buffer[8192];
  snprintf(buffer, 20, "graphite: ");

  va_list ap;
  va_start(ap, loginfo);
  vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), loginfo, ap);
  va_end(ap);
  write_to_all_logs(buffer, priority);
}

void nagios_graphite_parse_arguments(const char *args) {
  char arguments[1024];
  char *arg_term;

  // no arguments
  if(!args) return;

  strncpy(arguments, args, 1024);
  arg_term = strtok(arguments, " =");
  while(arg_term != NULL) {
    char *key, *value;

    key      = arg_term;
    arg_term = strtok(NULL, " =");
    value    = arg_term;

    if(!strncmp(key, "port", 4)) {
      g_nagios_graphite_port = atoi(value);
    }

    if(!strncmp(key, "debug", 4)) {
      g_nagios_graphite_debug = atoi(value);
    }

    if(!strncmp(key, "host", 4)) {
      g_nagios_graphite_host= strdup(value);
      nagios_graphite_logger(LG_DEBUG, "'%s'\n",g_nagios_graphite_host);
    }

    if(!strncmp(key, "prefix", 6)) {
      g_nagios_graphite_prefix = strdup(value);
      nagios_graphite_logger(LG_DEBUG, "'%s'\n",g_nagios_graphite_prefix);
    }

    arg_term = strtok(NULL, " =");
  }
}

int nagios_graphite_send_servicecheck(nebstruct_service_check_data *check_data) {

  char hostname[1024];
  char servicename[1024];
  char metricprefix[1024];

  // Sanitize hostname string
  sprintf(hostname,"%s",check_data->host_name);

  int i;
  for ( i = 0 ; hostname[i]; i++) {
    hostname[i] = tolower(hostname[i]);
    if (! isalnum(hostname[i])) {
      hostname[i]='_';
    }
  }

  // Sanitize service name
  sprintf(servicename,"%s",check_data->service_description);
  for ( i = 0 ; servicename[i]; i++) {
    servicename[i] = tolower(servicename[i]);
    if (! isalnum(servicename[i])) {
      servicename[i]='_';
    }
  }

  // Calculate service prefix
  sprintf(metricprefix,"%s.%s",hostname,servicename);

  // Send services metric
  nagios_graphite_send_generic_check_service("service",metricprefix,check_data) ;

  return 0;
}

int nagios_graphite_send_hostcheck(nebstruct_host_check_data *check_data) {

  char hostname[1024];
  char metricprefix[1024];

  // Convert hostname to string
  sprintf(hostname,"%s",check_data->host_name);

  int i;
  for ( i = 0 ; hostname[i]; i++) {
    hostname[i] = tolower(hostname[i]);
    if (! isalnum(hostname[i])) {
      hostname[i]='_';
    }
  }

  // Calculate host prefix
  sprintf(metricprefix,"%s",hostname);

  // Send host metric
  nagios_graphite_send_generic_check_host("host",metricprefix,check_data);
  return 0;
}

// Intentionally duplicated because of C not being able to dynamically typecast
int nagios_graphite_send_generic_check_service(char *type,char *metricprefix,nebstruct_service_check_data *check_data) {

  struct timeval ts ;
  char metricname[1024];
  char metricvalue[1024];

  gettimeofday(&ts,NULL);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"current_attempt");
  sprintf(metricvalue, "%i",check_data->current_attempt);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"max_attempts");
  sprintf(metricvalue, "%i",check_data->max_attempts);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"check_type");
  sprintf(metricvalue, "%i",check_data->check_type);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"state_type");
  sprintf(metricvalue, "%i",check_data->state_type);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"state");
  sprintf(metricvalue, "%i",check_data->state);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"execution_time");
  sprintf(metricvalue, "%f",check_data->execution_time);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"early_timeout");
  sprintf(metricvalue, "%d",check_data->early_timeout);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"timeout");
  sprintf(metricvalue, "%d",check_data->timeout);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"latency");
  sprintf(metricvalue, "%f",check_data->latency);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"return_code");
  sprintf(metricvalue, "%d",check_data->return_code);
  send_graphite(metricname,metricvalue,ts.tv_sec);

     if(check_data->perf_data) {
  nagios_graphite_logger(LG_INFO, "'%s'\n", check_data->perf_data);
  }

  return 0;
}

int nagios_graphite_send_generic_check_host(char *type,char *metricprefix,nebstruct_host_check_data *check_data) {

  struct timeval ts ;
  char metricname[1024];
  char metricvalue[1024];

  gettimeofday(&ts,NULL);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"current_attempt");
  sprintf(metricvalue, "%i",check_data->current_attempt);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"max_attempts");
  sprintf(metricvalue, "%i",check_data->max_attempts);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"check_type");
  sprintf(metricvalue, "%i",check_data->check_type);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"state_type");
  sprintf(metricvalue, "%i",check_data->state_type);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"state");
  sprintf(metricvalue, "%i",check_data->state);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"execution_time");
  sprintf(metricvalue, "%f",check_data->execution_time);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"early_timeout");
  sprintf(metricvalue, "%d",check_data->early_timeout);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"timeout");
  sprintf(metricvalue, "%d",check_data->timeout);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"latency");
  sprintf(metricvalue, "%f",check_data->latency);
  send_graphite(metricname,metricvalue,ts.tv_sec);

  sprintf(metricname, "%s.%s.%s",type,metricprefix,"return_code");
  sprintf(metricvalue, "%d",check_data->return_code);
  send_graphite(metricname,metricvalue,ts.tv_sec);

     if(check_data->perf_data) {
  nagios_graphite_logger(LG_INFO, "'%s'\n", check_data->perf_data);
  }

  return 0;
}

int nagios_graphite_send_notification(nebstruct_notification_data *notification_data) {
  /*

     struct timeval ts ;
     gettimeofday(&ts,NULL);
     json_object * jobj = json_object_new_object();

     json_add_pair(jobj, "hostname",        notification_data->host_name);
     sprintf(cast_buffer, "%ld",            notification_data->start_time.tv_sec);
     json_add_pair(jobj, "start_time",       cast_buffer);
     sprintf(cast_buffer, "%ld",            notification_data->end_time.tv_sec);
     json_add_pair(jobj, "end_time",       cast_buffer);
     json_add_pair(jobj, "service_description",        notification_data->service_description);
     sprintf(cast_buffer, "%i",             notification_data->reason_type);
     json_add_pair(jobj, "reason_type",      cast_buffer);
     sprintf(cast_buffer, "%i",             notification_data->state);
     json_add_pair(jobj, "state",      cast_buffer);
     json_add_pair(jobj, "output",        notification_data->output);

     if (notification_data->ack_author != NULL) {
     json_add_pair(jobj, "ack_author",        notification_data->ack_author);
     }
     if (notification_data->ack_data != NULL) {
     json_add_pair(jobj, "ack_data",        notification_data->ack_data);
     }
     sprintf(cast_buffer, "%i",             notification_data->escalated);
     json_add_pair(jobj, "escalated",      cast_buffer);
     sprintf(cast_buffer, "%i",             notification_data->contacts_notified);
     json_add_pair(jobj, "contacts_notified",      cast_buffer);

*/
  return 0;
}

int nagios_graphite_broker_check(int event_type, void *data) {
  if (event_type == NEBCALLBACK_SERVICE_CHECK_DATA) {
    nebstruct_service_check_data *c = (nebstruct_service_check_data *)data;
    if (c->type == NEBTYPE_SERVICECHECK_PROCESSED) {
      nagios_graphite_send_servicecheck(c);
    }
  }
  else if (event_type == NEBCALLBACK_HOST_CHECK_DATA) {
    nebstruct_host_check_data *c = (nebstruct_host_check_data *)data;
    if (c->type == NEBTYPE_HOSTCHECK_PROCESSED) {
      nagios_graphite_send_hostcheck(c);
    }
  }

  return 0;
}

int nagios_graphite_broker_notification(int event_type __attribute__ ((__unused__)), void *data) {
  nebstruct_notification_data *n = (nebstruct_notification_data *)data;
  nagios_graphite_send_notification(n);
}

int nagios_graphite_broker_state(int event_type __attribute__ ((__unused__)), void *data __attribute__ ((__unused__))) {
  return 0;
}

int nagios_graphite_broker_process(int event_type __attribute__ ((__unused__)), void *data) {
  struct nebstruct_process_struct *ps = (struct nebstruct_process_struct *)data;
  if (ps->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {

    //nagios_graphite_start_threads();
  }

  return 0;
}

/* this function gets called when the module is loaded by the event broker */
int nebmodule_init(int flags __attribute__ ((__unused__)), char *args, void *handle) {
  g_nagios_graphite_handle = handle;

  nagios_graphite_logger(LG_INFO, "nagios2graphite by Patrick Debois");

  nagios_graphite_parse_arguments(args);

  neb_register_callback(NEBCALLBACK_STATE_CHANGE_DATA,  g_nagios_graphite_handle, 0, nagios_graphite_broker_state);
  neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, g_nagios_graphite_handle, 0, nagios_graphite_broker_check);
  neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA,    g_nagios_graphite_handle, 0, nagios_graphite_broker_check);
  neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA,  g_nagios_graphite_handle, 0, nagios_graphite_broker_notification);
  // used for starting threads
  neb_register_callback(NEBCALLBACK_PROCESS_DATA,       g_nagios_graphite_handle, 0, nagios_graphite_broker_process);

  nagios_graphite_logger(LG_INFO, "successfully finished initialization");
}

int nebmodule_deinit(int flags __attribute__ ((__unused__)), int reason __attribute__ ((__unused__))) {
  nagios_graphite_logger(LG_INFO, "deinitializing");

  neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA,  nagios_graphite_broker_state);
  neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, nagios_graphite_broker_check);
  neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA,    nagios_graphite_broker_check);
  neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA,  nagios_graphite_broker_notification);
  neb_deregister_callback(NEBCALLBACK_PROCESS_DATA,       nagios_graphite_broker_process);

  return 0;
}
