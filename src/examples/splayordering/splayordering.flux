
readwrite Last () => size_t *;
pool First() => size_t *;

/********************************************
 * nodes for main flow
 *******************************************/

node detached NumberGenerator_splay
    ( guard First() )
    =>
    ( size_t val);

node detached Check
    ( size_t val
    , guard Last/write()
    , guard First() )
    =>
    ();

precedence Last < First;

/********************************************
 * nodes for secondary flow
 *******************************************/
 
node Delay
    ()
    =>
    ();

node AlsoGuard
    (guard Last/write())
    =>
    ();

source NumberGenerator_splay -> Check;
source Delay -> AlsoGuard;
