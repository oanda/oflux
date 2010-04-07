/** chain the guards in a single node 
 * (this technique will need precendence and conditional guards)
 */

readwrite Ga (int a) => Aob *;

readwrite Gb (int b) => Bob *;

condition isNotNull(Aob *) => bool;

node S () => (int a);

/*precedence Gb < Ga;
 * -- uncommenting the above should cause load errors
 *    since Ga < Gb is implied by the below guard chain
 */

node detached N 
	( int a
	, guard Ga/write(a) as ga
	, guard Gb/write(ga->id()) as gb if isNotNull(ga)
	) 
	=>
	();

source S -> N;
