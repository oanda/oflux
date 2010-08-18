exclusive Gb (int v) => int *; 
  /* remove the /gc to cause this program to leak memory */

node S () => (int a);
node NS (int a, guard Gb(a) as ab) => (int a); /* populates the rval guard */
node NSS (int a, guard Gb(a) as ab) => (); /* depopulates the rval guard */

node abstract N(int a) => ...;

source S -> N;
N = NS -> NSS;
