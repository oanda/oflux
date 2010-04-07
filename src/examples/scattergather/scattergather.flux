/** demonstrate scatter gather in oflux */

node Foo () => (int d, oflux::Scatter<GatherObj> so);

node detached Bar 
	( int d /* index of sorts */
	, oflux::Scatter<GatherObj> so)
	=>
	( oflux::Scatter<GatherObj> so);

node abstract BarAbstract 
	( int d
	, oflux::Scatter<GatherObj> so) 
	=> 
	...;

node abstract CollectAbstract (oflux::Scatter<GatherObj> so) => ();

node Collect (oflux::Scatter<GatherObj> so) => ();

condition isLast (const oflux::Scatter<GatherObj> & so) => bool;

initial Foo -> BarAbstract;

BarAbstract = Bar -> CollectAbstract;

CollectAbstract : [ isLast ] = Collect;
/* default to no successor */
