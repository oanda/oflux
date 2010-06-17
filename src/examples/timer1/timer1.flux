
include Timer.flux

instance Timer t;

node detached Generator1 ( guard t.State() as state) => ();
node detached Generator2 ( guard t.State() as state) => ();

node detached ExpiredReporter (EventBasePtr timer) => ();
node detached CancelledReporter (EventBasePtr timer) => ();

/*
 * flow
 */

source Generator1;
source Generator2;

t.OutExpired = ExpiredReporter;
t.OutCancelled = CancelledReporter;

