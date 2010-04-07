free Ga (int b) => FList *;

node S () => (int b);
node detached NS (int b, guard Ga(b) as f) => ();

source S -> NS;
