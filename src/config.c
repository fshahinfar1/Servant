#include <stdio.h>
#include <stdlib.h> // exit, atoi
#include <math.h>
#include <errno.h>
#include <getopt.h>
#include <linux/if_xdp.h> // XDP_ZEROCOPY
#include <linux/if_link.h>

#include "config.h"
#include "log.h"

#define REQUIRED_ARGUMENTS 3
 
struct config config = {};

void usage(char *prog_name)
{
    const char desc[] = ("Usage: %s [--Options] <ifname> <qid> <eBPF>\n"
            "*\t ifname:  name of interface to attach to\n"
            "*\t qid:     number of queue to attach to\n"
            "*\t eBPF:    path to eBPF program\n"
            "Options:\n"
            "\t num-frames, frame-size, batch-size,rx-size, tx-size,\n"
            "\t copy-mode, skb-mode, rps, no-jit\n"
            );
    printf(desc, prog_name);
}
 

void parse_args(int argc, char *argv[])
{
    // Default Values
    config.frame_size = 4096;
    config.frame_shift = log2(config.frame_size);
    config.headroom = 0;
    config.busy_poll = 0;
    config.busy_poll_duration = 50;
    config.batch_size = 64;
    config.terminate = 0;
    config.rx_size = 2048;
    config.tx_size = 1024;
    config.copy_mode = XDP_ZEROCOPY;
    config.xdp_mode = XDP_FLAGS_DRV_MODE;
    // config.copy_mode = XDP_COPY;
    config.jitted = 1;
    config.custom_kern_prog = 0;
    config.custom_kern_path = NULL;

    enum opts {
        NUM_FRAMES = 100,
        FRAME_SIZE,
        BATCH_SIZE,
        RX_SIZE,
        TX_SIZE,
        COPY_MODE,
        SKB_MODE,
        NO_JIT,
    };
    struct option long_opts[] = {
        {"num-frames", required_argument, NULL, NUM_FRAMES},
        {"frame-size", required_argument, NULL, FRAME_SIZE},
        {"batch-size", required_argument, NULL, BATCH_SIZE},
        {"rx-size", required_argument, NULL, RX_SIZE},
        {"tx-size", required_argument, NULL, TX_SIZE},
        {"copy-mode", no_argument, NULL, COPY_MODE},
        {"skb-mode", no_argument, NULL, SKB_MODE},
        {"no-jit", no_argument, NULL, NO_JIT},
    };
    int ret;
    char optstring[] = "";
    while (1) {
        ret = getopt_long(argc, argv, optstring, long_opts, NULL);
        if (ret == -1)
            break;
        switch (ret) {
            case NUM_FRAMES:
                // config.num_frames = atoi(optarg);
                INFO("Number of frames is determined automatically\n");
                break;
            case FRAME_SIZE:
                config.frame_size = atoi(optarg);
                config.frame_shift = log2(config.frame_size);
                break;
            case BATCH_SIZE:
                config.batch_size = atoi(optarg);
                break;
            case RX_SIZE:
                config.rx_size = atoi(optarg);
                break;
            case TX_SIZE:
                config.tx_size = atoi(optarg);
                break;
            case COPY_MODE:
                config.copy_mode = XDP_COPY;
                break;
            case SKB_MODE:
                config.xdp_mode = XDP_FLAGS_SKB_MODE;
                break;
            case NO_JIT:
                config.jitted = 0;
                break;
            default:
                ERROR("Unknown: argument!\n");
                exit(EXIT_FAILURE);
                break;
        }
    }
    if (argc - optind < REQUIRED_ARGUMENTS) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    config.ifname = argv[optind];
    config.ifindex = if_nametoindex(config.ifname);
    if(config.ifindex < 0) {
        ERROR("interface %s not found (%d)\n",
                config.ifname, config.ifindex);
        exit(EXIT_FAILURE);
    }
    optind++;
    config.qid = atoi(argv[optind]);
    optind++;
    config.ebpf_program_path = argv[optind];
    optind++;

    // How many descriptors are needed
    config.num_frames = (config.rx_size + config.tx_size) * 4;

    if(config.busy_poll){
        INFO("BUSY POLLING\n");
    }
    if (config.copy_mode == XDP_ZEROCOPY) {
        INFO("Running in ZEROCOPY mode!\n");
    } else if (config.copy_mode == XDP_COPY) {
        INFO("Running in COPY mode!\n");
    } else {
        ERROR( "AF_XDP mode was not detected!\n");
        exit(EXIT_FAILURE);
    }
    if (config.xdp_mode == XDP_FLAGS_DRV_MODE) {
      INFO("Running XDP in NATIVE mode\n");
    } else if (config.xdp_mode == XDP_FLAGS_SKB_MODE) {
      INFO("Running XDP in SKB mode\n");
    } else {
      ERROR("Unexpected XDP mode\n");
      exit(EXIT_FAILURE);
    }
    INFO("Batch Size: %d\n", config.batch_size);
    INFO("Rx Ring Size: %d\n", config.rx_size);
    INFO("Tx Ring Size: %d\n", config.tx_size);
}
