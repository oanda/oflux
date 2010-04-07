readwrite og1(int id) => int *;

node zRequestSrc () => ( int id );
node abstract zHandleRequest ( int id ) => ...;
node zExecute ( int id ) => (int id ); /* default handler inside kernal */
node Sink ( int id ) => ();

source zRequestSrc -> zHandleRequest;

zHandleRequest = zExecute -> Sink;

/* upto the plugin to connect to Sink */
