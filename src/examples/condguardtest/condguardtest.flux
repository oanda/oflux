exclusive Ga () => int *;
exclusive Gb (int v) => int *;

/*condition C_(int b1,int b2) => bool;*/

node S (guard Ga()) => (int b);
node NS (int b, guard Gb(b+1) if C_(b,b) ) => ();

source S -> NS;
