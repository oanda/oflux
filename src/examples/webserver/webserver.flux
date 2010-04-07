/* Your basic web server flow */

node Page (int socket) => ();

node ReadRequest (int socket) => (int socket, bool close, const char* request);

node Reply (int socket, bool close, int length, const char* content, const char* output) => ();

node ReadWrite (int socket, bool close, const char* file)
     => (int socket, bool close, int length, const char* content, const char* output);

node Listen () => (int socket);

node FourOhF (int socket, bool close, const char* file) => ();

node BadRequest (int socket) => ();

source Listen -> Page;

Page = ReadRequest -> ReadWrite -> Reply;

handle error ReadWrite -> FourOhF;

handle error ReadRequest -> BadRequest;

