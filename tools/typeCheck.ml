
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
		Unify.Success -> ((sfl_n,siot),(fl_n,iot))::unified
				(* true for input *)
		| (Unify.Fail (i,reason)) ->
			raise (if i < 0 then Failure (reason,srcpos) 
				else Failure (("Node "^(iot_to_str iot)^" "^fl_n^" and "^(iot_to_str siot)^" "
						^sfl_n^" unify failed."
						^reason^" ( argument "
						^(string_of_int i)^")"), srcpos))

let type_check unified symtable sfl_n fl_n srcpos =
	type_check_general (true,false) unified symtable sfl_n fl_n srcpos

let type_check_concur unified symtable n1 n2 p1 =
        let unified = type_check_general (true,true) unified
                symtable n1 n2 p1
        in  type_check_general (false,false) unified symtable n1 n2 p1

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
        }


let consequences ulist stable = 
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
        let ufs = union_find_uniq ufs in
        let get_umap ufs =  
                let do_one (umap,i) ul =
                        ( (List.map (fun y -> (y,i)) ul) @ umap, i+1 ) in
                let umap,_ = List.fold_left do_one ([],1) ufs
                in  umap
        in  { equiv_classes=ufs
            ; union_map=get_umap ufs
            }

        
let get_union_from_strio conseq_r strio = List.assoc strio conseq_r.union_map

let get_union_from_equiv conseq_r ul = 
        get_union_from_strio conseq_r (List.hd ul)

let consequences_equiv_fold ffun onobj conseq_res =
        List.fold_left ffun onobj conseq_res.equiv_classes

let consequences_umap_fold ffun onobj conseq_res =
        List.fold_left ffun onobj conseq_res.union_map



