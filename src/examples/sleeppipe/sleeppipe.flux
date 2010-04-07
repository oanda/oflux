

/* 
   Idea with this is to create a pipe of nodes with sleeps and without
*/

node Start () => (int transaction_number, mtime_t start_t);

node MainFlow (int transaction_number, mtime_t start_t) =>  ();

node Sleep (int transaction_number, mtime_t start_t) => 
(int transaction_number, mtime_t start_t, mtime_t sleep_t, long sleep_req);

node detached Fibonacci (int transaction_number, mtime_t start_t, 
	mtime_t sleep_t, long sleep_req) =>
	(int transaction_number, mtime_t start_t, 
		mtime_t sleep_t, long sleep_req, mtime_t cpu_t);

node Done (int transaction_number, mtime_t start_t,
	mtime_t sleep_t, long sleep_req, mtime_t cpu_t) => ();

source Start -> MainFlow;

MainFlow = Sleep -> Fibonacci -> Done;


