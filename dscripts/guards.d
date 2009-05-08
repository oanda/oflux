:::guard-acquire
{
	g = copyinstr(arg0);
	@acquires[g] = count();
	acquire_starts[g] = timestamp;
}

:::guard-release
{
	g = copyinstr(arg0);
	@acquire_times[g] = avg(timestamp - acquire_starts[g]);
}

:::guard-wait
{
	g = copyinstr(arg0);
	@waits[g] = count();
}

:::node-acquireguards
{
	g = copyinstr(arg0);
	node_acquire_starts[g] = timestamp;
}

:::node-haveallguards
{
	g = copyinstr(arg0);
	@node_acquire_times[g] = avg(timestamp - node_acquire_starts[g]);
}

tick-1s
{
	printf("\n");
	printa("guard %s: acquire %@d times %@d nsecs in avg, wait %@d times\n", @acquires, @acquire_times, @waits);
	printa("node %s: wait %@d nsecs in avg to acquire guards\n", @node_acquire_times);
}
