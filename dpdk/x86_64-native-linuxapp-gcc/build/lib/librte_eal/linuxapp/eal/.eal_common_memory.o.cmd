cmd_eal_common_memory.o = gcc -Wp,-MD,./.eal_common_memory.o.d.tmp -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3  -I/home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include -include /home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -I/home/rami/Teridion/dpdk/lib/librte_eal/linuxapp/eal/include -I/home/rami/Teridion/dpdk/lib/librte_eal/common -I/home/rami/Teridion/dpdk/lib/librte_eal/common/include -I/home/rami/Teridion/dpdk/lib/librte_ring -I/home/rami/Teridion/dpdk/lib/librte_mempool -I/home/rami/Teridion/dpdk/lib/librte_malloc -I/home/rami/Teridion/dpdk/lib/librte_ether -I/home/rami/Teridion/dpdk/lib/librte_ivshmem -I/home/rami/Teridion/dpdk/lib/librte_pmd_ring -I/home/rami/Teridion/dpdk/lib/librte_pmd_pcap -I/home/rami/Teridion/dpdk/lib/librte_pmd_af_packet -I/home/rami/Teridion/dpdk/lib/librte_pmd_xenvirt -W -Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -O3  -O0 -g -o eal_common_memory.o -c /home/rami/Teridion/dpdk/lib/librte_eal/common/eal_common_memory.c 
