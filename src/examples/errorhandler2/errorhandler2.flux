/* use error handlers with io conversion! */
node Node1 () => (C arg, int foo, int bar);
node Node2 (C arg, int foo) => ();
node Error (C arg, int foo) => ();

node B () => ();
node A () => ();

source Node1 -> Node2;

source B -> A;

handle error Node2 -> Error;
