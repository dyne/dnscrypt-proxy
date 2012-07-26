
#ifndef __APP_H__
#define __APP_H__ 1

typedef struct AppContext_ {
    struct ProxyContext_ *proxy_context;
} AppContext;

int dnscrypt_proxy_main(int argc, char *argv[]);
int dnscrypt_proxy_loop_break(void);

#endif
