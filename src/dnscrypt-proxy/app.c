
#include <config.h>
#include <sys/types.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/util.h>

#include "app.h"
#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "salsa20_random.h"
#include "logger.h"
#include "options.h"
#include "stack_trace.h"
#include "tcp_request.h"
#include "udp_request.h"

static AppContext app_context;

static int
proxy_context_init(ProxyContext * const proxy_context, int argc, char *argv[])
{
    char sockaddr_port[256U];
    int  sockaddr_len_int;

    memset(proxy_context, 0, sizeof *proxy_context);
    proxy_context->event_loop = NULL;
    proxy_context->tcp_accept_timer = NULL;
    proxy_context->tcp_conn_listener = NULL;
    proxy_context->udp_listener_event = NULL;
    proxy_context->udp_proxy_resolver_event = NULL;
    proxy_context->udp_proxy_resolver_handle = -1;
    proxy_context->udp_listener_handle = -1;
    if (options_parse(&app_context, proxy_context, argc, argv) != 0) {
        return -1;
    }
    if ((proxy_context->event_loop = event_base_new()) == NULL) {
        logger(NULL, LOG_ERR, "Unable to initialize the event loop");
        return -1;
    }
    if (strchr(proxy_context->resolver_ip, ':') != NULL &&
        *proxy_context->resolver_ip != '[') {
        evutil_snprintf(sockaddr_port, sizeof sockaddr_port, "[%s]:%s",
                        proxy_context->resolver_ip,
                        proxy_context->resolver_port);
    } else {
        evutil_snprintf(sockaddr_port, sizeof sockaddr_port, "%s:%s",
                        proxy_context->resolver_ip,
                        proxy_context->resolver_port);
    }
    sockaddr_len_int = (int) sizeof proxy_context->resolver_sockaddr;
    if (evutil_parse_sockaddr_port(sockaddr_port,
                                   (struct sockaddr *)
                                   &proxy_context->resolver_sockaddr,
                                   &sockaddr_len_int) != 0) {
        logger(NULL, LOG_ERR, "Unsupported resolver address: %s",
               sockaddr_port);
        return -1;
    }
    proxy_context->resolver_sockaddr_len = (ev_socklen_t) sockaddr_len_int;

    if (strchr(proxy_context->local_ip, ':') != NULL &&
        *proxy_context->local_ip != '[') {
        evutil_snprintf(sockaddr_port, sizeof sockaddr_port, "[%s]:%s",
                        proxy_context->local_ip, proxy_context->local_port);
    } else {
        evutil_snprintf(sockaddr_port, sizeof sockaddr_port, "%s:%s",
                        proxy_context->local_ip, proxy_context->local_port);
    }
    sockaddr_len_int = (int) sizeof proxy_context->local_sockaddr;
    if (evutil_parse_sockaddr_port(sockaddr_port,
                                   (struct sockaddr *)
                                   &proxy_context->local_sockaddr,
                                   &sockaddr_len_int) != 0) {
        logger(NULL, LOG_ERR, "Unsupported local address: %s",
               sockaddr_port);
        return -1;
    }
    proxy_context->local_sockaddr_len = (ev_socklen_t) sockaddr_len_int;

    return 0;
}

static void
proxy_context_free(ProxyContext * const proxy_context)
{
    if (proxy_context == NULL) {
        return;
    }
    options_free(proxy_context);
    logger_close(proxy_context);
}

static
int init_tz(void)
{
    static char  default_tz_for_putenv[] = "TZ=UTC+00:00";
    char         stbuf[10U];
    struct tm   *tm;
    time_t       now;

    tzset();
    time(&now);
    if ((tm = localtime(&now)) != NULL &&
        strftime(stbuf, sizeof stbuf, "%z", tm) == (size_t) 5U) {
        evutil_snprintf(default_tz_for_putenv, sizeof default_tz_for_putenv,
                        "TZ=UTC%c%c%c:%c%c", (*stbuf == '-' ? '+' : '-'),
                        stbuf[1], stbuf[2], stbuf[3], stbuf[4]);
    }
    putenv(default_tz_for_putenv);
    (void) localtime(&now);
    (void) gmtime(&now);

    return 0;
}

static void
revoke_privileges(ProxyContext * const proxy_context)
{
    (void) proxy_context;
#ifndef DEBUG
    salsa20_random_stir();
    init_tz();
    (void) strerror(ENOENT);
# ifndef _WIN32
    if (proxy_context->user_dir != NULL) {
        if (chdir(proxy_context->user_dir) != 0 ||
            chroot(proxy_context->user_dir) != 0 || chdir("/") != 0) {
            logger(proxy_context, LOG_ERR, "Unable to chroot to [%s]",
                   proxy_context->user_dir);
            exit(1);
        }
    }
    if (proxy_context->user_id != (uid_t) 0) {
        if (setgid(proxy_context->user_group) != 0 ||
            setegid(proxy_context->user_group) != 0 ||
            setuid(proxy_context->user_id) != 0 ||
            seteuid(proxy_context->user_id) != 0) {
            logger(proxy_context, LOG_ERR, "Unable to switch to user id [%lu]",
                   (unsigned long) proxy_context->user_id);
            exit(1);
        }
    }
# endif
#endif
}

int
dnscrypt_proxy_start_listeners(ProxyContext * const proxy_context)
{
    if (proxy_context->listeners_started != 0) {
        return 0;
    }
    if (tcp_listener_start(proxy_context) != 0 ||
        udp_listener_start(proxy_context) != 0) {
        exit(1);
    }
    logger(proxy_context, LOG_INFO, "Proxying from [%s (%s)] to [%s (%s)]",
           proxy_context->local_ip, proxy_context->local_port,
           proxy_context->resolver_ip, proxy_context->resolver_port);

    proxy_context->listeners_started = 1;

    return 0;
}

int
main(int argc, char *argv[])
{
    ProxyContext  proxy_context;

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    stack_trace_on_crash();
    if (proxy_context_init(&proxy_context, argc, argv) != 0) {
        logger_noformat(NULL, LOG_ERR, "Unable to start the proxy");
        exit(1);
    }
    app_context.proxy_context = &proxy_context;
    logger_noformat(&proxy_context, LOG_INFO, "Generating a new key pair");
    dnscrypt_client_init_with_new_key_pair(&proxy_context.dnscrypt_client);
    logger_noformat(&proxy_context, LOG_INFO, "Done");

    if (cert_updater_init(&proxy_context) != 0 ||
        tcp_listener_bind(&proxy_context) != 0 ||
        udp_listener_bind(&proxy_context) != 0) {
        exit(1);
    }
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
    revoke_privileges(&proxy_context);

    if (cert_updater_start(&proxy_context) != 0) {
        exit(1);
    }
    event_base_dispatch(proxy_context.event_loop);

    logger_noformat(&proxy_context, LOG_INFO, "Stopping proxy");
    cert_updater_free(&proxy_context);
    tcp_listener_stop(&proxy_context);
    udp_listener_stop(&proxy_context);
    event_base_free(proxy_context.event_loop);
    proxy_context_free(&proxy_context);
    app_context.proxy_context = NULL;
    salsa20_random_close();

    return 0;
}
