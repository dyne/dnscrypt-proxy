
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event2/util.h>
#include <sodium.h>

#include "dnscrypt_proxy.h"
#include "getpwnam.h"
#include "options.h"
#include "logger.h"
#include "minicsv.h"
#include "pid_file.h"
#include "utils.h"
#include "windows_service.h"
#ifdef PLUGINS
# include "plugin_options.h"
#endif

static struct option getopt_long_options[] = {
    { "config", 1, NULL, 'c' },
    { "dump-config", 0, NULL, 'C' },
    { "resolver-name", 1, NULL, 'R' },
    { "local-address", 1, NULL, 'a' },
#ifndef _WIN32
    { "daemonize", 0, NULL, 'd' },
#endif
    { "ephemeral-keys", 0, NULL, 'E' },
    { "client-key", 1, NULL, 'K' },
    { "resolvers-list", 1, NULL, 'L' },
    { "logfile", 1, NULL, 'l' },
    { "loglevel", 1, NULL, 'm' },
#ifndef _WIN32
    { "pidfile", 1, NULL, 'p' },
#endif
    { "plugin", 1, NULL, 'X' },
    { "provider-name", 1, NULL, 'N' },
    { "provider-key", 1, NULL, 'k' },
    { "resolver-address", 1, NULL, 'r' },
#ifndef _WIN32
    { "syslog", 0, NULL, 'S' },
    { "syslog-prefix", 1, NULL, 'Z' },
#endif
    { "max-active-requests", 1, NULL, 'n' },
#ifdef HAVE_GETPWNAM
    { "user", 1, NULL, 'u' },
#endif
    { "test", 1, NULL, 't' },
    { "tcp-only", 0, NULL, 'T' },
    { "edns-payload-size", 1, NULL, 'e' },
    { "ignore-timestamps", 0, NULL, 'I' },
    { "version", 0, NULL, 'V' },
    { "help", 0, NULL, 'h' },
#ifdef _WIN32
    { "install", 0, NULL, WIN_OPTION_INSTALL },
    { "reinstall", 0, NULL, WIN_OPTION_REINSTALL },
    { "uninstall", 0, NULL, WIN_OPTION_UNINSTALL },
    { "service-name", 1, NULL, WIN_OPTION_SERVICE_NAME },
#endif
    { NULL, 0, NULL, 0 }
};
#ifndef _WIN32
static const char *getopt_options = "a:c:Cde:EhIk:K:L:l:m:n:p:r:R:St:u:N:TVX:Z:";
#else
static const char *getopt_options = "a:c:Ce:EhIk:K:L:l:m:n:r:R:t:u:N:TVX:";
#endif

#ifndef DEFAULT_CONNECTIONS_COUNT_MAX
# define DEFAULT_CONNECTIONS_COUNT_MAX 250U
#endif
#define DEFAULT_LOCAL_ADDRESS "127.0.0.1:53"

static void
options_version(void)
{
    puts(PACKAGE_STRING);
}

static void
options_usage(void)
{
    const struct option *options = getopt_long_options;

    options_version();
    puts("\nOptions:\n");
    do {
        if (options->val < 256) {
            printf("  -%c\t--%s%s\n", options->val, options->name,
                   options->has_arg ? "=..." : "");
        } else {
            printf("    \t--%s%s\n", options->name,
                   options->has_arg ? "=..." : "");
        }
        options++;
    } while (options->name != NULL);
    puts("\nPlease consult the dnscrypt-proxy(8) man page for details.\n");
}

static
void options_init_with_default(AppContext * const app_context,
                               ProxyContext * const proxy_context)
{
    assert(proxy_context->event_loop == NULL);
    proxy_context->app_context = app_context;
    proxy_context->connections_count = 0U;
    proxy_context->connections_count_max = DEFAULT_CONNECTIONS_COUNT_MAX;
    proxy_context->edns_payload_size = (size_t) DNS_DEFAULT_EDNS_PAYLOAD_SIZE;
    proxy_context->client_key_file = NULL;
    proxy_context->local_ip = strdup(DEFAULT_LOCAL_ADDRESS);
    proxy_context->log_fp = NULL;
    proxy_context->log_file = NULL;
    proxy_context->pid_file = NULL;
    proxy_context->resolvers_list = (DEFAULT_RESOLVERS_LIST == NULL) ? NULL
                                  : strdup(DEFAULT_RESOLVERS_LIST);
    proxy_context->resolver_name = (DEFAULT_RESOLVER_NAME == NULL) ? NULL
                                 : strdup(DEFAULT_RESOLVER_NAME);
    proxy_context->provider_name = NULL;
    proxy_context->provider_publickey_s = NULL;
    proxy_context->resolver_ip = NULL;
    proxy_context->syslog = 0;
    proxy_context->syslog_prefix = NULL;
#ifndef _WIN32
    proxy_context->user_name = NULL;
    proxy_context->user_id = (uid_t) 0;
    proxy_context->user_group = (uid_t) 0;
    proxy_context->user_dir = NULL;
#endif
    proxy_context->daemonize = 0;
    proxy_context->test_cert_margin = (time_t) -1;
    proxy_context->test_only = 0;
    proxy_context->tcp_only = 0;
    proxy_context->ephemeral_keys = 0;
    proxy_context->ignore_timestamps = 0;
}

static int
options_check_protocol_versions(const char * const provider_name)
{
    const size_t dnscrypt_protocol_versions_len =
        sizeof DNSCRYPT_PROTOCOL_VERSIONS - (size_t) 1U;

    if (strncmp(provider_name, DNSCRYPT_PROTOCOL_VERSIONS,
                dnscrypt_protocol_versions_len) != 0 ||
        provider_name[dnscrypt_protocol_versions_len] != '.') {
        return -1;
    }
    return 0;
}

static char *
options_read_file(const char * const file_name)
{
    FILE   *fp;
    char   *file_buf;
    size_t  file_size = (size_t) 0U;

    assert(file_name != NULL);
    if ((fp = fopen(file_name, "rb")) == NULL) {
        return NULL;
    }
    while (fgetc(fp) != EOF && file_size < SIZE_MAX) {
        file_size++;
    }
    if (feof(fp) == 0 || file_size <= (size_t) 0U) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    if ((file_buf = malloc(file_size + 1)) == NULL) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_buf, file_size, (size_t) 1U, fp) != 1U) {
        fclose(fp);
        free(file_buf);
        return NULL;
    }
    (void) fclose(fp);
    file_buf[file_size] = 0;

    return file_buf;
}

static const char *
options_get_col(char * const * const headers, const size_t headers_count,
                char * const * const cols, const size_t cols_count,
                const char * const header)
{
    size_t i = (size_t) 0U;

    while (i < headers_count) {
        if (strcmp(header, headers[i]) == 0) {
            if (i < cols_count) {
                return cols[i];
            }
            break;
        }
        i++;
    }
    return NULL;
}

static int
options_parse_resolver(ProxyContext * const proxy_context,
                       char * const * const headers, const size_t headers_count,
                       char * const * const cols, const size_t cols_count)
{
    const char *dnssec;
    const char *namecoin;
    const char *nologs;
    const char *provider_name;
    const char *provider_publickey_s;
    const char *resolver_ip;
    const char *resolver_name;

    resolver_name = options_get_col(headers, headers_count,
                                    cols, cols_count, "Name");
    if (resolver_name == NULL) {
        logger(proxy_context, LOG_ERR,
               "Invalid resolvers list file: missing 'Name' column");
        exit(1);
    }
    if (*resolver_name == 0) {
        logger(proxy_context, LOG_ERR, "Resolver with an empty name");
        return -1;
    }
    if (evutil_ascii_strcasecmp(resolver_name,
                                proxy_context->resolver_name) != 0) {
        return 0;
    }
    provider_name = options_get_col(headers, headers_count,
                                    cols, cols_count, "Provider name");
    provider_publickey_s = options_get_col(headers, headers_count,
                                           cols, cols_count,
                                           "Provider public key");
    resolver_ip = options_get_col(headers, headers_count,
                                  cols, cols_count, "Resolver address");
    if (provider_name == NULL || *provider_name == 0) {
        logger(proxy_context, LOG_ERR,
               "Resolvers list is missing a provider name for [%s]",
               resolver_name);
        return -1;
    }
    if (provider_publickey_s == NULL || *provider_publickey_s == 0) {
        logger(proxy_context, LOG_ERR,
               "Resolvers list is missing a public key for [%s]",
               resolver_name);
        return -1;
    }
    if (resolver_ip == NULL || *resolver_ip == 0) {
        logger(proxy_context, LOG_ERR,
               "Resolvers list is missing a resolver address for [%s]",
               resolver_name);
        return -1;
    }
    dnssec = options_get_col(headers, headers_count,
                             cols, cols_count, "DNSSEC validation");
    if (dnssec != NULL && evutil_ascii_strcasecmp(dnssec, "yes") != 0) {
        logger(proxy_context, LOG_INFO,
               "- [%s] does not support DNS Security Extensions",
               resolver_name);
    } else {
        logger(proxy_context, LOG_INFO,
               "+ DNS Security Extensions are supported");
    }
    namecoin = options_get_col(headers, headers_count,
                               cols, cols_count, "Namecoin");
    if (namecoin != NULL && evutil_ascii_strcasecmp(namecoin, "yes") == 0) {
        logger(proxy_context, LOG_INFO,
               "+ Namecoin domains can be resolved");
    }
    nologs = options_get_col(headers, headers_count,
                             cols, cols_count, "No logs");
    if (nologs != NULL && evutil_ascii_strcasecmp(nologs, "no") == 0) {
        logger(proxy_context, LOG_WARNING,
               "- [%s] logs your activity - "
               "a different provider might be better a choice if privacy is a concern",
               resolver_name);
    } else {
        logger(proxy_context, LOG_INFO,
               "+ Provider supposedly doesn't keep logs");
    }

    proxy_context->provider_name = strdup(provider_name);
    proxy_context->provider_publickey_s = strdup(provider_publickey_s);
    proxy_context->resolver_ip = strdup(resolver_ip);
    if (proxy_context->provider_name == NULL ||
        proxy_context->provider_publickey_s == NULL ||
        proxy_context->resolver_ip == NULL) {
        logger_noformat(proxy_context, LOG_EMERG, "Out of memory");
        exit(1);
    }
    return 1;
}

static int
options_parse_resolvers_list(ProxyContext * const proxy_context, char *buf)
{
    char   *cols[OPTIONS_RESOLVERS_LIST_MAX_COLS];
    char   *headers[OPTIONS_RESOLVERS_LIST_MAX_COLS];
    size_t  cols_count;
    size_t  headers_count;

    assert(proxy_context->resolver_name != NULL);
    buf = minicsv_parse_line(buf, headers, &headers_count,
                             sizeof headers / sizeof headers[0]);
    if (headers_count < 4U || headers_count > OPTIONS_RESOLVERS_LIST_MAX_COLS) {
        return -1;
    }
    do {
        buf = minicsv_parse_line(buf, cols, &cols_count,
                                 sizeof cols / sizeof cols[0]);
        if (cols_count < 4U || cols_count > OPTIONS_RESOLVERS_LIST_MAX_COLS) {
            continue;
        }
        minicsv_trim_cols(cols, cols_count);
        if (*cols[0] == 0 || *cols[0] == '#') {
            continue;
        }
        if (options_parse_resolver(proxy_context, headers, headers_count,
                                   cols, cols_count) > 0) {
            return 0;
        }
    } while (*buf != 0);

    return -1;
}

static int
options_use_resolver_name(ProxyContext * const proxy_context)
{
    char *file_buf;
    char *resolvers_list_rebased;

    if ((resolvers_list_rebased =
         path_from_app_folder(proxy_context->resolvers_list)) == NULL) {
        logger_noformat(proxy_context, LOG_EMERG, "Out of memory");
        exit(1);
    }
    file_buf = options_read_file(resolvers_list_rebased);
    if (file_buf == NULL) {
        logger(proxy_context, LOG_ERR, "Unable to read [%s]",
               resolvers_list_rebased);
        exit(1);
    }
    assert(proxy_context->resolver_name != NULL);
    if (options_parse_resolvers_list(proxy_context, file_buf) < 0) {
        logger(proxy_context, LOG_ERR,
               "No resolver named [%s] found in the [%s] list",
               proxy_context->resolver_name, resolvers_list_rebased);
        exit(1);
    }
    free(file_buf);
    free(resolvers_list_rebased);

    return 0;
}

static int
options_use_client_key_file(ProxyContext * const proxy_context)
{
    unsigned char *key;
    char          *key_s;
    const size_t   header_len = (sizeof OPTIONS_CLIENT_KEY_HEADER) - 1U;
    size_t         key_s_len;

    if ((key_s = options_read_file(proxy_context->client_key_file)) == NULL) {
        logger_error(proxy_context, "Unable to read the client key file");
        return -1;
    }
    if ((key = sodium_malloc(header_len + crypto_box_SECRETKEYBYTES)) == NULL) {
        logger_noformat(proxy_context, LOG_EMERG, "Out of memory");
        free(key_s);
        return -1;
    }
    if (sodium_hex2bin(key, header_len + crypto_box_SECRETKEYBYTES,
                       key_s, strlen(key_s), ": -", &key_s_len, NULL) != 0 ||
        key_s_len < (header_len + crypto_box_SECRETKEYBYTES) ||
        memcmp(key, OPTIONS_CLIENT_KEY_HEADER, header_len) != 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "The client key file doesn't seem to contain a supported key format");
        sodium_free(key);
        free(key_s);
        return -1;
    }
    sodium_memzero(key_s, strlen(key_s));
    free(key_s);
    assert(sizeof proxy_context->dnscrypt_client.secretkey <=
           key_s_len - header_len);
    memcpy(proxy_context->dnscrypt_client.secretkey, key + header_len,
           sizeof proxy_context->dnscrypt_client.secretkey);
    sodium_free(key);

    return 0;
}

static int
options_apply(ProxyContext * const proxy_context)
{
    if (proxy_context->local_ip == NULL) {
        logger_noformat(proxy_context, LOG_ERR,
                        "--local-address requires a non-empty argument");
        exit(1);
    }
    if (proxy_context->client_key_file != NULL) {
        if (proxy_context->ephemeral_keys != 0) {
            logger_noformat(proxy_context, LOG_ERR,
                            "--client-key and --ephemeral-keys are mutually exclusive");
            exit(1);
        }
        if (options_use_client_key_file(proxy_context) != 0) {
            logger(proxy_context, LOG_ERR,
                   "Client key file [%s] could not be used", proxy_context->client_key_file);
            exit(1);
        }
    }
    if (proxy_context->resolver_name != NULL) {
        if (proxy_context->resolvers_list == NULL) {
            logger_noformat(proxy_context, LOG_ERR,
                            "Resolvers list (-L command-line switch) required");
            exit(1);
        }
        if (options_use_resolver_name(proxy_context) != 0) {
            logger(proxy_context, LOG_ERR,
                   "Resolver name (-R command-line switch) required. "
                   "See [%s] for a list of public resolvers.",
                   proxy_context->resolvers_list);
            exit(1);
        }
    }
    if (proxy_context->resolver_ip == NULL ||
        *proxy_context->resolver_ip == 0 ||
        proxy_context->provider_name == NULL ||
        *proxy_context->provider_name == 0 ||
        proxy_context->provider_publickey_s == NULL ||
        *proxy_context->provider_publickey_s == 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Resolver information required.");
        logger_noformat(proxy_context, LOG_ERR,
                        "The easiest way to do so is to provide a resolver name.");
        logger_noformat(proxy_context, LOG_ERR,
                        "Example: dnscrypt-proxy -R mydnsprovider");
        logger(proxy_context, LOG_ERR,
               "See the file [%s] for a list of compatible public resolvers",
               proxy_context->resolvers_list);
        logger_noformat(proxy_context, LOG_ERR,
                        "The name is the first column in this table.");
        logger_noformat(proxy_context, LOG_ERR,
                        "Alternatively, an IP address, a provider name "
                        "and a provider key can be supplied.");
#ifdef _WIN32
        logger_noformat(proxy_context, LOG_ERR,
                        "Consult https://dnscrypt.org "
                        "and https://github.com/jedisct1/dnscrypt-proxy/blob/master/README-WINDOWS.markdown "
                        "for details.");
#else
        logger_noformat(proxy_context, LOG_ERR,
                        "Please consult https://dnscrypt.org "
                        "and the dnscrypt-proxy(8) man page for details.");
#endif
        exit(1);
    }
    if (proxy_context->provider_name == NULL ||
        *proxy_context->provider_name == 0) {
        logger_noformat(proxy_context, LOG_ERR, "Provider name required");
        exit(1);
    }
    if (options_check_protocol_versions(proxy_context->provider_name) != 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Unsupported server protocol version");
        exit(1);
    }
    if (proxy_context->provider_publickey_s == NULL) {
        logger_noformat(proxy_context, LOG_ERR, "Provider key required");
        exit(1);
    }
    if (dnscrypt_fingerprint_to_key(proxy_context->provider_publickey_s,
                                    proxy_context->provider_publickey) != 0) {
        logger_noformat(proxy_context, LOG_ERR, "Invalid provider key");
        exit(1);
    }
    if (proxy_context->daemonize != 0) {
        if (proxy_context->log_file == NULL) {
            proxy_context->syslog = 1;
        }
        do_daemonize();
    }
#ifndef _WIN32
    if (proxy_context->pid_file != NULL &&
        pid_file_create(proxy_context->pid_file,
                        proxy_context->user_id != (uid_t) 0) != 0) {
        logger_error(proxy_context, "Unable to create pid file");
        exit(1);
    }
#endif
    if (proxy_context->log_file != NULL && proxy_context->syslog != 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "--logfile and --syslog are mutually exclusive");
        exit(1);
    }
    if (proxy_context->log_file != NULL &&
        (proxy_context->log_fp = fopen(proxy_context->log_file, "a")) == NULL) {
        logger_error(proxy_context, "Unable to open log file");
        exit(1);
    }
    if (proxy_context->syslog != 0) {
        assert(proxy_context->log_fp == NULL);
        logger_open_syslog(proxy_context);
    }
    return 0;
}

static void
options_strdup(ProxyContext * const proxy_context,
               char ** const resource,
               const char * const string)
{
    if (string == NULL || *string == 0) {
        if (*resource != NULL) {
            free(*resource);
        }
        *resource = NULL;
    } else {
        char *const duplicate = strdup(string);
        if (duplicate == NULL) {
            logger_noformat(proxy_context, LOG_EMERG, "Out of memory");
        } else {
            if (*resource != NULL) {
                free(*resource);
            }
            *resource = duplicate;
        }
    }
}

static int
options_parse_flag(ProxyContext * const proxy_context,
                   const int opt_flag, char * const optarg, const int boolval)
{
    switch (opt_flag) {
    case 'a':
        options_strdup(proxy_context, &proxy_context->local_ip, optarg);
        break;
    case 'd':
        proxy_context->daemonize = boolval;
        break;
    case 'e': {
        char *endptr;
        const unsigned long edns_payload_size = strtoul(optarg, &endptr, 10);

        if (*optarg == 0 || *endptr != 0 ||
            edns_payload_size > DNS_MAX_PACKET_SIZE_UDP_RECV) {
            logger(proxy_context, LOG_ERR,
                   "Invalid EDNS payload size: [%s]", optarg);
            exit(1);
        }
        if (edns_payload_size <= DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND) {
            proxy_context->edns_payload_size = (size_t) 0U;
            proxy_context->udp_max_size = DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND;
        } else {
            proxy_context->edns_payload_size = (size_t) edns_payload_size;
            assert(proxy_context->udp_max_size >=
                   DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND);
            if (proxy_context->edns_payload_size > DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND) {
                proxy_context->udp_max_size =
                    proxy_context->edns_payload_size;
            }
        }
        break;
    }
    case 'E':
        proxy_context->ephemeral_keys = boolval;
        break;
    case 'I':
        proxy_context->ignore_timestamps = boolval;
        break;
    case 'k':
        options_strdup(proxy_context, &proxy_context->provider_publickey_s,
                       optarg);
        break;
    case 'K':
        options_strdup(proxy_context, &proxy_context->client_key_file, optarg);
        break;
    case 'l':
        options_strdup(proxy_context, &proxy_context->log_file, optarg);
        break;
    case 'L':
        options_strdup(proxy_context, &proxy_context->resolvers_list, optarg);
        break;
    case 'R':
        options_strdup(proxy_context, &proxy_context->resolver_name, optarg);
        break;
#ifndef _WIN32
    case 'S':
        proxy_context->syslog = boolval;
        break;
    case 'Z':
        proxy_context->syslog = boolval;
        options_strdup(proxy_context, &proxy_context->syslog_prefix, optarg);
        break;
#endif
    case 'm': {
        char *endptr;
        const long max_log_level = strtol(optarg, &endptr, 10);

        if (*optarg == 0 || *endptr != 0 || max_log_level < 0) {
            logger(proxy_context, LOG_ERR,
                   "Invalid max log level: [%s]", optarg);
            exit(1);
        }
        proxy_context->max_log_level = max_log_level;
        break;
    }
    case 'n': {
        char *endptr;
        const unsigned long connections_count_max =
            strtoul(optarg, &endptr, 10);

        if (*optarg == 0 || *endptr != 0 ||
            connections_count_max <= 0U ||
            connections_count_max > UINT_MAX) {
            logger(proxy_context, LOG_ERR,
                   "Invalid max number of active request: [%s]", optarg);
            exit(1);
        }
        proxy_context->connections_count_max =
            (unsigned int) connections_count_max;
        break;
    }
    case 'p':
        options_strdup(proxy_context, &proxy_context->pid_file, optarg);
        break;
    case 'r':
        options_strdup(proxy_context, &proxy_context->resolver_ip, optarg);
        break;
    case 't': {
        char *endptr;
        const unsigned long margin =
            strtoul(optarg, &endptr, 10);

        if (*optarg == 0 || *endptr != 0 || margin == 0 ||
            margin > UINT32_MAX / 60U) {
            logger(proxy_context, LOG_ERR,
                   "Invalid certificate grace period: [%s]", optarg);
            exit(1);
        }
        proxy_context->test_cert_margin = (time_t) margin * (time_t) 60U;
        proxy_context->test_only = 1;
        break;
    }
#ifdef HAVE_GETPWNAM
    case 'u': {
        const struct passwd * const pw = getpwnam(optarg);
        if (pw == NULL) {
            logger(proxy_context, LOG_ERR, "Unknown user: [%s]", optarg);
            exit(1);
        }
        options_strdup(proxy_context, &proxy_context->user_name, pw->pw_name);
        proxy_context->user_id = pw->pw_uid;
        proxy_context->user_group = pw->pw_gid;
        options_strdup(proxy_context, &proxy_context->user_dir, pw->pw_dir);
        break;
    }
#endif
    case 'N':
        options_strdup(proxy_context, &proxy_context->provider_name, optarg);
        break;
    case 'T':
        proxy_context->tcp_only = boolval;
        break;
    case 'X':
#ifndef PLUGINS
        logger_noformat(proxy_context, LOG_ERR,
                        "Support for plugins hasn't been compiled in");
        exit(1);
#else
        if (plugin_options_parse_str
            (proxy_context->app_context->dcps_context, optarg) != 0) {
            logger_noformat(proxy_context, LOG_ERR,
                            "Error while parsing plugin options");
            exit(2);
        }
#endif
        break;
    default:
        options_usage();
        exit(1);
    }
}

static int
options_isspace(const char ch)
{
    return isspace((int) (unsigned char) ch);
}

static char *
options_trim(char * const string)
{
    size_t i = strlen(string);
    while (0 < i && options_isspace(string[i - 1]) != 0) {
        string[--i] = 0;
    }
    for (i = 0; string[i] != 0 && options_isspace(string[i]); ++i) {
    }
    return &string[i];
}

static void
options_config_line(ProxyContext * const proxy_context,
                    const char * const file_name, const int line_num,
                    const char * const key, char *const val)
{
    const struct option *found = NULL;
    const struct option *option = &getopt_long_options[0];

    for (; found == NULL && option->name != NULL; ++option) {
        const int flag = option->val;
        if (0 < flag && flag < 127 && flag != 'c' && flag != 'C') {
            if (strcmp(key, option->name) == 0) {
                found = option;
            }
        }
    }

    if (found == NULL) {
        logger(proxy_context, LOG_ERR,
               "Invalid configuration in [%s] at line [%d]",
               file_name, line_num);
    }
    else {
        int boolval = 0;
        if (found->has_arg == 0) {
            if (evutil_ascii_strcasecmp(val, "1") == 0 ||
                evutil_ascii_strcasecmp(val, "yes") == 0 ||
                evutil_ascii_strcasecmp(val, "true") == 0) {
                boolval = 1;
            }
            else if (evutil_ascii_strcasecmp(val, "0") != 0 &&
                     evutil_ascii_strcasecmp(val, "no") != 0 &&
                     evutil_ascii_strcasecmp(val, "false") != 0) {
                logger(proxy_context, LOG_ERR,
                       "Invalid value for [%s] in config [%s] at line [%d]",
                       key, file_name, line_num);
                exit(1);
            }
        }
        options_parse_flag(proxy_context, found->val, val, boolval);
    }
}

static void
options_config_file(ProxyContext * const proxy_context,
                    const char * const file_name)
{
    FILE       *fp;
    const int   line_len = 1024;
    char        line_buf[line_len];
    int         line_num = 1;

    assert(file_name != NULL);
    if ((fp = fopen(file_name, "rb")) == NULL) {
        logger(proxy_context, LOG_ERR, "Unable to read config [%s]", file_name);
        exit(1);
    }
    for (; fgets(line_buf, line_len, fp) != NULL; ++line_num) {
        char *trimmed = options_trim(line_buf);
        if (*trimmed && *trimmed != '#') {
            char *equals = strchr(trimmed, '=');
            if (equals == NULL) {
                logger(proxy_context, LOG_ERR, "Invalid line [%d] in config [%s]",
                       line_num, file_name);
            }
            else {
                *equals = 0;
                options_config_line(proxy_context, file_name, line_num,
                                    options_trim(trimmed),
                                    options_trim(equals + 1));
            }
        }
    }
    (void) fclose(fp);
}

static const char *
options_dump_string(const char * const string)
{
    return (string != NULL) ? string : "";
}

static const char *
options_dump_yesno(int value)
{
    return (value == 0) ? "no" : "yes";
}

static void
options_dump_config(ProxyContext * const proxy_context)
{
    printf("%-20s = %s\n", "client-key",
            options_dump_string(proxy_context->client_key_file));
    printf("%-20s = %u\n", "max-active-requests",
            proxy_context->connections_count_max);
    printf("%-20s = %s\n", "daemonize",
            options_dump_yesno(proxy_context->daemonize));
    printf("%-20s = %zu\n", "edns-payload-size",
            proxy_context->edns_payload_size);
    printf("%-20s = %s\n", "ephemeral-keys",
            options_dump_yesno(proxy_context->ephemeral_keys));
    printf("%-20s = %s\n", "ignore-timestamps",
            options_dump_yesno(proxy_context->ignore_timestamps));
    printf("%-20s = %s\n", "local-address",
            options_dump_string(proxy_context->local_ip));
    printf("%-20s = %s\n", "logfile",
            options_dump_string(proxy_context->log_file));
    printf("%-20s = %d\n", "loglevel",
            proxy_context->max_log_level);
    printf("%-20s = %s\n", "pidfile",
            options_dump_string(proxy_context->pid_file));
    printf("%-20s = %s\n", "provider-name",
            options_dump_string(proxy_context->provider_name));
    printf("%-20s = %s\n", "provider-key",
            options_dump_string(proxy_context->provider_publickey_s));
    printf("%-20s = %s\n", "resolver-address",
            options_dump_string(proxy_context->resolver_ip));
    printf("%-20s = %s\n", "resolver-name",
            options_dump_string(proxy_context->resolver_name));
    printf("%-20s = %s\n", "resolvers-list",
            options_dump_string(proxy_context->resolvers_list));
    printf("%-20s = %s\n", "syslog",
            options_dump_yesno(proxy_context->syslog));
    printf("%-20s = %s\n", "syslog-prefix",
            options_dump_string(proxy_context->syslog_prefix));
    printf("%-20s = %s\n", "tcp-only",
            options_dump_yesno(proxy_context->tcp_only));
    printf("%-20s = %ld\n", "test", proxy_context->test_only == 0 ? -1 :
            (long) (proxy_context->test_cert_margin / 60));
    printf("%-20s = %s\n", "user",
            options_dump_string(proxy_context->user_name));
}

int
options_parse(AppContext * const app_context,
              ProxyContext * const proxy_context, int argc, char *argv[])
{
    int   opt_flag;
    int   option_index = 0;
#ifdef _WIN32
    _Bool option_install = 0;
#endif
    _Bool dump_config = 0;

    options_init_with_default(app_context, proxy_context);
    while ((opt_flag = getopt_long(argc, argv,
                                   getopt_options, getopt_long_options,
                                   &option_index)) != -1) {
        switch (opt_flag) {
        case 'c':
            options_config_file(proxy_context, optarg);
            break;
        case 'C':
            dump_config = 1;
            break;
        case 'h':
            options_usage();
            exit(0);
        case 'V':
            options_version();
            exit(0);
#ifdef _WIN32
        case WIN_OPTION_INSTALL:
        case WIN_OPTION_REINSTALL:
            option_install = 1;
            break;
        case WIN_OPTION_UNINSTALL:
            if (windows_service_uninstall() != 0) {
                logger_noformat(NULL, LOG_ERR, "Unable to uninstall the service");
                exit(1);
            } else {
                logger(NULL, LOG_INFO,
                       "The [%s] service has been removed from this system",
                       get_windows_service_name());
                exit(0);
            }
            break;
        case WIN_OPTION_SERVICE_NAME:
            break;
#endif
        default:
            options_parse_flag(proxy_context, opt_flag, optarg, 1);
        }
    }
    if (dump_config == 1) {
        options_dump_config(proxy_context);
    }
    if (options_apply(proxy_context) != 0) {
        return -1;
    }
#ifdef _WIN32
    if (option_install != 0) {
        if (windows_service_install(proxy_context) != 0) {
            logger_noformat(NULL, LOG_ERR, "Unable to install the service");
            logger_noformat(NULL, LOG_ERR,
                            "Make sure that you are using an elevated command prompt "
                            "and that the service hasn't been already installed");
            exit(1);
        }
        logger(NULL, LOG_INFO,
               "The [%s] service has been installed and started",
               get_windows_service_name());
        logger(NULL, LOG_INFO, "The registry key used for this "
               "service is [%s]", windows_service_registry_parameters_key());
        logger(NULL, LOG_INFO, "Now, change your resolver settings to %s",
               proxy_context->local_ip);
        exit(0);
    }
#endif
    return 0;
}

static void
options_checked_free(char ** const strptr)
{
    if (*strptr != NULL) {
        free(*strptr);
        *strptr = NULL;
    }
}

void
options_free(ProxyContext * const proxy_context)
{
    options_checked_free(&proxy_context->client_key_file);
    options_checked_free(&proxy_context->local_ip);
    options_checked_free(&proxy_context->log_file);
    options_checked_free(&proxy_context->pid_file);
    options_checked_free(&proxy_context->provider_name);
    options_checked_free(&proxy_context->provider_publickey_s);
    options_checked_free(&proxy_context->resolver_ip);
    options_checked_free(&proxy_context->resolver_name);
    options_checked_free(&proxy_context->resolvers_list);
    options_checked_free(&proxy_context->syslog_prefix);
#ifndef _WIN32
    options_checked_free(&proxy_context->user_name);
    options_checked_free(&proxy_context->user_dir);
#endif
}
