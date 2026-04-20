#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_function.h>
#include <amxd/amxd_transaction.h>
#include <amxo/amxo.h>
#include <amxb/amxb.h>
#include <amxb/amxb_register.h>

/* -------------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------------- */
static volatile sig_atomic_t g_running    = 1;
static volatile int          g_mode       = 0; /* 0=idle 1=cpu 2=mem */
#define MAX_CPU_THREADS 8
static pthread_t             g_stress_tids[MAX_CPU_THREADS];
static int                   g_stress_nthreads = 0;
static void                 *g_mem_block  = NULL;

static amxd_dm_t      g_dm;
static amxo_parser_t  g_parser;
static amxb_bus_ctx_t *g_bus_ctx = NULL;

/* -------------------------------------------------------------------------
 * Signal handler
 * ------------------------------------------------------------------------- */
static void sig_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) g_running = 0;
}

/* -------------------------------------------------------------------------
 * Stress thread
 * ------------------------------------------------------------------------- */
static void *cpu_stress_thread(void *arg) {
    (void)arg;
    while (g_mode == 1) { /* busy loop */ }
    return NULL;
}

static void *mem_stress_thread(void *arg) {
    (void)arg;
    const size_t sz = 600UL * 1024 * 1024; /* 600 MB — ~15% of 4 GB */
    g_mem_block = malloc(sz);
    if (g_mem_block) {
        memset(g_mem_block, 0xAA, sz);  /* touch every page */
        syslog(LOG_INFO, "test-service: MemStress allocated 600 MB");
    } else {
        syslog(LOG_WARNING, "test-service: MemStress malloc failed");
    }
    while (g_mode == 2) sleep(1);
    free(g_mem_block);
    g_mem_block = NULL;
    syslog(LOG_INFO, "test-service: MemStress released memory");
    return NULL;
}

static void start_stress(int mode) {
    /* Stop any running stress threads */
    g_mode = 0;
    for (int i = 0; i < g_stress_nthreads; i++)
        pthread_join(g_stress_tids[i], NULL);
    g_stress_nthreads = 0;

    if (mode == 0) return;

    g_mode = mode;
    if (mode == 1) {
        /* One busy-loop thread per CPU core */
        long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
        if (ncpu < 1) ncpu = 1;
        if (ncpu > MAX_CPU_THREADS) ncpu = MAX_CPU_THREADS;
        syslog(LOG_INFO, "test-service: CPU stress started (%ld threads)", ncpu);
        for (long i = 0; i < ncpu; i++)
            pthread_create(&g_stress_tids[g_stress_nthreads++], NULL, cpu_stress_thread, NULL);
    } else if (mode == 2) {
        pthread_create(&g_stress_tids[g_stress_nthreads++], NULL, mem_stress_thread, NULL);
    }
}

/* -------------------------------------------------------------------------
 * Data model helpers
 * ------------------------------------------------------------------------- */
static void dm_set_string(const char *param, const char *value) {
    amxd_object_t *obj = amxd_dm_findf(&g_dm, "TestService.");
    if (!obj) return;
    amxd_trans_t trans;
    amxc_var_t val;
    amxd_trans_init(&trans);
    amxd_trans_set_attr(&trans, amxd_tattr_change_ro, true);
    amxd_trans_select_object(&trans, obj);
    amxc_var_init(&val);
    amxc_var_set(cstring_t, &val, value);
    amxd_trans_set_param(&trans, param, &val);
    amxd_trans_apply(&trans, &g_dm);
    amxd_trans_clean(&trans);
    amxc_var_clean(&val);
}

static void dm_set_uint32(const char *param, uint32_t value) {
    amxd_object_t *obj = amxd_dm_findf(&g_dm, "TestService.");
    if (!obj) return;
    amxd_trans_t trans;
    amxc_var_t val;
    amxd_trans_init(&trans);
    amxd_trans_set_attr(&trans, amxd_tattr_change_ro, true);
    amxd_trans_select_object(&trans, obj);
    amxc_var_init(&val);
    amxc_var_set(uint32_t, &val, value);
    amxd_trans_set_param(&trans, param, &val);
    amxd_trans_apply(&trans, &g_dm);
    amxd_trans_clean(&trans);
    amxc_var_clean(&val);
}

/* -------------------------------------------------------------------------
 * RPC handlers
 * ------------------------------------------------------------------------- */

/* ubus call TestService SetMode '{"mode":"CPUStress"}' */
/* ubus call TestService SetMode '{"mode":"MemStress"}' */
/* ubus call TestService SetMode '{"mode":"Idle"}'      */
amxd_status_t ts_set_mode(amxd_object_t *obj, amxd_function_t *fn,
                           amxc_var_t *args, amxc_var_t *ret) {
    (void)obj; (void)fn; (void)ret;
    const char *mode_str = GET_CHAR(args, "mode");
    if (!mode_str) return amxd_status_invalid_value;

    int mode = 0;
    if (strcasecmp(mode_str, "CPUStress") == 0)      mode = 1;
    else if (strcasecmp(mode_str, "MemStress") == 0) mode = 2;

    start_stress(mode);
    dm_set_string("Mode", mode == 1 ? "CPUStress" : mode == 2 ? "MemStress" : "Idle");
    syslog(LOG_INFO, "test-service: mode -> %s", mode_str);
    return amxd_status_ok;
}

/* ubus call TestService Crash '{}' */
amxd_status_t ts_crash(amxd_object_t *obj, amxd_function_t *fn,
                        amxc_var_t *args, amxc_var_t *ret) {
    (void)obj; (void)fn; (void)args; (void)ret;
    syslog(LOG_WARNING, "test-service: Crash() called — exiting to test hgw-doctor restart");
    exit(1);
}

/* -------------------------------------------------------------------------
 * main()
 * ------------------------------------------------------------------------- */
int main(void) {
    openlog("test-service", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "test-service starting");

    signal(SIGTERM, sig_handler);
    signal(SIGINT,  sig_handler);

    amxd_dm_init(&g_dm);
    amxo_parser_init(&g_parser);
    amxo_parser_parse_file(&g_parser,
                           "/etc/amx/test-service/test-service.odl",
                           amxd_dm_get_root(&g_dm));

    /* Bind RPC implementations programmatically (ODL has no shared-lib import) */
    amxd_object_t *ts_obj = amxd_dm_findf(&g_dm, "TestService.");
    if (ts_obj) {
        amxd_function_t *fn_set_mode = amxd_object_get_function(ts_obj, "SetMode");
        amxd_function_t *fn_crash    = amxd_object_get_function(ts_obj, "Crash");
        if (fn_set_mode) amxd_function_set_impl(fn_set_mode, ts_set_mode);
        if (fn_crash)    amxd_function_set_impl(fn_crash,    ts_crash);
        syslog(LOG_INFO, "test-service: RPCs bound (SetMode=%p Crash=%p)",
               (void *)fn_set_mode, (void *)fn_crash);
    } else {
        syslog(LOG_WARNING, "test-service: TestService object not found in DM");
    }

    amxb_be_load("/usr/bin/mods/amxb/mod-amxb-ubus.so");
    if (amxb_connect(&g_bus_ctx, "ubus:/var/run/ubus/ubus.sock") == 0) {
        amxb_register(g_bus_ctx, &g_dm);
        syslog(LOG_INFO, "test-service registered on ubus");
    }

    syslog(LOG_INFO, "test-service running");

    time_t start_time = time(NULL);
    while (g_running) {
        if (g_bus_ctx != NULL) {
            int fd = amxb_get_fd(g_bus_ctx);
            if (fd >= 0) {
                fd_set rfds;
                struct timeval tv = {1, 0};
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);
                if (select(fd + 1, &rfds, NULL, NULL, &tv) > 0)
                    amxb_read(g_bus_ctx);
            }
        } else {
            sleep(1);
        }
        amxp_signal_read();
        dm_set_uint32("UptimeSeconds", (uint32_t)(time(NULL) - start_time));
    }

    g_mode = 0;
    for (int i = 0; i < g_stress_nthreads; i++) pthread_join(g_stress_tids[i], NULL);
    syslog(LOG_INFO, "test-service stopped");
    if (g_bus_ctx) { amxb_disconnect(g_bus_ctx); amxb_free(&g_bus_ctx); }
    amxb_be_remove_all();
    amxo_parser_clean(&g_parser);
    amxd_dm_clean(&g_dm);
    closelog();
    return 0;
}
