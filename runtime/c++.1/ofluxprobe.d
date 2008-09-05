
provider oflux {
        probe node__start(char *,int,int);
	                /*node name, is source, is detached*/
        probe node__done(char *);
	                /*node name*/

	probe shim__call(char *);
	                /*syscall name*/
	probe shim__wait(char *);
	                /*syscall name*/
	probe shim__return(char *);
	                /*syscall name*/

	probe fifo__push(char *);
	                /*event name*/
	probe fifo__pop(char *);
	                /*event name*/

	probe guard__wait(char *, char *, int);
	                /*guard name, event name, wtype*/
	probe guard__acquire(char *, char *, int);
	                /*guard name, event name, is passed*/
	probe guard__release(char *, char *, int);
	                /*guard name, first event name, num events released*/
};
