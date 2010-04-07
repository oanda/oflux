
exclusive account_lock (int account) => Account *;

node Start () => (int account);

node Finish (int account, guard account_lock(account)) => ();

source Start -> Finish;


