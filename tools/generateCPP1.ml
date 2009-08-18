(* generation of structs *)

open SymbolTable
open CodePrettyPrinter
open ParserTypes

let has_dot n =
	try let _ = String.index n '.'
	    in true
	with Not_found -> false

let string_replace (from_c,to_c) str =
	let s = String.copy str in  
	let rec s_r str =
		try let ind = String.index str from_c
		    in  s_r (String.set str ind to_c; str)
		with Not_found -> str 
	in  s_r s

let string_has_prefix str poss_prefix =
        let len = String.length poss_prefix
        in  if (String.length str) >= len then (String.sub str 0 len) = poss_prefix else false

let remove_prefix prefix =
        let prefix_len = String.length prefix in
        let remove n =
                let len = String.length n
                in  if string_has_prefix n prefix then
                        String.sub n prefix_len (len-prefix_len)
                    else n
        in remove

let clean_dots str = string_replace ('.','_') str

let get_umap ufs =
	let do_one (umap,i) ul =
		( (List.map (fun y -> (y,i)) ul) @ umap, i+1 ) in
	let umap,_ = List.fold_left do_one ([],1) ufs
	in  umap

exception CppGenFailure of string

let break_namespaced_name nsn = Flatten.break_namespaced_name nsn

let has_namespaced_name nsn =
	match break_namespaced_name nsn with
		[_] -> false
		| _ -> true

let rec cstyle_name nsn_broken =
	match nsn_broken with
		[] -> ""
		| (h::t) -> 
			let sres = cstyle_name t in
			let sres = if String.length sres = 0 then sres else "__"^sres
			in  h^sres

let rec expand_namespace_decl_start code nsn_broken =
	match nsn_broken with
		[] -> code
		| (nsh::nst) ->
			add_code code ("namespace "^nsh^" {")

let rec expand_namespace_decl_end code nsn_broken =
	match nsn_broken with
		[] -> code
		| (nsh::nst) ->
			List.fold_left add_code code [ "}"; "   //namespace" ]

let get_decls stable (name,isin) = SymbolTable.get_decls stable (name,isin)

(*
let metadeclbase s = 
        if has_namespaced_name s then "" 
        else "struct "^s^"meta { enum { depth = 0 }; }; "
let metadeclfrom s f = 
        if has_namespaced_name s then "" 
        else "struct "^s^"meta { enum { depth = "^f^"meta::depth +1 }; }; "
let metadeclofu s = 
        if has_namespaced_name s then "" 
        else "struct "^s^"meta { enum { depth = -1 }; }; "
*)
let metadeclbase s = ""
let metadeclfrom s f = ""
let metadeclofu s = ""

let struct_decl nclss =
        (nclss^" : public oflux::BaseOutputStruct<"^nclss^"> ")

let base_type_typedef_for conseq_res sio =
        let i = TypeCheck.get_union_from_strio conseq_res sio in
        let nclss = "OFluxUnion"^(string_of_int i)
        in  "typedef "^nclss^" base_type;"

let omit_emit_for pluginopt n =
	match pluginopt with
		None -> (has_dot n) || ((List.length(break_namespaced_name n)) != 1) 
		| (Some pn) -> 
			(not (string_has_prefix n (pn^".")))
			||
			(let nopre = remove_prefix (pn^".") n in has_dot nopre)

let print_aliases ll =
	let tos (n,isin)= if isin then n^"_in" else n^"_out" in
	let tos_a (a,b) = ("("^(tos a)^","^(tos b)^")") in
	let pp_a al = print_string ("  "^(tos_a al)^"\n")
	in  (print_string "aliases :\n";
		List.iter pp_a ll;
		())

let print_full_collapsed fcll =
	let tos (n,isin)= if isin then n^"_in" else n^"_out" in
	let pp_s x = print_string ((tos x)^", ") in
	let pp_fc (i,ll) =
		(print_string ("  "^(string_of_int i)^" --> ");
		List.iter pp_s ll;
		print_string "\n")
	in  (print_string "full_collapsed :\n"; List.iter pp_fc fcll)

let emit_structs conseq_res symbol_table code =
	let avoids = conseq_res.TypeCheck.full_collapsed_names 
                @ (List.map (fun (x,_) -> x) conseq_res.TypeCheck.aliases) in
	let avoid_struct ((n,_) as x) = (List.exists (fun y -> x=y) avoids)
		|| (has_dot n)
		in
	let rec e_one i ll =
		match ll with
			(df::tl) ->
				let tm = strip_position df.ctypemod in
				let t = strip_position df.ctype in
				let a = strip_position df.name in
				let a = if String.length a = 0 then
						"arg"^(string_of_int i)
					else a
				in  (tm^" "^t^" "^a^";")::(e_one (i+1) tl)
			| _ -> []
			in
	let e_s n nd code =
                let inpl = nd.nodeinputs in
                let code =
                        if avoid_struct (n,true) then code
                        else    List.fold_left add_code code
                                ( [ "struct "^(struct_decl (n^"_in"))^" {"
                                  ]
                                @ [base_type_typedef_for conseq_res (n,true)]
                                @ (e_one 1 inpl)
                                @ [ "};"
                                  ; metadeclbase (n^"_in")
                                  ; "" ] )
                in  match nd.nodeoutputs with
		    None -> code
		    | (Some outl) ->
			if avoid_struct (n,false) then code
			else List.fold_left add_code code
					( [ "struct "^(struct_decl (n^"_out"))^" {" ]
                                        @ [base_type_typedef_for conseq_res (n,false)]
					@ (e_one 1 outl)
					@ [ "};"
                                          ; metadeclbase (n^"_out")
                                          ; "" ] )
		in
	let code = SymbolTable.fold_nodes e_s symbol_table code in
	let tos (n,isin)= if isin then n^"_in" else n^"_out" in
	let emit_type_def_alias code ((n,_) as is,what) =
		if has_namespaced_name n then
			code
		else let whats = tos what in
                     let iss = tos is in
                        List.fold_left add_code code 
                        [ "typedef "^whats^" "^iss^"; // emit_type_def_alias (1)"
                        ; metadeclfrom whats iss ] in
	let emit_type_def_union (code,ignoreis) (i,ul) =
	    try let nsnio = List.find (fun (x,_) -> has_namespaced_name x) ul in
		let tosnsnio = tos nsnio in
		let typedeffun s = ("typedef "^tosnsnio^" "^s^"; // emit_type_def_union (1)") in
		let typedeflist =
                        let onlyon =List.filter (fun (yn,_) -> not (has_namespaced_name yn)) ul
                        in  (List.map (fun y -> typedeffun (tos y)) onlyon)
                            @ (List.map (fun y -> metadeclfrom (tos y) tosnsnio) onlyon)
				 in
		let typedeflist = 
                        let tname = "OFluxUnion"^(string_of_int i)
                        in  (typedeffun tname)::(metadeclfrom tname tosnsnio)::typedeflist
		in  (List.fold_left add_code code typedeflist,i::ignoreis)
	    with Not_found ->
                let ofluxunion_i = "OFluxUnion"^(string_of_int i) in
		let one_typedef u = 
			("typedef "^ofluxunion_i
			^" "^(tos u)^"; // emit_type_def_union (2)") in
		let un,uisin = List.hd ul in
		let rec e_two i decl =
			match decl with
				(df::tl) ->
					let tm = strip_position df.ctypemod in
					let t = strip_position df.ctype in
					let a = strip_position df.name 
					in  ("inline "^tm^" "^t^" const & argn"
						^(string_of_int i)^"() const {"
						^" return "^a^"; }  ")
						::(e_two (i+1) tl)
				| _ -> [] in
		let decl = get_decls symbol_table (un,uisin)
		in  List.fold_left add_code code 
			( [ "struct "
			    ^(struct_decl ofluxunion_i)
			    ^" {" ]
                        @ ["typedef "^ofluxunion_i^" base_type;"]
			@ (e_one 1 decl)
			@ (e_two 1 decl)
			@ [ "};"
                          ; metadeclofu ofluxunion_i
                          ; "" ]
			@ (List.map one_typedef ul)
			@ (List.map (fun x -> metadeclbase (tos x)) ul)
			@ [ "" ] ) 
		   , ignoreis in
	(*let _ = (print_aliases conseq_res.TypeCheck.aliases;
		print_full_collapsed conseq_res.TypeCheck.full_collapsed) in*)
	let code = List.fold_left emit_type_def_alias code conseq_res.TypeCheck.aliases
	in  List.fold_left emit_type_def_union (code,[]) conseq_res.TypeCheck.full_collapsed

let emit_plugin_structs pluginname conseq_res symbol_table code =
        let plugin_double_colon = pluginname^"::" in
        let plugin_dot = pluginname^"." in
        let remove_plugin_double_colon = remove_prefix plugin_double_colon in
	let tos (n,isin) = 
                let n = remove_plugin_double_colon n
                in  if isin then n^"_in" else n^"_out" in
	let avoids = conseq_res.TypeCheck.full_collapsed_names 
                @ (List.map (fun (x,_) -> x) conseq_res.TypeCheck.aliases) in
	let avoid_struct ((n,_) as x) = (List.exists (fun y -> x=y) avoids)
		|| not (List.length (break_namespaced_name 
			(remove_prefix plugin_double_colon n)) = 1)
		in
	let rec e_one i ll =
		match ll with
			(df::tl) ->
				let tm = strip_position df.ctypemod in
				let t = strip_position df.ctype in
				let a = strip_position df.name in
				let a = if String.length a = 0 then
						"arg"^(string_of_int i)
					else a
				in  (tm^" "^t^" "^a^";")::(e_one (i+1) tl)
			| _ -> []
			in
	let e_s n nd code =
		let nop_n = remove_prefix plugin_dot n in
		let _ = Debug.dprint_string ("emit_plugin_struct.e_s considering "^n^"\n") in
                let inpl = nd.nodeinputs in
                let code =
                        if avoid_struct (nd.functionname,true) then 
				let _ = Debug.dprint_string (" emit_plugin_struct avoiding "^n^"_in\n")
				in  code
                        else    List.fold_left add_code code
                                ( [ "struct "^(struct_decl (nop_n^"_in"))^" {"
                                  ]
                                @ [base_type_typedef_for conseq_res (nd.functionname,true)]
                                @ (e_one 1 inpl)
                                @ [ "};"
                                  ; metadeclbase (nop_n^"_in")
                                  ; "" ] )
                in  match nd.nodeoutputs with
		    None -> 
			let _ = Debug.dprint_string (" emit_plugin_struct - no outputs\n")
			in code
		    | (Some outl) ->
			if avoid_struct (nd.functionname,false) then 
				let _ = Debug.dprint_string (" emit_plugin_struct avoiding "^n^"_out\n")
				in code
			else List.fold_left add_code code
					( [ "struct "^(struct_decl (nop_n^"_out"))^" {" ]
                                        @ [base_type_typedef_for conseq_res (nd.functionname,false)]
					@ (e_one 1 outl)
					@ [ "};"
                                          ; metadeclbase (nop_n^"_out")
                                          ; "" ] )
		in
	let code = SymbolTable.fold_nodes e_s symbol_table code in
	let emit_type_def_alias code ((n,_) as is,what) =
		if string_has_prefix n pluginname then
                        List.fold_left add_code code 
                                [ "typedef "^(tos what)^" "^(tos is)^"; // emit_type_def_alias (2)"
                                ; metadeclfrom (tos is) (tos what) ]
		else code in
	let emit_type_def_union (code,ignoreis) (i,ul) =
	    try let nsnio = List.find (fun (x,_) -> not (string_has_prefix x (pluginname^"::"))) ul in
		let tosnsnio = tos nsnio in
		let typedeffun s = ("typedef "^tosnsnio^" "^s^"; // emit_type_def_union (3)") in
		let typedeflist =
                        let onlyon =List.filter (fun (yn,_) -> (string_has_prefix yn (pluginname^"::"))) ul
                        in  (List.map (fun y -> typedeffun (tos y)) onlyon)
                            @ (List.map (fun y -> metadeclfrom (tos y) tosnsnio) onlyon)
				 in
		let typedeflist = 
                        let tname = "OFluxUnion"^(string_of_int i)
                        in  (typedeffun tname)::(metadeclfrom tname tosnsnio)::typedeflist
		in  (List.fold_left add_code code typedeflist,i::ignoreis)
	    with Not_found ->
		let one_typedef u = 
			("typedef OFluxUnion"^(string_of_int i)
			^" "^(tos u)^"; // emit_type_def_union (4)") in
		let un,uisin = List.hd ul in
		let rec e_two i decl =
			match decl with
				(df::tl) ->
					let tm = strip_position df.ctypemod in
					let t = strip_position df.ctype in
					let a = strip_position df.name 
					in  ("inline "^tm^" "^t^" const & argn"
						^(string_of_int i)^"() const {"
						^" return "^a^"; }  ")
						::(e_two (i+1) tl)
				| _ -> [] in
                let ofluxunion_i = "OFluxUnion"^(string_of_int i) in
		let decl = get_decls symbol_table (un,uisin)
		in  List.fold_left add_code code 
			( [ "struct "
			    ^(struct_decl ofluxunion_i)
			    ^" {" ]
                        @ ["typedef "^ofluxunion_i^" base_type;"]
			@ (e_one 1 decl)
			@ (e_two 1 decl)
			@ [ "};"
                          ; metadeclofu ofluxunion_i
                          ; "" ]
			@ (List.map one_typedef ul)
			@ (List.map (fun x -> metadeclbase (tos x)) ul)
			@ [ "" ] ) 
		   , ignoreis in
	(*let _ = (print_aliases conseq_res.TypeCheck.aliases;
		print_full_collapsed conseq_res.TypeCheck.full_collapsed) in*)
        let code = List.fold_left emit_type_def_alias code conseq_res.TypeCheck.aliases
        in  List.fold_left emit_type_def_union (code,[]) conseq_res.TypeCheck.full_collapsed

let emit_union_forward_decls conseq_res symtable code =
        let collt_i = conseq_res.TypeCheck.full_collapsed in
	let avoid_is = List.map (fun (i,_) -> i) collt_i in
	let avoid_union i = List.mem i avoid_is in
	let e_union code ul =
		if ul = [] then code
		else
		let i = TypeCheck.get_union_from_equiv conseq_res ul
		in  if avoid_union i then code
		    else add_code code ( "union OFluxUnion"^(string_of_int i)^";" ) in
	let code = TypeCheck.consequences_equiv_fold e_union code conseq_res
	in  code

let emit_unions nsopt conseq_res symtable code =
	let break_ns_name_and_shorten nn =
		match nsopt,break_namespaced_name nn with
			(Some ns, h::t) -> if h = ns then t else h::t
			| (_,ll) -> ll in
	let remove_nsopt s =
		match nsopt with
			(Some ns) -> remove_prefix (ns^"::") s
			| _ -> s in
        let collt_i = conseq_res.TypeCheck.full_collapsed in
	let avoid_is = List.map (fun (i,_) -> i) collt_i in
	let avoid_union i = List.mem i avoid_is in
	let e_one (fl_n,is_in) =
		let iostr = (if is_in then "_in" else "_out") in
		let cname = cstyle_name (break_ns_name_and_shorten fl_n)
		in  ((remove_nsopt fl_n)
			^iostr^" _"^cname^iostr^";") in
	let e_union code ul =
		let i = TypeCheck.get_union_from_equiv conseq_res ul
		in  if avoid_union i then code
		    else 
			let nn,nn_isinp = List.hd ul in
			let cname = cstyle_name (break_ns_name_and_shorten nn) in
			let iostr = if nn_isinp then "_in" else "_out" in
			let decll = get_decls symtable (nn,nn_isinp) in
			let gen_mem_fun (j,codelist) df =
				let tm = strip_position df.ctypemod in
				let t = strip_position df.ctype in
				let a = strip_position df.name in
				let a = if String.length a = 0 then
						"arg"^(string_of_int j)
					else a
				in  (j+1,("inline "^tm^" "^t^" const & argn"
					^(string_of_int j)^"() const { return "
					^"_"^cname^iostr^"."^a^"; }  ")::codelist)
				in
			let _,mem_funs = List.fold_left gen_mem_fun (1,[]) decll in
			let code = List.fold_left add_code code
				( [ "union OFluxUnion"^(string_of_int i)^" {" ]
				@ (List.map e_one ul)
				@ mem_funs
				@ [ "};"
                                  ; metadeclofu ("OFluxUnion"^(string_of_int i))
                                  ; "" ] )
			in  code
			in
	let code = TypeCheck.consequences_equiv_fold e_union code conseq_res
	in  code

let uses_all um n = 
        let rec fixedpointfun (fp,modified) umlocal =
                match umlocal with 
                        ((a,b)::t) ->
                                fixedpointfun
                                (if (List.mem a fp) && not (List.mem b fp) then
                                        (b::fp,true)
                                else (fp,modified)) t
                        | [] -> if modified then fixedpointfun (fp,false) um
                                else fp,false
                in 
        let fp,_ = fixedpointfun ([n],false) um
        in  fp

let generic_cross_equiv_weak_unify code code_assignopt_fun symtable conseq_res uses_model thisname =
        (*let collapsed_types_i = conseq_res.TypeCheck.full_collapsed in
        let aliased_types = conseq_res.TypeCheck.aliases in
	let avoid ll i = 
		try let _ = List.find (fun (ii,_) -> i=ii) ll
		    in true
		with Not_found -> false in
	let avoid_n x = avoid aliased_types x in*)
        let compatible_type d1 d2 =
                (strip_position d1.ctypemod
                ,strip_position d1.ctype)
                = 
                (strip_position d2.ctypemod
                ,strip_position d2.ctype)
                in
        let decl_compare d1 d2 = 
                compare (strip_position d1.ctypemod
                        ,strip_position d1.ctype
                        ,strip_position d1.name)
                        (strip_position d2.ctypemod
                        ,strip_position d2.ctype
                        ,strip_position d2.name) in
        let assignment_for_decl d1 d2 =
                ("to->"^(strip_position d1.name)^" = "
                ^"from->"^(strip_position d2.name)^";") in
        let decl_mem x l = List.exists (fun d -> 0 = (decl_compare d x)) l in
        let rec matchtypes assignments t_decls f_decls =
                match t_decls,f_decls with
                        ([],_) -> Some assignments
                        | (_,[]) -> None
                        | (ht::tt,hf::tf) ->
                                if ht = hf then 
                                        matchtypes ((assignment_for_decl ht hf)::assignments) tt tf
                                else if (not (decl_mem ht tf)) && (compatible_type ht hf) then
                                        matchtypes ((assignment_for_decl ht hf)::assignments) tt tf
                                else (let cr = decl_compare ht hf
                                     in if cr < 0 then (* ht < hf *)
                                            None
                                        else (* ht > hf *)
                                            matchtypes assignments t_decls tf)
                in
        let get_copy_code t_name f_name =
                let get_decls' y = List.sort decl_compare (get_decls symtable y) in 
                let t_decls = get_decls' t_name in
                let f_decls = get_decls' f_name 
                in  if (List.length t_decls) > (List.length f_decls) then None
                    else matchtypes [] t_decls f_decls
                in
        let primary_name ec =
                try let ecfilt = ec (*List.filter (fun (a,b) -> has_namespaced_name a) ec in*) in
                    let score (a,b) = 
                        let modname = 
                                match break_namespaced_name a with
                                        (h::_::_) -> h
                                        | [_] -> thisname
                                        | _ -> raise Not_found
                                in
                        let sc = List.length (uses_all uses_model modname) in
                        let _ = Debug.dprint_string ("primary score ("^modname^") ="^(string_of_int sc)^"\n")
                        in  sc, (a,b) in
                    let rec findmin mopt ll =
                        match mopt,ll with
                                (_,[]) -> mopt
                                | (None,(s,x)::t) -> findmin (Some (s,x)) t
                                | (Some (s',x'),(s,x)::t) -> 
                                        findmin (if compare (s,x) (s',x') < 0 then Some (s,x) else mopt) t
                    in  match findmin None (List.map score ecfilt) with
                            None -> raise Not_found
                            | (Some (_,res)) -> res
                with Not_found -> List.hd (List.sort compare ec) in
        let primary_name ec =
                let as_string (s,io) = s^(if io then "_in" else "_out") in
                let p_string x = Debug.dprint_string ((as_string x)^",") in
                let pn = primary_name ec
                in  ( Debug.dprint_string "primary choice [ "
                    ; List.iter p_string ec
                    ; Debug.dprint_string " ] ---> "
                    ; p_string pn
                    ; Debug.dprint_string "\n" ); pn in
                (*let as_string (s,io) = s^(if io then "_in" else "_out") in
                let h = List.hd ec in
                let r = try List.assoc h conseq_res.TypeCheck.aliases
                        with Not_found -> h in  
                let _ = Debug.dprint_string ("primary_name: "^(as_string h)^" -> "^(as_string r)^"\n")
                in  r
                in*)
        let equivc = List.map primary_name conseq_res.TypeCheck.equiv_classes in
        let cross_no_equal ll1 ll2 =
                let rec mfun ll x = 
                        match ll with 
                                (h::t) -> if x = h then mfun t x else (h,x)::(mfun t x)
                                | [] -> []
                in  List.concat (List.map (mfun ll1) ll2) in
        let code_assignopt_fun' code (a,b) = code_assignopt_fun code (get_copy_code a b,a,b) in
        let code = List.fold_left code_assignopt_fun' code (cross_no_equal equivc equivc)
        in  code



let determine_redundant_for_copyto omitname um =
        let um = List.filter (fun (x,_) -> not (x=omitname)) um in
        let modlist = Uniquify.uniq_discard compare ((List.map (fun (x,_) -> x) um) @ (List.map (fun (_,x) -> x) um)) in
        let bigmodel = List.map (fun m -> uses_all um m) modlist in
        let determine a b =
                List.exists (fun us -> (List.mem a us) && (List.mem b us)) bigmodel
        in determine
        
        

let emit_copy_to_functions pluginopt modulenameopt ignoreis conseq_res symtable uses_model code =
        (*let is_module =
                match modulenameopt with
                        None -> false
                        | _ -> true in*)
        let code = List.fold_left add_code code
                        [ "namespace oflux {  "
                        ; ""
                        ; "#ifndef OFLUX_COPY_TO_GENERAL"
                        ; "#define OFLUX_COPY_TO_GENERAL"
                        ; "template<typename T,typename F>"
                        (*; "template<typename T,typename F, int tdepth, int fdepth>"*)
                        ; "inline void copy_to(T *to, const F * from)"
                        ; "{"
                        ; "enum { copy_to_code_failure_bug = 0 } _an_enum;"
                        ; "CompileTimeAssert<copy_to_code_failure_bug> cta;"
                        ; "}"
                        ; "#endif // OFLUX_COPY_TO_GENERAL"
                        ; "" ] in
	let nspace =
		match modulenameopt,pluginopt with
			(Some nn,_) -> nn^"::"
			| _ -> ""
		in
        (*let equiv_classes = UnionFind.union_find 
                (match modulenameopt with 
                        (Some nn) -> List.filter (fun (a,b) -> not (nn = a)) uses_model 
                        | _ -> uses_model) in
        let bidi_uses x y = 
                List.exists (fun ll -> (List.mem x ll) && (List.mem y ll))
                        equiv_classes in*)
                        (*(List.mem (x,y) uses_model) 
                        || (List.mem (y,x) uses_model) in*)
        let omitname, thisname = 
                match modulenameopt,pluginopt with 
                        (Some m,_) -> m,m
                        | (_,Some p) -> p,""
                        | _ -> "","" in
        let bidi_uses = determine_redundant_for_copyto omitname uses_model in
        let is_ignorable x = List.mem (TypeCheck.get_union_from_strio conseq_res x) ignoreis in
        let define_canon_module_pair nsn1 nsn2 =
                let oneach nsn =
                        let broken = break_namespaced_name nsn
                        in  cstyle_name broken
                in  "_HAVE_COPY_TO_"^(oneach nsn1)^"_FROM_"^(oneach nsn2) in
        let code_template code (assign_opt,t_name,f_name) =
                let as_name (x,isin) = 
                        let xtmp = x^(if isin then "_in" else "_out")
                        in  match pluginopt,break_namespaced_name xtmp with
                                (None,mn::_::_) -> xtmp,Some mn
                                | (Some pn,mn::_::_) -> 
                                        if pn = mn then (nspace^xtmp),Some pn
                                        else xtmp,Some mn
                                | _ -> (nspace^xtmp),None in
                let t_name',t_nspace = as_name t_name in
                let f_name',f_nspace = as_name f_name in
                let is_already_taken_care_of1 =
                        (match t_nspace,f_nspace with
                                (Some tn,Some fn) -> bidi_uses tn fn
                                | (Some tn,None) -> bidi_uses tn ""
                                | (None,Some fn) -> bidi_uses "" fn
                                | _ -> false) in
                let is_already_taken_care_of2 =
                        ((is_ignorable t_name) 
                                && (is_ignorable f_name)
                                && (match pluginopt with
                                        None -> false
                                        | _ -> true)) && false in 
                let is_already_taken_care_of =
                        is_already_taken_care_of1 || is_already_taken_care_of2
                        in let _ = Debug.dprint_string ("c2 consider "^t_name'^","^f_name'^"\n")
                in  match is_already_taken_care_of, assign_opt with
                        (true,_) -> 
                                let _ = Debug.dprint_string ("  no gen1."
                                        ^(if is_already_taken_care_of1 then "1t" else "1f")
                                        ^(if is_already_taken_care_of2 then "2t" else "2f")
                                        ^"\n") in 
                                code
                        | (_,None) -> 
                                let _ = Debug.dprint_string "  no gen2\n" in 
                                code
                        | (_,Some []) -> 
                                let _ = Debug.dprint_string "  no gen3\n" in 
                                code
                        | (_,Some copy_code) -> 
                                let _ = Debug.dprint_string "  gen template\n" in
                                let _ = try let isreally = List.assoc t_name conseq_res.TypeCheck.aliases in
                                            let irname,_ = as_name isreally
                                            in  Debug.dprint_string  (" "^t_name'^" is "^irname^"\n")
                                        with Not_found -> ()
                                        in
                                let _ = try let isreally = List.assoc f_name conseq_res.TypeCheck.aliases in
                                            let irname,_ = as_name isreally
                                            in  Debug.dprint_string  (" "^f_name'^" is "^irname^"\n")
                                        with Not_found -> () in
                                let def_const = define_canon_module_pair t_name' f_name'
                                in List.fold_left add_code code
                                        ([ "#ifndef "^def_const
                                        ; "# define "^def_const
                                        ; "template<>"
                                        ; ("inline void copy_to<"
                                                ^t_name'^", "
                                                ^f_name'(*^", "*)
                                                (*^t_name'^"meta::depth, "
                                                ^f_name'^"meta::depth"*)
                                                (*^"0,0" temp*)
                                                ^">("^t_name'^" * to, const "
                                                ^f_name'^" * from)")
                                        ; "{" ]
                                        @ copy_code @
                                        [ "}"
                                        ; "#endif"
                                        ; "" ])
                in
        let code = generic_cross_equiv_weak_unify code code_template symtable conseq_res uses_model thisname
	in  List.fold_left add_code code [ ""; "};"; "   //namespace" ]

let emit_io_conversion_functions pluginopt modulenameopt conseq_res symtable uses_model code =
        let omitname,thisname = 
                match modulenameopt,pluginopt with 
                        (Some m,_) -> m,m
                        | (_,Some p) -> p,""
                        | _ -> "","" in
        let code = add_code code "oflux::IOConverterMap __ioconverter_map[] = {" in
        let code_table_entry code (assign_opt,t_name,f_name) =
                let as_name (x,isin) = x^(if isin then "_in" else "_out")
                in  match assign_opt with
                        None -> code
                        | (Some []) -> code
                        | (Some _) -> 
                                let t_u_n = TypeCheck.get_union_from_strio conseq_res t_name in
                                let f_u_n = TypeCheck.get_union_from_strio conseq_res f_name in
                                let tstr = as_name t_name in
                                let fstr = as_name f_name in
                                let t_u_n_str = string_of_int t_u_n in
                                let f_u_n_str = string_of_int f_u_n 
                                in  add_code code ("{ "^f_u_n_str^", "^t_u_n_str^", &oflux::create_real_io_conversion<"^tstr^", "^fstr^" > }, ")
                in
        let code = generic_cross_equiv_weak_unify code code_table_entry symtable conseq_res uses_model thisname
        in  List.fold_left add_code code [ "{ 0, 0, NULL }  " ; "};" ]
        

        

let emit_atomic_key_structs symtable code =
	let e_member df = ((strip_position df.ctypemod)
			^" "^(strip_position df.ctype)
			^" "^(strip_position df.name)^";") in
	let e_lthan df = 
		let n = strip_position df.name
		in  ("res = res || (eq && "^n^" < "^"o."^n^");"
			^" eq = eq && ("^n^" == o."^n^");")
		in
	let e_equals df =
		let n = strip_position df.name
		in  ("res = res && ("^n^" == "^"o."^n^");")
		in
	let e_one n gd code =
		List.fold_left add_code code
			( let clean_n = clean_dots n in
			  [ "struct "^(clean_n)^"_key {" ]
			@ (List.map e_member gd.garguments)
			@ [ ""
			  ; "inline bool operator<(const "^clean_n^"_key & o) const {" 
			  ; "bool eq=true;"
			  ; "bool res=!eq;"
			  ]
			@ (List.map e_lthan gd.garguments)
			@ [ "return res;"
			  ; "}"
			  ; ""
			  ; "inline bool operator==(const "^clean_n^"_key & o) const {"
			  ; "bool res = true;"
			  ]
			@ (List.map e_equals gd.garguments)
			@ [ "return res;"
			  ; "}"
			  ; ""
			  ; "};" ]
                        )
	in  SymbolTable.fold_guards e_one symtable code

let emit_atomic_key_structs_hashfuns symtable code nsopt =
        let e_hash df =
                let n = strip_position df.name in
                let tm = strip_position df.ctypemod in
                let t = strip_position df.ctype
                in  "res = res ^ oflux::hash<"^tm^" "^t^">()(k."^n^");" in
	let e_one n gd code =
		List.fold_left add_code code
			( let clean_n = clean_dots n in
                          let clean_n = match nsopt with
                                       (Some ns) -> ns^"::"^clean_n
                                        | None -> clean_n
                        in
                          [ ""
                          ; "namespace oflux {  "
                          ; "template<>"
                          ; "struct hash<"^clean_n^"_key> {"
                          ; "size_t operator()(const "^clean_n
                                ^"_key & k) const"
                          ; "{" 
                          ; "size_t res = 0;" ]
			@ (List.map e_hash gd.garguments)
                        @ [ "return res;"
                          ; "}"
                          ; "};"
                          ; "} // namespace oflux" ]
                        )
	in  SymbolTable.fold_guards e_one symtable code

let gather_atom_aliases symtable =
	let lookup_return_type n =
		let gd = SymbolTable.lookup_guard_symbol symtable n in
		let rt = gd.return
		in  (strip_position rt.dctypemod)^" "^(strip_position rt.dctype)
		in
	let extract_gr gr =
		let cn,_,_ = gr.guardname in
		let n,_,_ =
			match gr.localgname with
				None -> gr.guardname
				| (Some g) -> g
		in  n,lookup_return_type cn,gr.modifiers in
	let each_node n nd (ll,aliases) =
		let nf = nd.functionname in
		let ukey = List.map extract_gr nd.SymbolTable.nodeguardrefs 
		in  try let nn = List.assoc ukey ll
			in  (ll,(nf,nn)::aliases)
		    with Not_found -> ((ukey,nf)::ll,aliases) in
	let _,al = SymbolTable.fold_nodes each_node symtable ([],[])
	in  al
	

let emit_node_atomic_structs pluginopt readonly symtable aliases code =
	let ro_prefix = if readonly then "const " else "" in
	let lookup_return_type n =
		let gd = SymbolTable.lookup_guard_symbol symtable n in
		let rt = gd.return
		in  (strip_position rt.dctypemod)^" "^(strip_position rt.dctype)
		in
	let prefix_differently pref_hd pref_tl ll =
		match ll with
			(h::tl) -> (pref_hd^h)::(List.map (fun x-> pref_tl^x) tl)
			| _ -> [] in
        let remove_plugin_prefix =
                match pluginopt with
                        None -> (fun n -> n)
                        | (Some pn) -> (fun n -> remove_prefix (pn^".") n) in
	let nt_of_gr gr = 
		let name_canon,_,_ = gr.guardname in
		let namep = match gr.localgname with 
				None -> gr.guardname
				| Some ln -> ln
				in
		let is_ro = match gr.modifiers with
				(ParserTypes.Read::_) -> true
				| _ -> false in
		let name,pos,_ = namep
		in  try (remove_plugin_prefix name, lookup_return_type name_canon, is_ro) 
		    with Not_found -> raise (CppGenFailure ("can't find guard "^name^" at "^"l:"^(string_of_int pos.lineno)^" c:"^(string_of_int pos.characterno)))
		in
	let inits nl = List.map (fun n -> "_"^(clean_dots n)^"(NULL)") nl in
	let decls ntl = List.map (fun (n,t,_) -> 
		t^" * _"^(clean_dots n)^";") ntl in
	let accessor (n,t,iro) =
		let rop = if iro then ro_prefix else "" in
		let n = clean_dots n
		in rop^t^(if iro then " " else " & ")^n^"() "
			^rop^" { return *_"^n^"; }  " in
        let possessor (n,t,iro) =
		let n = clean_dots n in
                "bool have_"^n^"() const { return _"^n^" != NULL; }  " in
	let accessors ntl = List.map accessor ntl in
	let possessors ntl = List.map possessor ntl in
        let omit_emit n = omit_emit_for pluginopt n in
	let e_one n nd code =
	    if omit_emit n then code
	    else
		let nf = nd.functionname in 
                let n = remove_plugin_prefix n in
                if has_dot n then code
                else
		try let nn = List.assoc nf aliases
		    in  List.fold_left add_code code
			[ "typedef "^nn^"_atoms "^n^"_atoms;"
			; "" ]
		with Not_found ->
			let grs = nd.nodeguardrefs in
			let ntl = List.map nt_of_gr grs in
			let nl = List.map (fun (x,_,_) -> x) ntl
			in  List.fold_left add_code code
				( [ "class "^n^"_atoms {" 
				  ; "public:"
				  ; n^"_atoms()"
				  ]
				@ (prefix_differently ": " ", " (inits nl))
				@ [ "{}  " 
				  ; ""
				  ]
				@ (accessors ntl)
				@ (possessors ntl)
				@ [ "void fill(oflux::AtomicsHolder * ah);"
				  ; "private:"
				  ]
				@ (decls ntl)
				@ [ "};" ] )
	in  SymbolTable.fold_nodes e_one symtable code

let emit_atom_fill pluginopt symtable aliases code =
	let lookup_return_type n =
		let gd = SymbolTable.lookup_guard_symbol symtable n in
		let rt = gd.return
		in  (strip_position rt.dctypemod)^" "^(strip_position rt.dctype)
		in
	let nt_of_gr gr = 
		let name_canon,_,_ = gr.guardname in
		let namep = match gr.localgname with 
				None -> gr.guardname
				| Some ln -> ln in
		let name = strip_position namep
		in  (name, lookup_return_type name_canon) in
        let trim_dot =
                match pluginopt with
                        None -> (fun x -> x)
                        | (Some pn) -> (remove_prefix (pn^"."))
                        in
	let code_for_one (cl,i) (n,t) =
                let atomic_n = "atomic_"^(clean_dots (trim_dot n))
                in
		(("oflux::Atomic * "^atomic_n^" = ah->get("
		^(string_of_int i)^",oflux::AtomicsHolder::HA_get_lexical)->atomic();")
                ::("_"^(clean_dots (trim_dot n))
			^" = ("^atomic_n^" ? reinterpret_cast<"^t
                ^" *> ("^atomic_n^"->data()) : NULL);")
                ::cl), i+1 in
        let omit_emit n = omit_emit_for pluginopt n in
	let e_one n nd code =
		try if omit_emit n then code 
		    else let _ = List.assoc nd.functionname aliases
		        in code
		with Not_found ->
			let ntl = List.map nt_of_gr nd.nodeguardrefs in
			let code = List.fold_left add_code code
				[ "void "^(trim_dot n)^"_atoms::fill(oflux::AtomicsHolder * ah)"
				; "{"
				] in
			let codelist, _ = List.fold_left code_for_one ([],0) ntl in
			let code = List.fold_left add_code code codelist
			in  add_code code "}"  
	in  SymbolTable.fold_nodes e_one symtable code

let emit_atom_map_decl symtable code =
	let e_one n gd code =
                if gd.gexternal then code
                else
		let clean_n = clean_dots n
		in  add_code code ("extern oflux::AtomicMapAbstract * "^clean_n
                                ^"_map_ptr;") in
	let code = add_code code "namespace ofluximpl {" in
	let code = SymbolTable.fold_guards e_one symtable code 
	in  List.fold_left add_code code 
		[ "}"
		; "    //namespace"
		; ""
		]
	

let emit_atom_map_map plugin_opt symtable code =
	let e_one n gd code =
                if gd.gexternal then code
                else
                let clean_n = clean_dots n in
                List.fold_left add_code code
		((
                if gd.gtype = "pool" then
                        ("oflux::AtomicPool "^clean_n^"_map;")
                else
		let atomic_class_str =
			match gd.gtype with
				"exclusive" -> "oflux::AtomicExclusive"
				| "readwrite" -> "oflux::AtomicReadWrite"
				| "free" -> "oflux::AtomicFree"
				| _ -> raise (CppGenFailure ("unsupported guard type "^gd.gtype))
		in  (if 0 = (List.length gd.garguments) then
			("oflux::AtomicMapTrivial<"
				^atomic_class_str^"> "^clean_n^"_map;")
		    else
                        let mappolicy =
                                "oflux::"
                                ^(if gd.gunordered 
                                        then "Hash"
                                        else "Std")
                                ^"MapPolicy<"^clean_n^"_key> "
                        in
			("oflux::AtomicMapStdMap<"^mappolicy
				^","^atomic_class_str^"> "^clean_n^"_map;")
			))::[ "oflux::AtomicMapAbstract * "^clean_n
                                ^"_map_ptr = &"^clean_n^"_map;" ])
		in
	let e_two n gd codelist =
                let omit_emit = 
                        match plugin_opt with
                                None -> false
                                | (Some pn) -> not (string_has_prefix n (pn^".")) in
		let clean_n = clean_dots n
		in  if omit_emit then codelist else ("{ \""^n^"\", &"^clean_n^"_map }, ")::codelist in
	let code = SymbolTable.fold_guards e_one symtable code in
	let codelist = SymbolTable.fold_guards e_two symtable []
	in  List.fold_left add_code code 
		( [ "oflux::AtomicMapMap __atomic_map_map[] = {"
		  ]
		@ codelist
		@ [ "{ NULL, NULL }  "
		  ; "};"
		  ] )

let possible_instances_of to_dfl from_dfl =
	let rec index (i,rl) x = (i+1,(x,i)::rl) in
	let _,numbered_fdfl = List.fold_left index (1,[]) from_dfl in
	let rec cart_prod lll =
		match lll with
			[ll] -> List.map (fun x -> [x]) ll
			| (hl::tll) ->
				List.concat (List.map (fun tl ->
					List.map (fun h -> h::tl) 
					hl) (cart_prod tll))
			| [] -> [] in
	let type_match df1 df2 =
		match Unify.unify_single (SymbolTable.strip_position3 df1)
			(SymbolTable.strip_position3 df2) with
		    None -> true
		    | (Some _) -> false in
	let do_one to_df =
		List.filter (fun (df,_) -> type_match to_df df) numbered_fdfl
	in  match to_dfl with
                [] -> [[]]
		| _ -> cart_prod (List.map do_one to_dfl)

	

(*
let emit_guard_trans_map (with_proto,with_code,with_argnos,with_map) 
                conseq_res symtable code =
	* trick is to determine all possible unifications 
	   and pop them in this table.
	*
	let get_u_n x = TypeCheck.get_union_from_strio conseq_res (x,true) in
	let one_inst garg gn nn nf (code,line,donel) pi =
                if pi = [] then code,line,donel
                else
		let alist = List.map (fun (_,i) -> (string_of_int i)^", ") pi in
		let cat a b = a^b in
		let arglist = "{ "^(List.fold_right cat alist "0 }, ") in
		let gpi = List.combine garg pi in
		let ff str (_,i) = str^"_"^(string_of_int i) in
		let pistr = List.fold_left ff "_" pi in
		let translate_name = clean_dots ("g_trans_"^gn^"_"^nn^pistr) 
                in  if List.mem translate_name donel then
                        (code,line,donel)
                    else
                        let proto = "void "^translate_name^"( "
                                ^"void * out, const void *in)"
                                ^(if with_code then "" else ";") in
                        let assign (gdf,(ndf,_)) =
                                "(reinterpret_cast<"^(clean_dots gn)^"_key *>"
                                ^"(out))->"^(strip_position gdf.name)
                                ^" = (reinterpret_cast<const "
                                ^nf^"_in *>(in))->"^(strip_position ndf.name)^";" in
                        let ccode = [ "{" ]
                                  @ (List.map assign gpi)
                                  @ [ "}" ] in
                        let u_n = get_u_n nf in
                        let argnum = List.length pi in
                        let mapline = "{ \""^gn^"\", "^(string_of_int u_n)
                                ^", "^(string_of_int argnum)
                                ^", &(_argument_arr["^(string_of_int line)^"][0])"
                                ^", "^translate_name^" },  "
                        in  List.fold_left add_code code
                            ( (if with_proto then [ proto ] else [])
                            @ (if with_code then ccode else [])
                            @ (if with_map then [ mapline ] else [] ) 
                            @ (if with_argnos then [ arglist ] else [] ) 
                            ), line+1, translate_name::donel
                        in
        let e_g gn gd (code,line,donel) =
                let e_n nn nd (code,line,donel) =
			let nf = nd.functionname in
			let pio = possible_instances_of gd.garguments nd.nodeinputs in  
                        let code,line,donel = List.fold_left (one_inst gd.garguments gn nn nf) (code,line,donel) pio
                        in  code,line,donel
                in  SymbolTable.fold_nodes e_n symtable (code,line,donel)
                in
        let code,_,_ = SymbolTable.fold_guards e_g symtable (code,0,[])
        in  code
*)

let emit_guard_trans_map (with_proto,with_code,with_map) conseq_res symtable code =
        let get_u_n x = TypeCheck.get_union_from_strio conseq_res (x,true) in
        let e_n nn nd (code,donel) =
                let nf = nd.functionname in
                let u_n = get_u_n nf in
                let fill_expr uel =
                        let on_ue ue =
                                match ue with
                                        (Arg s) -> ("(reinterpret_cast<const "
                                                ^nf^"_in *>(in)->"^s^")")
                                        | (Context s) -> s in
                        let ffun str ue = 
                                str^(on_ue ue)
                        in  List.fold_left ffun "" uel in
                let assign gn (gdf,ndf_expr) =
                        "(reinterpret_cast<"^(clean_dots gn)^"_key *>"
                        ^"(out))->"^(strip_position gdf.name)
                        ^" = "
                        ^(fill_expr ndf_expr)^";" in
                let res_test test_expr =
                        match test_expr with
                                [] -> ""
                                | _ ->
                                        ("if(!("^(fill_expr test_expr)
                                        ^")) { return false; }  ") in
                let wtype grm = 
                        match grm with 
                                (Read::_) -> 1
                                | (Write::_) -> 2
                                | _ -> 3 in
                let e_gr (code,donel) gr =
                        let gn = strip_position gr.guardname in
                        let gd = SymbolTable.lookup_guard_symbol symtable gn in
                        let hash = HashString.hash (gr.arguments, gr.guardcond) in
                        let gtfunc = clean_dots ("g_trans_"^nn^"_"^gn^"_"^hash) in
                        if (not with_map) && List.mem gtfunc donel then
                                (code,donel)
                        else
                        let proto = "bool "^gtfunc^"( "
                                ^"void * out, const void *in)"
                                ^(if with_code then "" else ";") in
                        let mapline = ("{ \""^gn^"\", "
                                ^(string_of_int u_n)^", "
                                ^"\""^hash^"\", "
                                ^(string_of_int (wtype gr.modifiers))^", "
                                ^"&"^gtfunc
                                ^" },  ") in
                        let codell = 
                                let assignon = List.combine gd.garguments gr.arguments 
                                in  List.map (assign gn) assignon in
                        let code = if with_proto then
                                        add_code code proto
                                else code in
                        let code = if with_code then 
                                        List.fold_left add_code code
                                        ( [ proto
                                          ; "{  "
                                          ] 
                                        @ [res_test gr.guardcond]
                                        @ (codell)
                                        @ [ "return true;"
                                          ; "}  "
                                          ; "" ] )
                                else code in
                        let code = if with_map then 
                                        add_code code mapline
                                else code 
                        in  code,gtfunc::donel
                in  List.fold_left e_gr (code,donel) (nd.nodeguardrefs)
                in
        let code,_ = SymbolTable.fold_nodes e_n symtable (code,[])
        in  code


let emit_node_func_decl plugin_opt is_concrete errorhandlers symtable code =
	let is_eh n = List.mem n errorhandlers in
        let emit_for n =
                (is_concrete n) &&
                (match plugin_opt with
                        None -> (not (has_dot n))
                        | (Some pn) -> string_has_prefix n (pn^".")
                                &&(let nopre = remove_prefix (pn^".") n in not (has_dot nopre)))
                in
        let trim_dc,trim_dot =
                match plugin_opt with
                        None -> (fun x -> x), (fun x -> x)
                        | (Some pn) -> (remove_prefix (pn^"::")),(remove_prefix (pn^"."))
                        in
	let e_one n _ code =
		let nd = SymbolTable.lookup_node_symbol symtable n
		in  if emit_for n then
                        let nt = trim_dot n in
                        let fnt = trim_dc nd.functionname
                        in 
			add_code code
			("int "^fnt^"(const "^nt^"_in *, "^nt
			^"_out *, "^nt^"_atoms *"
			^(if is_eh n then ", int" else "")
			^");")
		else code
	in  SymbolTable.fold_nodes e_one symtable code

let emit_cond_func_decl plugin_opt symtable code =
        let omit_emit n = omit_emit_for plugin_opt n in
                (*match plugin_opt with
                        None -> (has_dot n) || ((List.length(break_namespaced_name n)) != 1) 
                        | (Some pn) -> not (string_has_prefix n (pn^"."))
                in *)
        let trim_dot =
                match plugin_opt with
                        None -> (fun x -> x)
                        | (Some pn) -> (remove_prefix (pn^"."))
                        in
	let e_one n cond code =
		match cond.SymbolTable.arguments with
			[d] ->
			    if omit_emit n then code
			    else
				let tm = d.ctypemod in
				let t = d.ctype in
				let arg = d.name
				in
				add_code code
				("bool "^(trim_dot n)^"("^(strip_position tm)
				^" "^(strip_position t)
				^" "^(strip_position arg)^");")
			| _ -> raise (CppGenFailure ("emit_cond_func_decl -internal"))
	in SymbolTable.fold_conditionals e_one symtable code

let emit_create_map plugin_opt is_concrete symtable conseq_res ehs code =
	let is_eh f = List.mem f ehs in
	(*let lookup_union_number (n,isin) =
		try TypeCheck.get_union_from_strio conseq_res (n,isin)
		with Not_found -> raise (CppGenFailure ("emit_create_map: not found union "
			^n^" "^(if isin then "input" else "output")))
		in*)
        let is_abstract n = SymbolTable.is_abstract symtable n in
	let e_one n _ code =
                let ignore_emit = 
                        match plugin_opt with
                                None -> has_dot n
                                | (Some pn) -> not (string_has_prefix n (pn^"."))
		in  if ignore_emit then code else 
			let nd = SymbolTable.lookup_node_symbol symtable n in
                        let nf = nd.functionname in
			let nfind = (match plugin_opt with
                                        None -> nf
                                        | (Some pn) -> remove_prefix (pn^"::") nf)
			in if (is_concrete n) && not (is_abstract n) then
				(*let u_no_in = lookup_union_number (nf,true) in
				let u_no_out = lookup_union_number (nf,false) 
				in*)  add_code code ("{ \""^nfind
					^"\", &oflux::create"
					^(if is_eh n then "_error" else "")
					(*^"<OFluxUnion"^(string_of_int u_no_in)^", "
					^"OFluxUnion"^(string_of_int u_no_out)^", "*)
                                        ^"<"(*^nf^"_in::base_type, "
                                        ^nf^"_out::base_type, "*)
					^nf^"_in, "
					^nf^"_out, "
					^nf^"_atoms, "
					^"&"^nd.functionname^" > },  ")
			else code
		in
	let code = List.fold_left add_code code 
			[ ""
			; "oflux::CreateMap __create_map[] = {"
			] in
	let code = SymbolTable.fold_nodes e_one symtable code in
	let code = List.fold_left add_code code [ "{ NULL, NULL }  " ; "};" ]
	in  add_code code "oflux::CreateMap * __create_map_ptr = __create_map;"

let emit_master_create_map modules code =
	let per_module m =
		let mm = if m = "" then m else m^"::"
		in  "{ \""^mm^"\", "^mm^"ofluximpl::__create_map_ptr }, "
	in  List.fold_left add_code code 
		( [ "oflux::ModularCreateMap __master_create_map[] = {" ]
		@ (List.map per_module modules)
		@ [ "{ \"\", __create_map_ptr }, "
		  ; "{ NULL, NULL }  " 
		  ; "};" 
		  ; "" ] 
                )

let emit_converts is_plugin modulenameopt ignoreis conseq_res code =
        let collapsed_types_i = conseq_res.TypeCheck.full_collapsed in
        let aliased_types = conseq_res.TypeCheck.aliases in
	let nspace =
		match modulenameopt with
			None -> ""
			| (Some nn) -> if is_plugin then "" else nn^"::"
		in
	let avoid ll i = 
		try let _ = List.find (fun (ii,_) -> i=ii) ll
		    in true
		with Not_found -> false in
	let avoid_n x = avoid aliased_types x in
	let code = 
		(*if is_plugin then code
		else*)
		List.fold_left add_code code
		[ "namespace oflux {  "
		; ""
		; "#ifndef OFLUX_CONVERT_GENERAL"
		; "#define OFLUX_CONVERT_GENERAL"
		; "template<typename H>"
		; "inline H * convert(typename H::base_type *)"
		; "{"
		; "enum { general_conversion_not_supported_bug = 0 } _an_enum;"
		; "oflux::CompileTimeAssert<general_conversion_not_supported_bug> cta;"
		; "}"
		; ""
		; "template<typename H>"
		; "inline H const * const_convert(typename H::base_type const * g)"
		; "{"
		; "return convert<H>(const_cast<typename H::base_type *>(g));"
		; "}"
		; "#endif //OFLUX_CONVERT_GENERAL"
		; ""
		] in 
	let code_template code u_name s_name isderef =
                let broken_name = 
			match modulenameopt, break_namespaced_name s_name with
				(Some p, h::t) -> 
					if is_plugin && (p=h) then 
						t
					else h::t
				| (_,ll) -> ll in
		let cs_name = cstyle_name broken_name
		in  if (List.length broken_name) = 1 
		    then
                        List.fold_left add_code code 
			[ "template<>"
			; ("inline "^nspace^s_name^" * convert<"
				^nspace^s_name^" >( "^nspace^u_name^" * s)")
			; "{"
			; ("return "^(if isderef then ("&((*s)._"^cs_name^")")
					else "s")^";")
			; "}"
			; ""
			] 
                    else code
                in
	let convert_ff (code,isdone) ((n,n_isinp),u_n) =
		let pluginprefix =
			match is_plugin, modulenameopt with
				(true,Some p) -> p^"::"
				| _ -> "" in
		let u_name = pluginprefix^"OFluxUnion"^(string_of_int u_n)
		in      try let _ = List.assoc u_n collapsed_types_i
			    in  if List.mem u_n isdone then
					code,isdone
				else code_template code u_name u_name false
					, u_n::isdone
			with Not_found ->
		let io_suffix = if n_isinp then "_in" else "_out" in
		let should_avoid = avoid_n (n,n_isinp) in
		let s_name = n^io_suffix 
		in  (if should_avoid 
				then code 
				else code_template code u_name s_name true)
			, isdone in
	let code,_ = TypeCheck.consequences_umap_fold convert_ff (code,ignoreis) conseq_res
	in  List.fold_left add_code code [ ""; "};"; "   //namespace" ]

let emit_cond_map conseq_res symtable code =
	let unify s t =
		let s' = SymbolTable.strip_position3 s in
		let t' = SymbolTable.strip_position3 t 
		in  match Unify.unify_single s' t' with
			(Some _) -> false
			| _ -> true in
	let try_arg n nf unionnumber d (i,code) arg =
		let t =
			let tm = d.ctypemod in
			let t = d.ctype
			in  (strip_position tm)^" "^(strip_position t)
		in  (i+1,if unify d arg then 
			add_code code ("{ "^(string_of_int unionnumber)
					^", "^(string_of_int i)
					^", \""^nf^"\", "
					^"&oflux::eval_condition_argn<OFluxUnion"
					^(string_of_int unionnumber)^", "
					^t^", "
					^"&"^nf^", "
					^"&OFluxUnion"^(string_of_int unionnumber)
					^"::argn"^(string_of_int i)
					^"> },  ")
			else code)
		in
	let try_union cond_n condfunc_n d code u =
		let n,isin = List.hd u in
		let unionnumber = TypeCheck.get_union_from_strio conseq_res (n,isin) in
		let args = get_decls symtable (n,isin) in
		let _,code = List.fold_left (try_arg cond_n condfunc_n unionnumber d) (1,code) args
		in code in
	let e_one n cond code =
		match cond.SymbolTable.arguments with
			[d] -> TypeCheck.consequences_equiv_fold
                                (try_union n cond.SymbolTable.cfunction d) 
                                code conseq_res
			| _ -> raise (CppGenFailure "e_one - internal") in
	let code = List.fold_left add_code code
			[ ""
			; "oflux::ConditionalMap __conditional_map[] = {"
			] in
	let code = SymbolTable.fold_conditionals e_one symtable code
	in  List.fold_left add_code code 
			[ "{ 0, 0, NULL, NULL }  "
			; "};" 
			; "" 
			]

let calc_max_guard_size symtable =
	let max x y = if x > y then x else y in
	let m_one n gd sz = max sz (List.length gd.garguments)
	in  SymbolTable.fold_guards m_one symtable 1
		

let emit_test_main code =
	List.fold_left add_code code
		[ "#ifdef TESTING"
		; "#include <signal.h>"
		; "#include <cstring>"
		; "using namespace oflux;"
		; "boost::shared_ptr<RunTimeBase> theRT;"
		; "void handlesighup(int)"
		; "{"
		; "signal(SIGHUP,handlesighup);"
		; "if(theRT) theRT->soft_load_flow();"
		; "}"
		; "int main(int argc, char * argv[])"
		; "{"
		; "assert(argc >= 2);"
		; "#ifdef HASINIT"
		; "init(argc-1,&(argv[1]));"
		; "#endif //HASINIT"
		; "static flow::FunctionMaps ffmaps(ofluximpl::__conditional_map, ofluximpl::__master_create_map, ofluximpl::__theGuardTransMap, ofluximpl::__atomic_map_map, ofluximpl::__ioconverter_map);"
		; "RunTimeConfiguration rtc = {"
		; "  1024*1024 // stack size"
		; ", 1 // initial threads (ignored really)"
		; ", 0 // max threads"
		; ", 0 // max detached"
		; ", 0 // thread collection threshold"
		; ", 1000 // thread collection sample period (every N node execs)"
		; ", argv[1] // XML file"
		; ", &ffmaps"
		; ", \"xml\""
		; ", \"lib\""
		; ", NULL"
		; "};"
		; "theRT.reset(create_"
                  ^(CmdLine.get_runtime_engine())
                  ^"_runtime(rtc));"
		; "signal(SIGHUP,handlesighup); // install SIGHUP handling"
		; "logging::toStream(std::cout); // comment out if no oflux logging is desired"
		; "const char * oflux_config=getenv(\"OFLUX_CONFIG\");"
                ; "if(oflux_config == NULL || NULL==strstr(oflux_config,\"nostart\")) {"
		; "theRT->start();"
                ; "}"
		; "#ifdef HASDEINIT"
		; "deinit();"
		; "#endif //HASDEINIT"
                ; "theRT.reset(); // force collection before static dtr order"
		; "}"
		; "#endif // TESTING"
		]

let get_module_file_suffix modulenameopt =
	let ns_broken = 
		match modulenameopt with
			None -> []
			| (Some nsn) -> break_namespaced_name nsn
		in
	let const_ns_fun csofar str = 
		csofar^(if String.length csofar = 0 then "" else "__")^str in
	let const_ns = List.fold_left const_ns_fun "" ns_broken in
	let const_ns = 
		if String.length const_ns > 0 
		then "_"^const_ns
		else ""
	in  const_ns


let emit_plugin_cpp pluginname brbef braft um deplist =
	let stable = braft.Flow.symtable in
        let conseq_res = braft.Flow.consequences in
	let fm = braft.Flow.fmap in
	let ehs = braft.Flow.errhandlers in
	let h_code = CodePrettyPrinter.empty_code in
        let ns_broken = break_namespaced_name pluginname in
	let def_const_ns_fun csofar str = csofar^(String.capitalize str)^"__" in
	let def_const_ns = List.fold_left def_const_ns_fun "" ns_broken in
	let def_const_ns = 
		if String.length def_const_ns > 0 
		then "_"^def_const_ns
		else ""
		in
	let file_suffix = get_module_file_suffix (Some pluginname) in
        let lc_codeprefix = CmdLine.get_code_prefix () in
        let uc_codeprefix = String.uppercase lc_codeprefix in
        let dep_includes = List.map
                (fun dep -> "#include \""
                        ^lc_codeprefix
                        ^"_"^dep^".h\"")
                deplist in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		( [ "#ifndef _"^uc_codeprefix^def_const_ns
		  ; "#define _"^uc_codeprefix^def_const_ns ]
                @ dep_includes
		@ [ "#include \"mImpl"^file_suffix^".h\""
		] ) in
	let modules = braft.Flow.modules in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		(List.map (fun m -> "#include \""^lc_codeprefix^"_"^m^".h\"") modules) in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ "#include \""^lc_codeprefix^".h\" // get the standard stuff from the pre-built header"
		; ""
		; ""
		; "// ---------- OFlux generated header (do not edit by hand) ------------"
		] in
	let namespaceheader code = expand_namespace_decl_start code ns_broken in
	let namespacefooter code = expand_namespace_decl_end code ns_broken in
        let ffunmapname = "oflux::flow::FunctionMaps * flowfunctionmaps_"^file_suffix^"()" in
        let h_code = List.fold_left CodePrettyPrinter.add_code h_code
                [ ""
                ; "namespace oflux {"
                ; "namespace flow {"
                ; "class FunctionMaps;"
                ; "}"
                ; "}"
                ; "extern \"C\" {"
                ; ffunmapname^";"
                ; "}"
                ; "" 
                ] in
	let h_code = namespaceheader h_code in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ ""
		; "namespace ofluximpl {"
                ; "extern oflux::CreateMap * __create_map_ptr;" 
		; "extern oflux::CreateMap __create_map[];"
		; "extern oflux::ConditionalMap __conditional_map[];"
		; "extern oflux::AtomicMapMap __atomic_map_map[];"
		; "extern oflux::GuardTransMap __theGuardTransMap[];"
		; "extern oflux::IOConverterMap __ioconverter_map[];"
		; "}"
		; "    //namespace"
		; ""
		] in
	let h_code = emit_atom_map_decl stable h_code in
	let is_concrete n = Flow.is_concrete stable fm n in
	let h_code = emit_union_forward_decls conseq_res stable h_code in
	let h_code,ignoreis = emit_plugin_structs pluginname conseq_res stable h_code in
	let h_code = emit_atomic_key_structs stable h_code in
	let node_atom_aliases = gather_atom_aliases stable in
	let h_code = emit_node_atomic_structs (Some pluginname) true stable node_atom_aliases h_code in
	let h_code = CodePrettyPrinter.add_cr h_code in
	let h_code = emit_node_func_decl (Some pluginname) is_concrete ehs stable h_code in
	let h_code = emit_cond_func_decl (Some pluginname) stable h_code in
	let h_code = emit_guard_trans_map (true,false,false) 
			conseq_res stable h_code in
	let h_code = CodePrettyPrinter.add_cr h_code in
	let h_code = emit_unions (Some pluginname) conseq_res stable h_code in
	let h_code = namespacefooter h_code in
        let h_code = emit_atomic_key_structs_hashfuns stable h_code (Some pluginname) in
	let h_code = emit_converts true (Some pluginname) ignoreis conseq_res h_code in
        let h_code = emit_copy_to_functions (Some pluginname) None ignoreis conseq_res stable um h_code in
	let h_code = CodePrettyPrinter.add_code h_code ("#endif // _"^uc_codeprefix) in
	let cpp_code = CodePrettyPrinter.empty_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code
		[ "#include \""^lc_codeprefix^file_suffix^".h\""
		; "#include \"OFluxFlow.h\""
		; "#include \"OFluxAtomic.h\""
		; "#include \"OFluxAtomicHolder.h\""
		; "#include \"OFluxRunTimeBase.h\""
		; "#include \"OFluxIOConversion.h\""
		; "#include \"OFluxLogging.h\""
		; "#include <boost/shared_ptr.hpp>"
		; "#include <iostream>"
		; ""
		; "// ---------- OFlux generated header (do not edit by hand) ------------"
		; ""
		] in
	let cpp_code = namespaceheader cpp_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code [""; "namespace ofluximpl {"; "" ] in
	let cpp_code = emit_create_map (Some pluginname) is_concrete stable conseq_res ehs cpp_code in
	let cpp_code = emit_cond_map conseq_res stable cpp_code in
	let cpp_code = 
		let cpp_code = emit_atom_map_map (Some pluginname) stable cpp_code in
		let cpp_code = emit_guard_trans_map (false,true,false)  conseq_res stable cpp_code in
		(*let max_guard_size = calc_max_guard_size stable in
		let cpp_code = add_code cpp_code
				("const int _argument_arr[]["
				^(string_of_int (max_guard_size+1))
				^"] = {") in
		let cpp_code = emit_guard_trans_map (false,false,true,false) conseq_res stable cpp_code in
		let cpp_code = List.fold_left add_code cpp_code [ "{ 0 }  "; "};" ] in *)
		let cpp_code = add_code cpp_code
				"oflux::GuardTransMap __theGuardTransMap[] = {" in
		let cpp_code = emit_guard_trans_map (false,false,true)  conseq_res stable cpp_code in
		let cpp_code = List.fold_left add_code cpp_code 
				[ "{ NULL,0,0,0, NULL}  "; "};" ] in
                let cpp_code = emit_io_conversion_functions (Some pluginname) None conseq_res stable um cpp_code
		in  cpp_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code [""; "};"; "   //namespace"; "" ] in
	let cpp_code = emit_atom_fill (Some pluginname) stable node_atom_aliases cpp_code in
	let cpp_code = namespacefooter cpp_code in
        let cpp_code = List.fold_left add_code cpp_code
		( [ ""
                  ; "extern \"C\" {"
                  ; ffunmapname^" {"
                  ; "static oflux::ModularCreateMap mcm[] = {"
                  ; "{ \""^pluginname^"::\", "^pluginname^"::ofluximpl::__create_map_ptr },  "
                  ]
                @ (List.map (fun d -> "{ \""^d^"::\", "^d^"::ofluximpl::__create_map_ptr },  ") modules)
        
                @ [ "{ NULL, NULL }  "
                  ; "};"
                  ; "static oflux::flow::FunctionMaps ffm("
                  ; "       "^pluginname^"::ofluximpl::__conditional_map"
                  ; "     , mcm"
                  ; "     , "^pluginname^"::ofluximpl::__theGuardTransMap"
                  ; "     , "^pluginname^"::ofluximpl::__atomic_map_map"
                  ; "     , "^pluginname^"::ofluximpl::__ioconverter_map );"
                  ; "return &ffm;"
                  ; "}"
                  ; "}"
                  ]
                )
	in  h_code, cpp_code


let emit_cpp modulenameopt br um =
	let stable = br.Flow.symtable in
        let conseq_res = br.Flow.consequences in
	let fm = br.Flow.fmap in
	let ehs = br.Flow.errhandlers in
	let h_code = CodePrettyPrinter.empty_code in
	let file_suffix = get_module_file_suffix modulenameopt in
	let ns_broken,is_module = 
		match modulenameopt with
			None -> [], false
			| (Some nsn) -> break_namespaced_name nsn, true
		in
	let def_const_ns_fun csofar str = csofar^(String.capitalize str)^"__" in
	let def_const_ns = List.fold_left def_const_ns_fun "" ns_broken in
	let def_const_ns = 
		if String.length def_const_ns > 0 
		then "_"^def_const_ns
		else ""
		in
        let lc_codeprefix = CmdLine.get_code_prefix () in
        let uc_codeprefix = String.uppercase lc_codeprefix in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ "#ifndef _"^uc_codeprefix^def_const_ns
		; "#define _"^uc_codeprefix^def_const_ns
		; "#include \"mImpl"^file_suffix^".h\""
		] in
	let modules = br.Flow.modules in
        let module_filt m = 
                match modulenameopt with
                        None -> true
                        |(Some mm) -> not (m = mm) in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		(List.map (fun m -> "#include \""^lc_codeprefix^"_"^m^".h\"") 
                (List.filter module_filt modules)) in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ "#include \"OFlux.h\""
		; "#include \"OFluxEvent.h\""
		; "#include \"OFluxCondition.h\""
		; ""
		; ""
		; "// ---------- OFlux generated header (do not edit by hand) ------------"
		] in
	let namespaceheader code =
		match modulenameopt with
			None -> code
			| _ -> expand_namespace_decl_start code ns_broken
		in
	let namespacefooter code =
		match modulenameopt with
			None -> code
			| _ -> expand_namespace_decl_end code ns_broken
		in
	let h_code = namespaceheader h_code in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		(if is_module then [ "class ModuleConfig;" ] else []) in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ ""
		; "namespace ofluximpl {"
		; if is_module 
		  then "extern oflux::CreateMap * __create_map_ptr;" 
		  else "extern oflux::ModularCreateMap __master_create_map[];"
		; "extern oflux::CreateMap __create_map[];"
		; "extern oflux::ConditionalMap __conditional_map[];"
		; if is_module then "" else "extern oflux::AtomicMapMap __atomic_map_map[];"
		; if is_module then "" else "extern oflux::GuardTransMap __theGuardTransMap[];"
		; if is_module then "" else "extern oflux::IOConverterMap __ioconverter_map[];"
		; "}"
		; "    //namespace"
		; ""
		] in
	(*let _ = let ff n _ () = Debug.dprint_string (n^" guard sym(3)\n")
		in  SymbolTable.fold_guards ff stable () in*)
	let h_code = if is_module then h_code 
		     else emit_atom_map_decl stable h_code in
	(*let _ = let ff n _ () = Debug.dprint_string (n^" guard sym(4)\n")
		in  SymbolTable.fold_guards ff stable () in*)
	let is_concrete n = Flow.is_concrete stable fm n in
	(*let _ = let ff n _ () = Debug.dprint_string (n^" guard sym(5)\n")
		in  SymbolTable.fold_guards ff stable () in*)
	let h_code = emit_union_forward_decls conseq_res stable h_code in
	let h_code,ignoreis = emit_structs conseq_res stable h_code in
	let h_code = emit_atomic_key_structs stable h_code in
	let node_atom_aliases = gather_atom_aliases stable in
	let h_code = emit_node_atomic_structs None true stable node_atom_aliases h_code in
	let h_code = CodePrettyPrinter.add_cr h_code in
	let h_code = emit_node_func_decl None is_concrete ehs stable h_code in
	let h_code = if is_module then h_code 
		     else emit_guard_trans_map (true,false,false) 
			conseq_res stable h_code in
	let h_code = emit_cond_func_decl None stable h_code in
	let h_code = CodePrettyPrinter.add_cr h_code in
	let h_code = emit_unions None conseq_res stable h_code in
	let h_code = namespacefooter h_code in
        let h_code = emit_atomic_key_structs_hashfuns stable h_code modulenameopt in 
	let h_code = emit_converts false modulenameopt ignoreis conseq_res h_code in
        let h_code = emit_copy_to_functions None modulenameopt ignoreis conseq_res stable um h_code in
	let h_code = CodePrettyPrinter.add_code h_code ("#endif // _"^uc_codeprefix) in
	let cpp_code = CodePrettyPrinter.empty_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code
		[ "#include \""^lc_codeprefix^file_suffix^".h\""
		; "#include \"OFluxFlow.h\""
		; "#include \"OFluxAtomic.h\""
		; "#include \"OFluxAtomicHolder.h\""
		; "#include \"OFluxRunTime.h\""
		; "#include \"OFluxIOConversion.h\""
		; "#include \"OFluxLogging.h\""
		; "#include <iostream>"
		; ""
		; "// ---------- OFlux generated header (do not edit by hand) ------------"
		; ""
		] in
	let cpp_code = namespaceheader cpp_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code [""; "namespace ofluximpl {"; "" ] in
	let cpp_code = emit_create_map None is_concrete stable conseq_res ehs cpp_code in
	let cpp_code = if is_module then cpp_code else emit_master_create_map modules cpp_code in
	let cpp_code = emit_cond_map conseq_res stable cpp_code in
	let cpp_code = if is_module then cpp_code
		else
		let cpp_code = emit_atom_map_map None stable cpp_code in
		let cpp_code = emit_guard_trans_map (false,true,false)  conseq_res stable cpp_code in
		(*let max_guard_size = calc_max_guard_size stable in
		let cpp_code = add_code cpp_code
				("const int _argument_arr[]["
				^(string_of_int (max_guard_size+1))
				^"] = {") in
		let cpp_code = emit_guard_trans_map (false,false,true,false) conseq_res stable cpp_code in
		let cpp_code = List.fold_left add_code cpp_code [ "{ 0 }  "; "};" ] in *)
		let cpp_code = add_code cpp_code
				"oflux::GuardTransMap __theGuardTransMap[] = {" in
		let cpp_code = emit_guard_trans_map (false,false,true)  conseq_res stable cpp_code in
		let cpp_code = List.fold_left add_code cpp_code 
				[ "{ NULL,0,0,0, NULL}  "; "};" ] in
                let cpp_code = emit_io_conversion_functions None modulenameopt conseq_res stable um cpp_code
		in  cpp_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code [""; "};"; "   //namespace"; "" ] in
	let cpp_code = emit_atom_fill None stable node_atom_aliases cpp_code in
	let cpp_code = namespacefooter cpp_code in
	let cpp_code = 
		match modulenameopt with
			None -> emit_test_main cpp_code 
			| _ -> cpp_code
	in  h_code, cpp_code

