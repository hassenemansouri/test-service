#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <amxc/amxc.h>
#include <amxp/amxp.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxo/amxo.h>
#include <amxb/amxb.h>
#include <amxb/amxb_register.h>

static volatile int g_running = 1;
static amxd_dm_t g_dm;
static amxo_parser_t g_parser;
static amxb_bus_ctx_t *g_bus_ctx = NULL;

static void sig_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) g_running = 0;
}

int main(void) {
    openlog("test-service", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Test service starting");

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    amxd_dm_init(&g_dm);
    amxo_parser_init(&g_parser);
    amxo_parser_parse_file(&g_parser, "/etc/amx/test-service/test-service.odl", amxd_dm_get_root(&g_dm));

    amxb_be_load("/usr/bin/mods/amxb/mod-amxb-ubus.so");
    if (amxb_connect(&g_bus_ctx, "ubus:/var/run/ubus/ubus.sock") == 0) {
        amxb_register(g_bus_ctx, &g_dm);
        syslog(LOG_INFO, "Test service registered on ubus");
    }

    syslog(LOG_INFO, "Test service running");

    while (g_running) {
        int fd = amxb_get_fd(g_bus_ctx);
        if (fd >= 0) {
            fd_set rfds;
            struct timeval tv = {1, 0};
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            if (select(fd + 1, &rfds, NULL, NULL, &tv) > 0)
                amxb_read(g_bus_ctx);
        }
        amxp_sigmngr_handle(NULL);
        syslog(LOG_DEBUG, "Test service alive");
    }

    syslog(LOG_INFO, "Test service stopped");
    if (g_bus_ctx) { amxb_disconnect(g_bus_ctx); amxb_free(&g_bus_ctx); }
    amxb_be_remove_all();
    amxo_parser_clean(&g_parser);
    amxd_dm_clean(&g_dm);
    closelog();
    return 0;
}
