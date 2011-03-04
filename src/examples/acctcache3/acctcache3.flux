
exclusive account_lock (int account) => Account *;

node detached Start () => (int account);

node detached Finish (int account, guard account_lock(account)) => ();

source Start -> Finish;


