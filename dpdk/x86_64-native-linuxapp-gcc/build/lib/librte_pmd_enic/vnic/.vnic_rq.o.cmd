cmd_vnic/vnic_rq.o = gcc -Wp,-MD,vnic/.vnic_rq.o.d.tmp -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3  -I/home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include -include /home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -I/home/rami/Teridion/dpdk/lib/librte_pmd_enic/vnic/ -I/home/rami/Teridion/dpdk/lib/librte_pmd_enic/ -O3 -W -Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wno-strict-aliasing  -O0 -g -o vnic/vnic_rq.o -c /home/rami/Teridion/dpdk/lib/librte_pmd_enic/vnic/vnic_rq.c 
