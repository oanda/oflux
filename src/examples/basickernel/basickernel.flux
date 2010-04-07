
node Foo () => (int a);
node abstract FooSucc (int a) => ...;
node Toast () => ();

source Foo -> FooSucc;

terminate FooSucc;
