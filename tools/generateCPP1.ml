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

let emit_structs conseq_res symbol_table code =
	let struct_decl nclss =
		(nclss^" : public oflux::BaseOutputStruct<"
		    ^nclss^"> ") in
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
                                @ (e_one 1 inpl)
                                @ [ "};"; "" ] )
                in  match nd.nodeoutputs with
		    None -> code
		    | (Some outl) ->
			if avoid_struct (n,false) then code
			else List.fold_left add_code code
					( [ "struct "^(struct_decl (n^"_out"))^" {" ]
					@ (e_one 1 outl)
					@ [ "};"; "" ] )
		in
	let code = SymbolTable.fold_nodes e_s symbol_table code in
	let tos (n,isin) = if isin then n^"_in" else n^"_out" in
	let emit_type_def_alias code ((n,_) as is,what) =
		if has_namespaced_name n then
			code
		else add_code code ("typedef "^(tos what)^" "^(tos is)^";") in
	let emit_type_def_union (code,ignoreis) (i,ul) =
	    try let nsnio = List.find (fun (x,_) -> has_namespaced_name x) ul in
		let tosnsnio = tos nsnio in
		let typedeffun s = ("typedef "^tosnsnio^" "^s^";") in
		let typedeflist =
			List.map (fun y -> typedeffun (tos y))
				(List.filter (fun (yn,_) -> not (has_namespaced_name yn)) ul) in
		let typedeflist = (typedeffun ("OFluxUnion"^(string_of_int i)))::typedeflist
		in  (List.fold_left add_code code typedeflist,i::ignoreis)
	    with Not_found ->
		let one_typedef u = 
			("typedef OFluxUnion"^(string_of_int i)
			^" "^(tos u)^";") in
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
			    ^(struct_decl ("OFluxUnion"^(string_of_int i)))
			    ^" {" ]
			@ (e_one 1 decl)
			@ (e_two 1 decl)
			@ [ "};"; "" ]
			@ (List.map one_typedef ul)
			@ [ "" ] ) 
		   , ignoreis in
	let code = List.fold_left emit_type_def_alias code conseq_res.TypeCheck.aliases
	in  List.fold_left emit_type_def_union (code,[]) conseq_res.TypeCheck.full_collapsed

let emit_unions conseq_res symtable code =
        let collt_i = conseq_res.TypeCheck.full_collapsed in
	let avoid_is = List.map (fun (i,_) -> i) collt_i in
	let avoid_union i = List.mem i avoid_is in
	let e_one (fl_n,is_in) =
		let iostr = (if is_in then "_in" else "_out") in
		let cname = cstyle_name (break_namespaced_name fl_n)
		in  (fl_n^iostr^" _"^cname^iostr^";") in
	let e_union code ul =
		let i = TypeCheck.get_union_from_equiv conseq_res ul
		in  if avoid_union i then code
		    else 
			let nn,nn_isinp = List.hd ul in
			let cname = cstyle_name (break_namespaced_name nn) in
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
				@ [ "};"; "" ] )
			in  code
			in
	let code = TypeCheck.consequences_equiv_fold e_union code conseq_res
	in  code

let generic_cross_equiv_weak_unify code code_assignopt_fun symtable conseq_res =
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
        (*let code_template code (t_name,f_name) =
                let as_name (x,isin) = x^(if isin then "_in" else "_out")
                in  match get_copy_code t_name f_name with
                        None -> code
                        | (Some copy_code) -> 
                                let t_name' = nspace^(as_name t_name) in
                                let f_name' = nspace^(as_name f_name)
                                in  List.fold_left add_code code
                                        ([ "template<>"
                                        ; ("inline void copy_to<"^t_name'^", "
                                                ^f_name'^">("^t_name'^" * to, const "
                                                ^f_name'^" * from)")
                                        ; "{" ]
                                        @ copy_code @
                                        [ "}"
                                        ; "" ])
                in *)
        let equivc = List.map List.hd conseq_res.TypeCheck.equiv_classes in
        let cross_no_equal ll1 ll2 =
                let rec mfun ll x = 
                        match ll with 
                                (h::t) -> if x = h then mfun t x else (h,x)::(mfun t x)
                                | [] -> []
                in  List.concat (List.map (mfun ll1) ll2) in
        let code_assignopt_fun' code (a,b) = code_assignopt_fun code (get_copy_code a b,a,b) in
        let code = List.fold_left code_assignopt_fun' code (cross_no_equal equivc equivc)
        in  code
        

let emit_copy_to_functions modulenameopt _ conseq_res symtable code =
        let code = List.fold_left add_code code
                        [ "namespace oflux {  "
                        ; ""
                        ; "#ifndef OFLUX_COPY_TO_GENERAL"
                        ; "template<typename T,typename F>"
                        ; "inline void copy_to(T *to, const F * from)"
                        ; "{"
                        ; "enum { copy_to_code_failure_bug = 0 } _an_enum;"
                        ; "oflux::CompileTimeAssert<copy_to_code_failure_bug> cta;"
                        ; "}"
                        ; "#endif // OFLUX_COPY_TO_GENERAL"
                        ; "" ] in
	let nspace =
		match modulenameopt with
			None -> ""
			| (Some nn) -> nn^"::"
		in
        let code_template code (assign_opt,t_name,f_name) =
                let as_name (x,isin) = x^(if isin then "_in" else "_out")
                in  match assign_opt with
                        None -> code
                        | (Some copy_code) -> 
                                let t_name' = nspace^(as_name t_name) in
                                let f_name' = nspace^(as_name f_name)
                                in  List.fold_left add_code code
                                        ([ "template<>"
                                        ; ("inline void copy_to<"^t_name'^", "
                                                ^f_name'^">("^t_name'^" * to, const "
                                                ^f_name'^" * from)")
                                        ; "{" ]
                                        @ copy_code @
                                        [ "}"
                                        ; "" ])
                in
        let code = generic_cross_equiv_weak_unify code code_template symtable conseq_res
	in  List.fold_left add_code code [ ""; "};"; "   //namespace" ]

let emit_io_conversion_functions conseq_res symtable code =
        let code = add_code code "oflux::IOConverterMap __ioconverter_map[] = {" in
        let code_table_entry code (assign_opt,t_name,f_name) =
                let as_name (x,isin) = x^(if isin then "_in" else "_out")
                in  match assign_opt with
                        None -> code
                        | (Some _) -> 
                                let t_u_n = TypeCheck.get_union_from_strio conseq_res t_name in
                                let f_u_n = TypeCheck.get_union_from_strio conseq_res f_name in
                                let tstr = as_name t_name in
                                let fstr = as_name f_name in
                                let t_u_n_str = string_of_int t_u_n in
                                let f_u_n_str = string_of_int f_u_n 
                                in  add_code code ("{ "^f_u_n_str^", "^t_u_n_str^", &oflux::create_real_io_conversion<"^tstr^", "^fstr^" > }, ")
                in
        let code = generic_cross_equiv_weak_unify code code_table_entry symtable conseq_res
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
			  ; "};" ])
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
	

let emit_node_atomic_structs readonly symtable aliases code =
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
		in  try (name, lookup_return_type name_canon, is_ro) 
		    with Not_found -> raise (CppGenFailure ("can't find guard "^name^" at "^"l:"^(string_of_int pos.lineno)^" c:"^(string_of_int pos.characterno)))
		in
	let inits nl = List.map (fun n -> "_"^n^"(NULL)") nl in
	let decls ntl = List.map (fun (n,t,_) -> 
		t^" * _"^n^";") ntl in
	let accessor (n,t,iro) =
		let rop = if iro then ro_prefix else ""
		in rop^t^(if iro then " " else " & ")^n^"() "
			^rop^" { return *_"^n^"; }  " in
	let accessors ntl = List.map accessor ntl in
	let e_one n nd code =
	    if has_dot n then code
	    else
		let nf = nd.functionname in 
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
				@ [ "void fill(oflux::AtomicsHolder * ah);"
				  ; "private:"
				  ]
				@ (decls ntl)
				@ [ "};" ] )
	in  SymbolTable.fold_nodes e_one symtable code

let emit_atom_fill symtable aliases code =
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
	let code_for_one (cl,i) (n,t) =
		(("_"^n^" = reinterpret_cast<"^t^" *> (ah->get("
		^(string_of_int i)^",false)->atomic()->data());")::cl), i+1 in
	let e_one n nd code =
		try if has_dot n then code 
		    else let _ = List.assoc n aliases
		        in code
		with Not_found ->
			let ntl = List.map nt_of_gr nd.nodeguardrefs in
			let code = List.fold_left add_code code
				[ "void "^n^"_atoms::fill(oflux::AtomicsHolder * ah)"
				; "{"
				] in
			let codelist, _ = List.fold_left code_for_one ([],0) ntl in
			let code = List.fold_left add_code code codelist
			in  add_code code "}"  
	in  SymbolTable.fold_nodes e_one symtable code

let emit_atom_map_decl symtable code =
	let e_one n gd code =
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
	

let emit_atom_map_map symtable code =
	let e_one n gd code =
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
				| _ -> raise (CppGenFailure ("unsupported guard type "^gd.gtype))
		in  (if 0 = (List.length gd.garguments) then
			("oflux::AtomicMapTrivial<"
				^atomic_class_str^"> "^clean_n^"_map;")
		    else
			("oflux::AtomicMapStdMap<"^clean_n^"_key"
				^","^atomic_class_str^"> "^clean_n^"_map;")
			))::[ "oflux::AtomicMapAbstract * "^clean_n
                                ^"_map_ptr = &"^clean_n^"_map;" ])
		in
	let e_two n gd codelist =
		let clean_n = clean_dots n
		in  ("{ \""^n^"\", &"^clean_n^"_map }, ")::codelist in
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

	

let emit_guard_trans_map (with_proto,with_code,with_argnos,with_map) 
                conseq_res symtable code =
	(* trick is to determine all possible unifications 
	   and pop them in this table.
	*)
	let get_u_n x = TypeCheck.get_union_from_strio conseq_res (x,true) in
	let one_inst garg gn nn nf (code,line) pi =
                if pi = [] then code,line
                else
		let alist = List.map (fun (_,i) -> (string_of_int i)^", ") pi in
		let cat a b = a^b in
		let arglist = "{ "^(List.fold_right cat alist "0 }, ") in
		let gpi = List.combine garg pi in
		let ff str (_,i) = str^"_"^(string_of_int i) in
		let pistr = List.fold_left ff "_" pi in
		let translate_name = clean_dots ("g_trans_"^gn^"_"^nn^pistr) in
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
		    ), line+1
		in
        let e_g gn gd (code,line) =
                let e_n nn nd (code,line) =
			let nf = nd.functionname in
			let pio = possible_instances_of gd.garguments nd.nodeinputs in  
                        let code,line = List.fold_left (one_inst gd.garguments gn nn nf) (code,line) pio
                        in  code,line
                in  SymbolTable.fold_nodes e_n symtable (code,line)
                in
        let code,_ = SymbolTable.fold_guards e_g symtable (code,0)
        in  code


let emit_node_func_decl is_concrete errorhandlers symtable code =
	let is_eh n = List.mem n errorhandlers in
	let e_one n _ code =
		let nd = SymbolTable.lookup_node_symbol symtable n
		in
		if (is_concrete n) && (not (has_dot n)) then
			add_code code
			("int "^nd.functionname^"(const "^n^"_in *, "^n
			^"_out *, "^n^"_atoms *"
			^(if is_eh n then ", int" else "")
			^");")
		else code
	in  SymbolTable.fold_nodes e_one symtable code

let emit_cond_func_decl symtable code =
	let e_one n cond code =
		match cond.SymbolTable.arguments with
			[d] ->
			    if (has_dot n) || ((List.length(break_namespaced_name n)) != 1) then code
			    else
				let tm = d.ctypemod in
				let t = d.ctype in
				let arg = d.name
				in
				add_code code
				("bool "^n^"("^(strip_position tm)
				^" "^(strip_position t)
				^" "^(strip_position arg)^");")
			| _ -> raise (CppGenFailure ("emit_cond_func_decl -internal"))
	in SymbolTable.fold_conditionals e_one symtable code

let emit_create_map is_concrete symtable conseq_res ehs code =
	let is_eh f = List.mem f ehs in
	let lookup_union_number (n,isin) =
		try TypeCheck.get_union_from_strio conseq_res (n,isin)
		with Not_found -> raise (CppGenFailure ("emit_create_map: not found union "
			^n^" "^(if isin then "input" else "output")))
		in
        let is_abstract n = SymbolTable.is_abstract symtable n in
	let e_one n _ code =
		if has_dot n then code else 
			let nd = SymbolTable.lookup_node_symbol symtable n in
			let nf = nd.functionname
			in if (is_concrete n) && not (is_abstract n) then
				let u_no_in = lookup_union_number (nf,true) in
				let u_no_out = lookup_union_number (nf,false) 
				in  add_code code ("{ \""^nf
					^"\", &oflux::create"
					^(if is_eh n then "_error" else "")
					^"<OFluxUnion"^(string_of_int u_no_in)^", "
					^"OFluxUnion"^(string_of_int u_no_out)^", "
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

let emit_converts modulenameopt ignoreis conseq_res code =
        let collapsed_types_i = conseq_res.TypeCheck.full_collapsed in
        let aliased_types = conseq_res.TypeCheck.aliases in
	let nspace =
		match modulenameopt with
			None -> ""
			| (Some nn) -> nn^"::"
		in
	let avoid ll i = 
		try let _ = List.find (fun (ii,_) -> i=ii) ll
		    in true
		with Not_found -> false in
	let avoid_n x = avoid aliased_types x in
	let code = List.fold_left add_code code
		[ "namespace oflux {  "
		; ""
		; "#ifndef OFLUX_CONVERT_GENERAL"
		; "#define OFLUX_CONVERT_GENERAL"
		; "template<typename G,typename H>"
		; "inline H * convert(G *)"
		; "{"
		; "enum { general_conversion_not_supported_bug = 0 } _an_enum;"
		; "oflux::CompileTimeAssert<general_conversion_not_supported_bug> cta;"
		; "}"
		; ""
		; "template<typename G,typename H>"
		; "inline H const * const_convert(G const * g)"
		; "{"
		; "return convert<G,H>(const_cast<G*>(g));"
		; "}"
		; "#endif //OFLUX_CONVERT_GENERAL"
		; ""
		] in 
	let code_template code u_name s_name isderef =
                let broken_name = break_namespaced_name s_name in
		let cs_name = cstyle_name broken_name
		in  if (List.length broken_name) = 1 then
                        List.fold_left add_code code 
			[ "template<>"
			; ("inline "^nspace^s_name^" * convert<"^nspace^u_name^", "
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
		let u_name = "OFluxUnion"^(string_of_int u_n)
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
		; "using namespace oflux;"
		; "RunTime * theRT = NULL;"
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
		; "FlowFunctionMaps ffmaps(ofluximpl::__conditional_map, ofluximpl::__master_create_map, ofluximpl::__theGuardTransMap, ofluximpl::__atomic_map_map, ofluximpl::__ioconverter_map);"
		; "RunTimeConfiguration rtc = {"
		; "  1024*1024 // stack size"
		; ", 1 // initial threads (ignored really)"
		; ", 0 // max threads"
		; ", 0 // max detached"
		; ", 0 // thread collection threshold"
		; ", 1000 // thread collection sample period (every N node execs)"
		; ", argv[1] // XML file"
		; ", &ffmaps"
		; "};"
		; "RunTime rt(rtc);"
		; "theRT = &rt;"
		; "signal(SIGHUP,handlesighup); // install SIGHUP handling"
		; "loggingToStream(std::cout); // comment out if no oflux logging is desired"
		; "rt.start();"
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

let emit_cpp modulenameopt br =
	let stable = br.Flow.symtable in
        let conseq_res = TypeCheck.consequences br.Flow.ulist stable in
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
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ "#ifndef _OFLUX_GENERATED"^def_const_ns
		; "#define _OFLUX_GENERATED"^def_const_ns
		; "#include \"mImpl"^file_suffix^".h\""
		] in
	let modules = br.Flow.modules in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		(List.map (fun m -> "#include \"OFluxGenerate_"^m^".h\"") modules) in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ "#include \"OFlux.h\""
		; "#include \"OFluxEvent.h\""
		; "#include \"OFluxCondition.h\""
		] in
	let h_code = List.fold_left CodePrettyPrinter.add_code h_code
		[ ""
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
	let h_code,ignoreis = emit_structs conseq_res stable h_code in
	let h_code = emit_atomic_key_structs stable h_code in
	let node_atom_aliases = gather_atom_aliases stable in
	let h_code = emit_node_atomic_structs true stable node_atom_aliases h_code in
	let h_code = CodePrettyPrinter.add_cr h_code in
	let h_code = emit_node_func_decl is_concrete ehs stable h_code in
	let h_code = if is_module then h_code 
		     else emit_guard_trans_map (true,false,false,false) 
			conseq_res stable h_code in
	let h_code = emit_cond_func_decl stable h_code in
	let h_code = CodePrettyPrinter.add_cr h_code in
	let h_code = emit_unions conseq_res stable h_code in
	let h_code = namespacefooter h_code in
	let h_code = emit_converts modulenameopt ignoreis conseq_res h_code in
        let h_code = emit_copy_to_functions modulenameopt ignoreis conseq_res stable h_code in
	let h_code = CodePrettyPrinter.add_code h_code "#endif // _OFLUX_GENERATED" in
	let cpp_code = CodePrettyPrinter.empty_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code
		[ "#include \"OFluxGenerate"^file_suffix^".h\""
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
	let cpp_code = emit_create_map is_concrete stable conseq_res ehs cpp_code in
	let cpp_code = if is_module then cpp_code else emit_master_create_map modules cpp_code in
	let cpp_code = emit_cond_map conseq_res stable cpp_code in
	let cpp_code = if is_module then cpp_code
		else
		let cpp_code = emit_atom_map_map stable cpp_code in
		let cpp_code = emit_guard_trans_map (true,true,false,false)  conseq_res stable cpp_code in
		let max_guard_size = calc_max_guard_size stable in
		let cpp_code = add_code cpp_code
				("const int _argument_arr[]["
				^(string_of_int (max_guard_size+1))
				^"] = {") in
		let cpp_code = emit_guard_trans_map (false,false,true,false) conseq_res stable cpp_code in
		let cpp_code = List.fold_left add_code cpp_code [ "{ 0 }  "; "};" ] in
		let cpp_code = add_code cpp_code
				"oflux::GuardTransMap __theGuardTransMap[] = {" in
		let cpp_code = emit_guard_trans_map (false,false,false,true)  conseq_res stable cpp_code in
		let cpp_code = List.fold_left add_code cpp_code 
				[ "{ NULL,0,0,NULL, NULL}  "; "};" ] in
                let cpp_code = emit_io_conversion_functions conseq_res stable cpp_code
		in  cpp_code in
	let cpp_code = List.fold_left CodePrettyPrinter.add_code cpp_code [""; "};"; "   //namespace"; "" ] in
	let cpp_code = emit_atom_fill stable node_atom_aliases cpp_code in
	let cpp_code = namespacefooter cpp_code in
	let cpp_code = 
		match modulenameopt with
			None -> emit_test_main cpp_code 
			| _ -> cpp_code
	in  h_code, cpp_code, conseq_res

