
exclusive Last () => size_t *;

/********************************************
 * nodes for main flow
 *******************************************/

node NumberGenerator
    ()
    =>
    (size_t val);

node Check
    (size_t val, guard Last())
    =>
    ();

/********************************************
 * nodes for secondary flow
 *******************************************/
 
node Delay
    ()
    =>
    ();

node AlsoGuard
    (guard Last())
    =>
    ();

source NumberGenerator -> Check;
source Delay -> AlsoGuard;