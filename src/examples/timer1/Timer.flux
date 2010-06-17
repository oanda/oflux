
static module Timer
 begin

 /*
  * guards
  */

 free State () => Timer::State *;

 /*
  * internal nodes:
  */

 node Expire_splay
	( guard State() as state )
	=>
	( EventBasePtr timer 
	, bool is_cancelled );

 node abstract ExpiredAndCancelled
	( EventBasePtr timer 
	, bool is_cancelled )
	=>
	...;


 /*
  * input nodes: (none)
  */

 /*
  * output nodes:
  */
 node abstract OutExpired 
	( EventBasePtr timer) 
	=> ...; /* hook */

 node abstract OutCancelled
	( EventBasePtr timer)
	=> ...; /* hook */

 condition isTrue (bool a) => bool;

 /*
  * flow
  */

 source Expire_splay -> ExpiredAndCancelled;

 ExpiredAndCancelled : [ *, isTrue ] = OutCancelled;
 ExpiredAndCancelled : [ *, * ] = OutExpired;

 end
