#include <stdio.h>
#include <unistd.h>

struct sys_config {
	int check_interval;
	char log_path[64];
};

struct sys_config global_config = {60, "/var/log/sysmon.log"};	// .data section
int active_connections;						// .bss section

void
print_system_stats(const char *module_name)
{
	// module_name is passed in rdi (x86-64 calling convention)
	printf("[%s] System is running smoothly. Connections: %d\n", module_name, active_connections);
	printf("[%s] Logging to: %s\n", module_name, global_config.log_path);
}

int
main(int argc, char **argv)
{
	active_connections = 5;
	print_system_stats("CoreMonitor");
	return 0;
}
