exclusive Ga () => int *;
exclusive Gb (int v) => int *;

node S () => (int a, int b);
node NS (int a, int b, guard Gb(a) as ab, guard Gb(b) as bb) => ();
/*node NS (int a, int b, guard Gb(b) as bb) => ();*/

source S -> NS;
