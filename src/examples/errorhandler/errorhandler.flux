
node Node1 () => (int arg);
node Node2 (int arg) => ();
node Error (int arg) => ();
node Error1 (int arg) => (int arg);
node Error2 (int arg) => ();

source Node1 -> Node2;

Error = Error1 -> Error2;

handle error Node2 -> Error;