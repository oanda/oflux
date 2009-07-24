
(** Basic type checking for maintaining consequences of 
    the type unification that are done when semantically analyzing 
    an OFlux program *)

open ParserTypes

exception Failure of string * position
   
type unification_result = ((string * bool) * (string * bool)) list

let empty = []

let concat a b = a @ b

let type_check_general (siot,iot) unified symtable sfl_n fl_n srcpos =
	let iot_to_str iot = if iot then "input" else "output"
	in  match Unify.unify' (siot,iot) symtable sfl_n fl_n with
		(Unify.Success Unify.Strong) -> ((sfl_n,siot),(fl_n,iot))::unified
				(* true for input *)
		| (Unify.Success Unify.Weak) -> unified
		| (Unify.Fail (i,reason)) ->
			raise (if i < 0 then Failure (reason,srcpos) 
				else Failure (("Node "^(iot_to_str iot)^" "^fl_n^" and "^(iot_to_str siot)^" "
						^sfl_n^" unify failed."
						^reason^" ( argument "
						^(string_of_int i)^")"), srcpos))

let type_check unified symtable sfl_n fl_n srcpos =
	type_check_general (true,false) unified symtable sfl_n fl_n srcpos

let type_check_concur unified symtable n1 n2 p1 =
        let unified = type_check_general (true,true) unified symtable n2 n1 p1
        in  type_check_general (false,false) unified symtable n2 n1 p1

let type_check_inputs_only unified symtable n1 n2 p1 =
        type_check_general (true,true) unified symtable n1 n2 p1

let type_check_outputs_only unified symtable n1 n2 p1 =
        type_check_general (false,false) unified symtable n1 n2 p1

let from_basic_nodes symtable sinks sources =
        let ulist =
		match (List.map (fun x -> (x,false)) sinks) 
				@ (List.map (fun x -> (x,true)) sources) with
			(h::t) -> List.map (fun y ->(h,y)) t
			| _ -> []
		in
        let ensure_each n nd ul =
                if nd.SymbolTable.nodeoutputs = None then
                        ((n,true),(n,true))::ul
                else ((n,true),(n,true))::((n,false),(n,false))::ul in
        let ulist = SymbolTable.fold_nodes ensure_each symtable ulist
        in  ulist
        
type consequence_result = 
        { equiv_classes : (string * bool) list list
        ; union_map : ((string * bool) * int) list
        ; full_collapsed : (int * ((string * bool) list)) list
        ; full_collapsed_names : (string * bool) list
        ; aliases : ((string * bool) * (string * bool)) list 
                (** (X,Y) implies X is Y *)
        ; subset_order : (int * int) list
                (** (a,b) in this list means that the
                        union_map^{-1} will indicate
                    that a is a subset of b 
                    not reflexive,trans,anti-symmetric closed
                  *)
        }

let get_decl_list_from_union conseq_res symtable nn =
        let (str,io),_ = List.find (fun (_,n) -> n=nn) conseq_res.union_map in
        let nd = SymbolTable.lookup_node_symbol_from_function symtable str 
        in  if io then nd.SymbolTable.nodeinputs
            else match nd.SymbolTable.nodeoutputs with
                        None -> raise (Failure ("get_decl_list_from_union tried "^str^(if io then "_in" else "_out"),ParserTypes.noposition))
                        | (Some dl) -> dl

let get_subset_order stable full_collapsed =
        let on_each (i,ul) = 
                let h = List.hd ul in
                let decls = SymbolTable.get_decls stable h
                in  (i,decls) in
        let decl_compare decl1 decl2 =
                compare (strip_position decl1.name
                        ,strip_position decl1.ctype
                        ,strip_position decl1.ctypemod)
                        (strip_position decl2.name
                        ,strip_position decl2.ctype
                        ,strip_position decl2.ctypemod) in
        let rec list_compare l1 l2 =
                match l1, l2 with
                        (h1::t1,h2::t2) ->
                                let r = decl_compare h1 h2
                                in  if r = 0 then list_compare t1 t2
                                    else r
                        | _ -> 0 in
        let decls_compare (_,l1) (_,l2) =
                let r = compare (List.length l1) (List.length l2) 
                in  if r = 0 then list_compare l1 l2
                    else r in
        let ll_sorted = List.sort decls_compare (List.map on_each full_collapsed) in
        let rec is_subset decls1 decls2 =
                match decls1,decls2 with
                        ([],_) -> true
                        | (_,[]) -> false
                        | (h1::t1,h2::t2) ->
                                let cr = decl_compare h1 h2
                                in  if cr = 0 then is_subset t1 t2
                                    else if cr < 0 then false
                                    else is_subset decls1 t2 in
        let find_a_subset (i,decls) sofarlist =
                try let j,decls = List.find (fun (j,sdecls) -> is_subset sdecls decls) sofarlist
                    in  Some (j,i)
                with Not_found -> None in
        let ffun (subset_relation, sofar) (i,decls) =
                ((match find_a_subset (i,decls) sofar with
                        None -> subset_relation
                        |(Some (a,b)) -> (a,b)::subset_relation)
                ,(i,decls)::sofar) in
        let subset_rel,_ = List.fold_left ffun ([],[]) ll_sorted
        in  subset_rel
                        
                

let get_collapsed_types stable ufs umap =
	let is_equal x1 x2 = 
		try let l = List.combine (SymbolTable.get_decls stable x1) 
				(SymbolTable.get_decls stable x2) 
		    in  List.for_all 
				(fun (df1,df2) ->
					(((strip_position df1.ctypemod) = (strip_position df2.ctypemod))
					&&
					((strip_position df1.ctype) = (strip_position df2.ctype))
					&&
					((strip_position df1.name) = (strip_position df2.name))))
				l
		with (Invalid_argument _) -> false
		in
	let do_one' ul =
		let rec d_o ll u =
			match ll with
				(((hh::_) as h)::t) -> 
					if is_equal hh u then
						(u::h)::t
					else h::(d_o t u)
				| _ -> [u]::ll
		in List.fold_left d_o [] ul in
	let get_aliases ll =
                let alias h y = 
                        let as_string (s,io) = s^(if io then "_in" else "_out") in
                        let _ = Debug.dprint_string ("get_aliases: "^(as_string y)^" := "^(as_string h)^"\n")
                        in (y,h) in
		match ll with
			[] -> []
			| (h::t) -> List.map (alias h) t in
	let do_one (full_collapsed_i,full_collapsed_ns,aliases) ul =
		let i = List.assoc (List.hd ul) umap
		in  match do_one' ul with
			[singlel] -> ((i,ul)::full_collapsed_i, ul @ full_collapsed_ns, aliases)
			| resl -> (full_collapsed_i,full_collapsed_ns,List.concat (List.map get_aliases resl) @ aliases)
	in  List.fold_left do_one ([],[],[]) ufs

let get_umap ufs =  
        let do_one (umap,i) ul =
                ( (List.map (fun y -> (y,i)) ul) @ umap, i+1 ) in
        let umap,_ = List.fold_left do_one ([],1) ufs
        in  umap


let finish_consequences stable ufs =
        let umap = get_umap ufs in
        let full_collapsed, full_collapsed_names, aliases =
                get_collapsed_types stable ufs umap in
        let subset_order = get_subset_order stable full_collapsed
        in  { equiv_classes=ufs
            ; union_map=umap
            ; full_collapsed=full_collapsed
            ; full_collapsed_names=full_collapsed_names
            ; aliases=aliases
            ; subset_order=subset_order
            }

let consequences ulist stable = 
	let get_io_type cls =
		let git (nf,isin) = 
			try let nd = SymbolTable.lookup_node_symbol_from_function stable nf
			    in  (if isin 
				 then Some nd.SymbolTable.nodeinputs 
				 else nd.SymbolTable.nodeoutputs )
			with _ -> None in
		let rec git_cls cls =
			match cls with
				(h::t) -> (match git h with
						None -> git_cls t
						| (Some r) -> r)
				| _ -> raise Not_found in
		let dfl = git_cls cls in
		let strip_pos x = ParserTypes.strip_position x in
		let strip_each df =
			(strip_pos df.ctypemod)
			^(strip_pos df.ctype)
			(*^(strip_pos df.name)*)
		in  (List.map strip_each dfl, cls) in
	let compress_by_type_definition ufs =
		let annotated_ufs = List.map get_io_type ufs in
		let compare_a_ufs (a,_) (b,_) = compare a b in
		(*let annotated_ufs = List.sort compare_a_ufs annotated_ufs in*)
		let annotated_ufs = 
			let merge (a,al) (b,bl) = (a,al @ bl)
			in  Uniquify.uniq compare_a_ufs merge annotated_ufs 
		in  List.map (fun (_,x) -> x) annotated_ufs in
        let union_find_uniq ufs =
                let rec uniq ll ul =
                        match ul with
                                (h::t) -> if List.mem h ll then uniq ll t
                                        else uniq (h::ll) t
                                | _ -> ll
                        in
                let oneach_cls cl = uniq [] cl
                in  List.map oneach_cls ufs
                in
        let translate_node_to_function stable ulist =
                let to_func (n,boolval) =
                        try let nd = SymbolTable.lookup_node_symbol stable n
                            in  nd.SymbolTable.functionname, boolval
                        with _ ->  (n,boolval)
                in  List.map (fun (x,y) -> (to_func x,to_func y)) ulist
                in
	let ulist = translate_node_to_function stable ulist in
        let ufs = UnionFind.union_find ulist in  
	(*let ufs = compress_by_type_definition ufs in -- this is too aggressive*)
        let ufs = union_find_uniq ufs 
        in  finish_consequences stable ufs

        
let get_union_from_strio conseq_r strio = List.assoc strio conseq_r.union_map

let get_union_from_equiv conseq_r ul = 
        get_union_from_strio conseq_r (List.hd ul)

let consequences_equiv_fold ffun onobj conseq_res =
        List.fold_left ffun onobj conseq_res.equiv_classes

let consequences_umap_fold ffun onobj conseq_res =
        List.fold_left ffun onobj conseq_res.union_map

let make_compatible stable_change conseq_const conseq_change =
        let ufs_const = conseq_const.equiv_classes in
        let ufs = conseq_change.equiv_classes in
        let intersects ec1 ec2 = List.mem (List.hd ec1) ec2 in
        let partfor ec ufs =
                match List.partition (intersects ec) ufs with
                        ([fd],rem) -> fd,rem
                        | _ -> raise Not_found in
        let ec_tostring ec =
                match ec with  
                        ((s,b)::_) -> s^(if b then "_in" else "_out")
                        | _ -> "<empty>" in
        let rec compat ufs_const ufs =
                match ufs_const, ufs with
                        ([],_) -> ufs
                        | (h::t,_) ->
                                (try let n,ufs = partfor h ufs
                                    in n::(compat t ufs)
                                with Not_found ->
                                        raise (Failure ("make_compatible failure for class containing "^(ec_tostring h),noposition)))
        in  finish_consequences stable_change (compat ufs_const ufs)


