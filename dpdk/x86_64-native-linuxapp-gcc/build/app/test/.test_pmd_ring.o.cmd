cmd_test_pmd_ring.o = gcc -Wp,-MD,./.test_pmd_ring.o.d.tmp -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3  -I/home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include -include /home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -O3 -W -Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -D_GNU_SOURCE  -O0 -g -o test_pmd_ring.o -c /home/rami/Teridion/dpdk/app/test/test_pmd_ring.c 
