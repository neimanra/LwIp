cmd_testacl = gcc -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3  -I/home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include -include /home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -W -Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings  -Wl,-Map=testacl.map,--cref -o testacl main.o -Wl,--no-as-needed -Wl,-export-dynamic -L/home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/lib  -L/home/rami/Teridion/dpdk/x86_64-native-linuxapp-gcc/lib -Wl,--whole-archive -Wl,-lintel_dpdk -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive 
