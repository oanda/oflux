include testm.flux

node Start () => (std::string txt);
node End (std::string txt) => ();

instance testm ggg;

source Start -> ggg.In;
ggg.Out = End;
