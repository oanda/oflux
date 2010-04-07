/* demonstrate I/O conversion */

node A () => (int a, char b, long c);

node B (int a) => (int d, char e);

node C (char e) => ();

node D (long c) => ();

node BD (int a,long c) => ();

node Band (int a) => ();

source A -> BD;

BD = Band & D;

Band = B -> C;

/* should do an error handler too */
