
/*
 * The story of an event:
 *
 *  1.  it is event__born
 *  2.  it makes at least one attempt to node__acquireguards:
 *  2.1 each guard it succeeds on it does a guard__acquire
 *  2.2 if one guard is unavailable it will guard__wait
 *  3.  once it has all of its guards it will node__haveallguards
 *  4.  it will pass through the fifo 
 *  4.1 fifo__push marks its entry
 *  4.2 fifo__pop marks its exit
 *  5.  node__start tells us the node function is about to be run
 *  6.  if the node is not detached, then we may see shimmed calls(may repeat):
 *  6.1 shim__call interposed system call entry
 *  6.2 shim__wait waiting to rejoin runtime (syscall finished)
 *  6.3 shim__return interposed system call exit
 *  7.  node__done tells us the node function has returned
 *  8.  event_death tells us the event has been collected 
 *      (its successors will keep it alive until they are done with it)
 *
 * Some of these probe points must be grouped onto the same 
 * thread.  Those groups are:
 *
 *     {1,2} {2} {2,3,4.1} {4.2,5,6,7} {8}
 *
 * It is possible that the same event might only be touched by 1 thread
 */

provider oflux {
        probe event__born(
                  void *  /*event ptr*/
                , char *  /*node name*/
                );
        probe event__death(
                  void *  /*event ptr*/
                , char *  /*node name*/
                );

        probe node__acquireguards(
                  void *  /*event ptr*/
                , char *  /*node name*/
                );
        probe node__haveallguards(
                  void *  /*event ptr*/
                , char *  /*node name*/
                );
        probe node__start(
                  void *  /*event ptr*/
                , char *  /*node name*/
                , int     /*is source*/
                , int     /*is detached*/
                );
        probe node__done(
                  void *  /*event ptr*/
                , char *  /*node name*/
                );

	probe fifo__push(
                  void *  /*event ptr*/
                , char *  /*node name*/
                );
	probe fifo__pop (
                  void *  /*event ptr*/
                , char *  /*node name*/
                );

	probe guard__wait(
                  char *  /*guard name*/
                , char *  /*node name*/ 
                , int     /*wtype*/
                );
	probe guard__acquire(
                  char *  /*guard name*/
                , char *  /*node name*/  
                , int     /*is passed*/
                );
	probe guard__release(
                  char *  /*guard name*/ 
                , char *  /*first event name*/ 
                , int     /*num events released*/
                );

        /*
         * Exception-related probes
         */

        probe aha__exception__begin__throw(
                  int /*guard num*/
                );
};
