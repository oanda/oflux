(** all digraphs are 1. forests or 2. have a strong cycle *) 

type 'a tree = T of 'a * 'a tree list ref

let new_tree i = T (i,ref [])

let add_succ (T (_,tl)) trsucc = tl := trsucc::(!tl)

let add_or_create tmap i =
	try tmap, List.assoc i tmap
	with Not_found ->
		let nt = new_tree i
		in  (i,nt)::tmap, nt

let rec has x (T (y,tl)) = (x=y) || (List.exists (has x) (!tl))

exception Cycle

let treeify edgelist =
	let tfy tmap (x,y) =
		let tmap, xt = add_or_create tmap x in
		let tmap, yt = add_or_create tmap y in
		let _ = if has x yt then raise Cycle else () in
		let _ = add_succ xt yt
		in tmap
	in  List.fold_left tfy [] edgelist

let detect edgelist =
        try  let _ = treeify edgelist
             in  false
        with Cycle -> true




