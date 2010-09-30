node Select () => ();
node Read () => ();
node Route () => ();
node Send () => ();

node detached One () => ();
node detached Two () => ();
node detached Three () => ();
node detached Four () => ();
node detached Five () => ();

source Select -> Read -> Route -> Send;
source One -> Two -> Three -> Four -> Five;
