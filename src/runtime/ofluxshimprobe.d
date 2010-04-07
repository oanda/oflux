
provider oflux {

	probe shim__call(
                  char *  /*syscall name*/
                );
	probe shim__wait(
                  char *  /*syscall name*/
                );
	probe shim__return(
                  char *  /*syscall name*/
                );

};
