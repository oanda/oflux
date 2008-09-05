(* pretty printing C++ish code *)

type code = string list * int (* list of lines, indent level *)

let empty_code = ([],0)

let add_code (cdl,ind) str =
	let rec indent n = 
		if n > 0 then "    "^(indent (n-1)) else "" in
	let should_find ch str =
		try let rind = String.rindex str ch
		    in  rind + 2 >= (String.length str)
		with Not_found -> false in
	let should_indent str = should_find '{' str in
	let should_undent str = should_find '}' str in
	let max a b = if a > b then a else b in
	let ind = if should_undent str then max 0 (ind-1) else ind
	in  (((indent ind)^str)::cdl,if should_indent str then ind+1 else ind)

let add_cr (cdl,ind) = (("")::cdl,ind)

let to_string (cdl,_) =
	let t_s r s = (s^"\n"^r)
	in  List.fold_left t_s "" cdl

