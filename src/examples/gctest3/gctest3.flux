readwrite gG () => void *; /* used to protect a walk of g */
readwrite/gc g (int a) => int *;

node SInsert () => (int a);
node DoInsert 
	( int a
	, guard gG/read() /* used to protect g */
	, guard g/write(a)
	) => 
	();

precedence gG < g; 

node SRemove () => ( int b );
node FindRemove 
	( int b
	, guard gG/write() /* so we can walk g */
	) => 
	( int a );
node DoRemove
	( int a
	, guard gG/read() /* used to protect g */
	, guard g/write(a)
	) =>
	();


source SInsert -> DoInsert;
source SRemove -> FindRemove -> DoRemove;
