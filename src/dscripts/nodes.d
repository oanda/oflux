#!/usr/sbin/dtrace -s

#pragma D option quiet

/* declare total_shim_time so that I can do += on it */
self uint64 total_shim_time;

oflux$1:::event-born
{
    event_born[arg0] = timestamp;
}

oflux$1:::node-acquireguards
{
    node_start_guard_acquisition[arg0] = timestamp;
}

oflux$1:::node-haveallguards
/ node_start_guard_acquisition[arg0] /
{
    @time_node_took_to_acquire_all_guards[copyinstr(arg1)] =
        quantize((timestamp - node_start_guard_acquisition[arg0]) / 1000);

    node_start_guard_acquisition[arg0] = 0;
}

oflux$1:::fifo-push
{
    node_begin_wait_on_runtime_queue[arg0] = timestamp;
}

oflux$1:::fifo-pop
/ node_begin_wait_on_runtime_queue[arg0] /
{
    @time_node_waited_on_runtime_queue[copyinstr(arg1)] =
        quantize((timestamp - node_begin_wait_on_runtime_queue[arg0]) / 1000);

    node_begin_wait_on_runtime_queue[arg0] = 0;
}

oflux$1:::node-start
/ event_born[arg0] /
{
    node_start[arg0] = timestamp;

    self->is_detached = (int)arg3;

    /* 
     * I feel dirty doing this - but this is the only way
     * to prevent a memory leak if shim-call triggers
     */
    self->node_started = 1;

    @time_from_event_born_to_node_start[copyinstr(arg1)] =
        quantize((node_start[arg0] - event_born[arg0]) / 1000);
}

/* I don't like using self->node_started here but I have to */
oflux$1:::shim-call
/ self->node_started /
{
    self->shim_start = timestamp;
}

oflux$1:::shim-return
/ self->shim_start /
{
    self->total_shim_time += (timestamp - self->shim_start);
}

oflux$1:::node-done
/ node_start[arg0] /
{
    node_done[arg0] = timestamp;
}

oflux$1:::node-done
/ node_start[arg0] && !self->is_detached /
{
    @node_runtime_while_not_detached_and_holding_runtime_lock[copyinstr(arg1)] =
        avg(((node_done[arg0] - node_start[arg0]) - self->total_shim_time) / 1000);
}

/* if node_start[arg0] exists then shim-call and shim-return would have happened if shim calls existed */
oflux$1:::node-done
/ node_start[arg0] /
{
    self->runtime_time = node_done[arg0] - node_start[arg0];

    @node_runtime_including_shim_calls[copyinstr(arg1)] =
        quantize(self->runtime_time / 1000);

    @node_runtime_excluding_shim_calls[copyinstr(arg1)] =
        quantize((self->runtime_time - self->total_shim_time) / 1000);

    @node_runtime_while_holding_runtime_lock[copyinstr(arg1)] =
        avg((self->runtime_time - self->total_shim_time) / 1000);

    @time_from_event_born_to_node_done[copyinstr(arg1)] =
        avg((node_done[arg0] - event_born[arg0]) / 1000);

    @node_execution_count[copyinstr(arg1)] = count();

    self->runtime_time = 0;
    self->total_shim_time = 0;
    self->shim_start = 0;
    self->node_started = 0;
    self->is_detached = 0;

    node_start[arg0] = 0;
}

/* have to put condition in because nodes have started before my script runs */
oflux$1:::event-death
/ node_done[arg0] && event_born[arg0] /
{
    @time_from_node_done_to_event_death[copyinstr(arg1)] =
        quantize((timestamp - node_done[arg0]) / 1000);

    @event_lifetime[copyinstr(arg1)] =
        quantize((timestamp - event_born[arg0]) / 1000);

    node_done[arg0] = 0;
    event_born[arg0] = 0;
}

:::END
{
    printf("Node Execution Count:\n");
    printf("===============================================================================");
    printa(@node_execution_count);

    printf("\n");

    printf("Average Time From Event Born To Node Completion:\n");
    printf("===============================================================================");
    printa(@time_from_event_born_to_node_done);

    printf("\n");

    printf("Average Node Runtime For Non-Detached Nodes While Holding Runtime Lock:\n");
    printf("===============================================================================");
    printa(@node_runtime_while_not_detached_and_holding_runtime_lock);

    printf("\n");

    printf("Average Node Runtime For All Nodes:\n");
    printf("===============================================================================");
    printa(@node_runtime_while_holding_runtime_lock);

    printf("\n");

    printf("Node Time Breakdown (by node):\n");
    printf("===============================================================================");
    printf("\n");

    printf("time_from_event_born_to_node_start\n");
    printf("time_node_took_to_acquire_all_guards\n");
    printf("time_node_waited_on_runtime_queue\n");
    printf("node_runtime_including_shim_calls\n");
    printf("node_runtime_excluding_shim_calls\n");
    printf("time_from_node_done_to_event_death\n");
    printf("event_lifetime\n");

    printf("\n");

    /*
     * [sigh]
     */
    printa("%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\ntime_from_event_born_to_node_start %@a\ntime_node_took_to_acquire_all_guards %@a\ntime_node_waited_on_runtime_queue %@a\nnode_runtime_including_shim_calls %@a\nnode_runtime_excluding_shim_calls %@a\ntime_from_node_done_to_event_death %@a\nevent_lifetime %@a\n",
           @time_from_event_born_to_node_start,
           @time_node_took_to_acquire_all_guards,
           @time_node_waited_on_runtime_queue,
           @node_runtime_including_shim_calls,
           @node_runtime_excluding_shim_calls,
           @time_from_node_done_to_event_death,
           @event_lifetime);
}
