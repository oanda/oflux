(* used to make a list of things unique *)

let uniq comp resolv ll =
	let rec uniq' ll =
		match ll with
			(h1::h2::t) ->
				if (comp h1 h2) = 0 then
					uniq' ((resolv h1 h2)::t)
				else h1::(uniq' (h2::t))
			| _ -> ll in
	let lls = List.sort comp ll 
	in  uniq' lls

let uniq_keep_order comp resolv ll =
	let rec within wl x =
		match wl with 
			(h::t) -> 
				if (compare h x) = 0 then (resolv h x)::t
				else h::(within t x)
			| _ -> [x] in
	let rec uniq' sofar ll =
		match ll with
			(h::t) -> uniq' (within sofar h) t
			| _ -> ll
	in  List.rev (uniq' [] ll)

let uniq_discard comp ll = 
	let discard a _ = a
	in  uniq comp discard ll

let uniq_append comp ll =
	let append a b = a @ b
	in  uniq comp append ll
	
