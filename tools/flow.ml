(* capture the flow *)

open ParserTypes

type flow = 
	Source of string * string * flow ref ref * flow ref ref
	| ChExpr of string * (string option list * flow ref ref) list 
		(* abstract nodes *)
	| ConExpr of string * (flow ref ref) list 
		(* abstract nodes *)
	| CNode of string * string * flow ref ref * flow ref ref 
		(* non source, error handler *)
	| NullNode

let null_flow = NullNode

let dprint_string = Debug.dprint_string
let debug () = ! Debug.debug

let flow_apply (sourcefn,chexprfn,conexprfn,cnodefn,nullfn) fl =
	let deref_one (sol,frr) = (sol,! !frr) 
	in  match fl with
		(Source (s1,s2,f1,f2)) -> sourcefn s1 s2 (! ! f1) (! ! f2)
		| (ChExpr (n,ll)) -> chexprfn n (List.map deref_one ll)
		| (ConExpr (n,ll)) -> conexprfn n 
                        (List.map (fun frr -> ! !frr) ll)
		| (CNode (s1,s2,f1,f2)) -> cnodefn s1 s2 (! ! f1) (! ! f2)
		| NullNode -> nullfn ()



let get_name fl = 
	match fl with 
		(Source (n,_,_,_)) -> n
		| (ChExpr (n,_)) -> n
		| (ConExpr (n,_)) -> n
		| (CNode (n,_,_,_)) -> n
		| NullNode -> "<-nullnode->"


module StringKey =
struct
	type t = string
	let compare = compare
end

module FlowMap = Map.Make(StringKey)

type flowmap = flow ref FlowMap.t

let pp_flow_map debug flowmap =
	if not debug then ()
	else
	let con2 a b = a^b in
	let strconcat ll = List.fold_left con2 "" ll in
	let strconcatmap f ll = strconcat (List.map f ll) in
	let citem so =
		match so with 
			None -> "_, "
			| (Some s) -> (s^", ") in
	let lookup_map = 
		let f s fl ll = (fl,s)::ll
		in  FlowMap.fold f flowmap [] in
	let lookup_fnode flr =
		let rec assoc_spec flr ll =
			match ll with
				((h,r)::t) -> if h == flr then r 
					      else assoc_spec flr t
				| _ -> raise Not_found
		in  try assoc_spec flr lookup_map
		    with Not_found -> "" in
	let get_extra flr = "("^(lookup_fnode flr)^")" in
	let get_name flr = (get_name (!flr))^(get_extra flr) in
	let mitrf name (sol,f) =
		(name^"["^(strconcatmap citem sol)^"] : "^(get_name (!f))^"\n") in
        let mitrf2 _ f = (get_name (!f))^" & " in
	let iterf k fl =
		k^" := "
		^(match !fl with
			NullNode -> " NullNode "
			| (Source (n,f,t,e)) ->
				(" Source( "^n^", "^f^", "
				^(get_name (!t))^", "^(get_name (!e))^") ")
			| (CNode (n,f,t,e)) ->
				(" CNode( "^n^", "^f^", "
				^(get_name (!t))^", "^(get_name (!e))^") ")
			| (ConExpr (n,fll)) ->
				(n^" : "^(strconcatmap (mitrf2 n) fll)
                                ^(get_extra fl)^"\n")
			| (ChExpr (n,solfl)) ->
				((strconcatmap (mitrf n) solfl)
                                ^(get_extra fl))) in
	let print k fl = print_string ((iterf k fl)^"\n")
	in  FlowMap.iter print flowmap

let is_concrete stable fm n =
	try (match !(FlowMap.find n fm) with
		(Source (_,_,_,_)|CNode (_,_,_,_)) -> 
			let nd = SymbolTable.lookup_node_symbol stable n
			in  not (nd.SymbolTable.nodeoutputs = None)
		| _ -> false)
	with Not_found -> false

let is_null_node fl =
	match fl with
		NullNode -> true
		| _ -> false

let flowmap_fold f fm a = 
	let f' s a b = f s (!a) b
	in  FlowMap.fold f' fm a

let flowmap_diff fma fmb =
        let to_list fm = 
                let f s fl ll = (s,! fl)::ll
                in  FlowMap.fold f fm [] in
        let lla = to_list fma in
        let llb = to_list fmb in
        let name_of_rr f = get_name (! ! f) in
        let compare_flow f1 f2 =
                match f1,f2 with
                        (Source (n11,n12,f11,f12), Source (n21,n22,f21,f22)) ->
                                compare (n11,n12,name_of_rr f11,name_of_rr f12)
                                        (n21,n22,name_of_rr f21,name_of_rr f22)
                        | (CNode (n11,n12,f11,f12), CNode (n21,n22,f21,f22)) ->
                                compare (n11,n12,name_of_rr f11,name_of_rr f12)
                                        (n21,n22,name_of_rr f21,name_of_rr f22)
                        | (ConExpr (n1,frrl1),ConExpr (n2,frrl2)) ->
                                let on_list frrl = List.sort compare (List.map name_of_rr frrl)
                                in  compare (n1,on_list frrl1) (n2,on_list frrl2)
                        | (ChExpr (n1,sofrrl1),ChExpr (n2,sofrrl2)) ->
                                let on_item (sol,frr) = (sol,name_of_rr frr) in
                                let on_list sofrrl = List.map on_item sofrrl
                                in  compare (n1,on_list sofrrl1)
                                            (n2,on_list sofrrl2)
                        | (NullNode,NullNode) -> 0
                        | (NullNode,_) -> 0-1
                        | (_,NullNode) -> 1
                        | (Source _,_) -> 0-1
                        | (_,Source _) -> 1
                        | (CNode _,_) -> 0-1
                        | (_,CNode _) -> 1
                        | (ChExpr _,_) -> 0-1
                        | (_,ChExpr _) -> 1 in
        let rec merge lla llb =
                match lla,llb with
                        ((han,haf)::ta,(hbn,hbf)::tb) ->
                                let c = compare han hbn
                                in  if c < 0 then 
                                        (Some haf,None)::(merge ta llb)
                                    else if c > 0 then
                                        (None,Some hbf)::(merge lla tb)
                                    else 
                                        let cf = compare_flow haf hbf
                                        in  if cf = 0 then merge ta tb
                                            else (Some haf,Some hbf)::(merge ta tb)
                        | ([],(_,hf)::t) -> (None,Some hf)::(merge [] t)
                        | ((_,hf)::t,[]) -> (Some hf,None)::(merge t [])
                        | _ -> []
        in  merge lla llb

let flowmap_find n fm = !(FlowMap.find n fm)
	
let rec find_error_handlers fmap froml =
	match froml with
		(h::tl) -> 
			h::(match flowmap_find h fmap with
				(ChExpr (_,ll)) -> 
					let eh = List.map (fun (_,frr) -> get_name (! ! frr)) ll
					in  (find_error_handlers fmap eh)
					    @ (find_error_handlers fmap tl)
                                | (ConExpr (_,ll)) ->
                                        let eh = List.map (fun frr -> get_name (! !frr)) ll 
                                        in  (find_error_handlers fmap eh)
                                            @ (find_error_handlers fmap tl)
				| _ -> find_error_handlers fmap tl)
		| _ -> []


exception Failure of string * position

let ref_null() = ref NullNode
let ref_ref_null() = ref (ref NullNode)

let empty_flow_map = (*FlowMap.add "error" (ref_null ())*) FlowMap.empty

let build_map fmap decls =
	let build_single fmap nd =
		FlowMap.add (strip_position nd.nodename) (ref_null ()) fmap
	in  List.fold_left build_single fmap decls

let check_guard_refs stable =
	(* check that the guards arguments are typed properly *)
	let asdf (sp,df) = 
		let _, pos, _ = df.name 
		in      ( strip_position df.ctypemod
			, strip_position df.ctype
			, sp ), pos in
	let e_one n nd () =
		let map_node_arg_type n = 
			let df = List.find (fun x -> (strip_position x.name) = n) nd.SymbolTable.nodeinputs 
			in SymbolTable.strip_position3 df in
		let check_gr gr =
			let gname,gpos,_ = gr.guardname in
			let gd = 
                            try SymbolTable.lookup_guard_symbol stable gname
                            with Not_found ->
                                 raise (Failure ("guard symbol "^gname^" undefined",gpos))
			in  try List.map asdf 
				(List.combine gr.arguments gd.SymbolTable.garguments)
			    with (Invalid_argument _) ->
			    	raise (Failure ("# guard args wrong for "^gname,gpos))
			in
		let ngrs = nd.SymbolTable.nodeguardrefs in
                let on_each matchpair =
                        match matchpair with 
                                ((a,b,[Arg nn]), pos) -> 
					(match Unify.unify_single (map_node_arg_type nn) (a,b,nn) with
						None -> ()
						| (Some r) ->
							raise (Failure ("cannot unify guard argument - "^r,pos)))
                                | _ -> () (*with context no typing*)
		in  List.iter (fun gr -> List.iter on_each (check_gr gr)) ngrs
	in  SymbolTable.fold_nodes e_one stable ()


let add_named symtable fmap = 
	let iterf n fl sinks =
		let nd = SymbolTable.lookup_node_symbol symtable n
		in  if n != "error" then
			match !fl with
				NullNode -> 
					dprint_string ("iterf on "^n^"\n");
					fl := CNode(n,nd.SymbolTable.functionname,ref_ref_null (),ref_ref_null ());
					(n::sinks)
				| _ -> sinks
		    else sinks
		in
	let iterf2 n fl sinks =
		if n != "error" then
			match !fl with
				(Source (_,_,succ,_)|CNode (_,_,succ,_)) ->
					if (! !succ) = NullNode then
						(n::sinks)
					else sinks
				| _ -> sinks
		else sinks in
	let res = FlowMap.fold iterf fmap [] in
	let _ = dprint_string "AFTER ADD NAMED FMAP(part1)\n"; pp_flow_map (debug()) fmap 
	in  FlowMap.fold iterf2 fmap res


let add_main_fn symtable fmap unified mainfun =
	let (_,srcpos,_) as name = mainfun.sourcename in
	let fl_n = strip_position name in
	let fl_n_f = mainfun.sourcefunction in
	let sfl_n_opt, unified = 
		match mainfun.successor with
			None -> (None , unified)
			| (Some succ) -> 
				let sfl_n = strip_position succ
				in (Some sfl_n
				   , TypeCheck.type_check unified symtable sfl_n fl_n srcpos )
	in      try let sfl = match sfl_n_opt with
				None -> ref_null()
				| (Some sfl_n) -> FlowMap.find sfl_n fmap in
		    let fl = FlowMap.find fl_n fmap in
		    let _ = fl := Source (fl_n,fl_n_f,ref sfl,ref_ref_null ())
		    in unified
		with Not_found -> 
		    raise (Failure ("Node "^fl_n
			^(match sfl_n_opt with
				None -> ""
				| (Some sfl_n) -> (" or "^sfl_n))
			^" undeclared",srcpos))

let add_conditionals symtable expr = (* returns a new symtable *)
	let n = expr.exprname in
	let cond = expr.condbinding in
	if cond = [] then symtable
	else
		let n,npos,_ = n in
		let decll =
			try let nd = SymbolTable.lookup_node_symbol symtable n
			    in  nd.SymbolTable.nodeinputs
			with Not_found ->
				raise (Failure ("cannot find node definition "^n,npos))
			in
		let onl = try List.combine decll cond
			with (Invalid_argument _) ->
				raise (Failure ("mismatch on expression "^n^" argument count",npos)) in
		let unify pos ty1 ty2 =
			match Unify.unify_single ty1 ty2 with
				None -> ()
				| (Some msg) ->
					raise (Failure ("type unify failure for conditional. "^msg,pos)) in
		let a_c (argno,symtable) spo =
			match spo with
				(ty,Ident ((s,spos,_) as cond)) ->
					(try let conds = SymbolTable.lookup_conditional_symbol symtable s in
					     let ty' = 
					     	match conds.SymbolTable.arguments with
							[decl] -> SymbolTable.strip_position3 decl
							| _ -> raise Not_found in
					     let sty = SymbolTable.strip_position3 ty
					     in unify spos sty ty'; (argno+1,symtable)
					with Not_found ->
						(argno+1,SymbolTable.add_conditional symtable { externalcond=false; condname=cond; condfunction=s; condinputs=[ty] }))
				| (_,Star) -> (argno+1, symtable)
			in
		let _, symtable = List.fold_left a_c (1,symtable) onl
		in  symtable


let add_concurrent_expr symtable fmap unified expr =
        let ident_n, identpos, _ = expr.exprname in
        let tounify_names = List.map strip_position
                expr.successors in
        let type_check_concur_local unified ename =
                TypeCheck.type_check_concur unified symtable ident_n ename identpos in
        let unified = List.fold_left type_check_concur_local unified tounify_names in
	let getfl n =
		try FlowMap.find n fmap
		with Not_found -> raise (Failure ("node "^n^" referenced in the flow not found",identpos)) in
        let efl = getfl ident_n in
        let _ = efl := ConExpr(ident_n,List.map
                        (fun n -> ref (getfl n)) tounify_names)
        in unified

let add_choice_expr symtable fmap unified expr =
	let ident_n, identpos, _ = expr.exprname in
	let cond = expr.condbinding in
	let conseq = expr.successors in
	let rec type_check_all unified ll = 
		match ll with
			((h1,hpos,_)::((h2,_,_) as hh)::t) -> 
				let unied = TypeCheck.type_check unified symtable h2 h1 hpos
				in type_check_all unied (hh::t)
			| _ -> unified in
	let lookup_node pos n =
		try SymbolTable.lookup_node_symbol symtable n
		with Not_found -> raise (Failure ("cannot find node definition "^n,pos)) in
	let nd = lookup_node identpos ident_n in
	(*let in_param = nd.SymbolTable.nodeinputs in*)
	let out_param = nd.SymbolTable.nodeoutputs in
	let conseq_in, conseq_out, first_n, last_n = 
		match conseq, List.rev conseq with
			(first::_,last::_) ->
				let first_n,firstpos,_ = first in
				let last_n,lastpos,_ = last in
				let firstnd = lookup_node firstpos first_n in
				let lastnd = lookup_node lastpos last_n
				in firstnd.SymbolTable.nodeinputs
				   ,lastnd.SymbolTable.nodeoutputs 
				   ,first_n
				   ,last_n
			| _ -> raise (Failure("expression must have consequences",identpos)) in
	let unified = type_check_all unified conseq in
	let unified =
                TypeCheck.type_check_inputs_only unified symtable ident_n first_n identpos in
	let unified = 
		match out_param, conseq_out with
			(Some op,Some co) ->
                                TypeCheck.type_check_outputs_only unified symtable ident_n last_n identpos 
                        | _ -> unified in
	let fl = FlowMap.find ident_n fmap in
	let _ = dprint_string "DEBUGGING:\n"; 
		dprint_string ("on "^ident_n^"\n");
		pp_flow_map (debug ()) fmap in
	let n, lines = 
		match !fl with
			NullNode -> ident_n, []
			| (ChExpr (n,ll)) -> n,ll
			| _ -> raise (Failure ("the node "^ident_n^" is not an expression",identpos)) in
	let convert_cond_part c =
		match c with 
			Star -> None
			| (Ident (s,_,_)) -> Some s in
	let ncond = List.map convert_cond_part cond in
	let getfl (n,_,_) =
		try n, FlowMap.find n fmap
		with Not_found -> raise (Failure ("node "^n^" referenced in the flow not found",identpos)) in
	let rec commit_conseq ((r_n:flow ref),isfirstconseq) ll =
		let errfl = (*FlowMap.find "error" fmap*) ref_null() in
		let rec addtoend nll (r_n:flow ref) flow =
			if NullNode = !r_n then ()
			else
			match flow with
				(CNode (cn,_,frr,_)) ->
					let _ = if List.mem cn nll then
						raise (Failure ("cycle on node "^cn,identpos))
						else ()
					in
					if (! !frr) = NullNode then
						frr := r_n
					else
						addtoend (cn::nll) r_n (! !frr)
				| (ChExpr (en,solfrl)) -> 
					let _ = if List.mem en nll then
						raise (Failure ("cycle on node "^en,identpos))
						else ()
					in 
					List.iter (fun (_,frr) -> 
						if (! !frr) = NullNode then
							frr := r_n
						else 
							addtoend (en::nll) r_n (! ! frr) )
					solfrl
				| (ConExpr (en,frl)) -> 
					let _ = if List.mem en nll then
						raise (Failure ("cycle on node "^en,identpos))
						else ()
					in 
					List.iter (fun frr -> 
						if (! !frr) = NullNode then
							frr := r_n
						else 
							addtoend (en::nll) r_n (! ! frr) )
					frl
				| _ -> raise (Failure ("add_expr...addtoend unexpected", identpos))
		in  match ll with 
			(h::t) ->
				(*let _ = dprint_string ("DEBUGGING... detail(bef) "^(strip_position h)^":\n"); 
					pp_flow_map (debug ()) fmap in*)
				let nh,fl = getfl h in
				let nd = lookup_node noposition nh
				in  (match (!fl) with
					(ChExpr (_,_)) ->
						(*let _ = dprint_string ("doing addtoend...."^n^"\n") in*)
						addtoend [n] r_n (!fl)
					| (ConExpr (_,_)) ->
						addtoend [n] r_n (!fl)
					| NullNode ->
						(*let _ = dprint_string ("doing null node....\n") in*)
						if not isfirstconseq 
						then fl := CNode (nh,nd.SymbolTable.functionname, ref r_n, ref errfl)
						else ()
					| (CNode (cname,_,frr,errfl)) ->
						(*let _ = dprint_string ("doing cnode...."^cname^"\n") in*)
						if not ((! !frr) = NullNode) then
							raise (Failure ("node "^nh^" already has a sucessor", identpos))
						else 
							fl := CNode(nh,nd.SymbolTable.functionname, ref r_n,errfl)
					| _ -> raise (Failure ("successor expression cannot use a source", identpos))
					); 
					(*dprint_string ("DEBUGGING... detail(aft) "^(strip_position h)^":\n"); 
					pp_flow_map (debug ()) fmap;*)
					commit_conseq (fl,false) t
			| [] -> ()
		in
	let _ = commit_conseq (ref_null(),true) (List.rev conseq) in
	let head_n, headconseqfl = getfl (List.hd conseq) in
	let _ = fl := ChExpr (n, (ncond,ref headconseqfl)::lines)
	in  unified

let add_expr symtable fmap unified expr =
        (match expr.etype with
                Choice -> add_choice_expr
                | Concurrent -> add_concurrent_expr)
                        symtable fmap unified expr

let add_error symtable fmap unified err =
	let nodes = err.onnodes in
	let handler = err.handler in
	let hname,hpos,_ = handler in
	let h = try FlowMap.find hname fmap 
		with Not_found -> raise (Failure ("error handler "^hname
			^" not declared as a node",hpos)) in
	let rec follow_concrete h =
		match ! h with
			(CNode (_,_,_,_) | NullNode) -> h
			| (ChExpr (ern,[_,frr])) -> (! frr)
			| (ConExpr (ern,[frr])) -> (! frr)
			| _ -> raise (Failure ("error handler is a conditional expression - not allowed - "^hname,hpos))
		in
	let h = follow_concrete h in
	let a_e unified node =
		let n,npos,_ = node in
		let fl = FlowMap.find n fmap
		in  match !fl with
			((Source (a,_,b,hr)) | (CNode (a,_,b,hr))) -> 
				let _ = hr := h
				in  TypeCheck.type_check_general (true,true) unified symtable hname n npos
			| _ -> raise (Failure ("trying to bind error handler to non-concrete node "^n,npos))
	in  List.fold_left a_e unified nodes


type built_flow = 
		{ sources : string list
		; fmap : flowmap
		; ulist : TypeCheck.unification_result
		; symtable : SymbolTable.symbol_table
		; errhandlers : string list
		; modules : string list
                ; terminates : string list
                ; runoncesources : string list
                ; consequences : TypeCheck.consequence_result
                ; guard_order_pairs : (string * string) list
		}

let build_flow_map symboltable node_decls m_fns exprs errs terms mod_defs order_decls =
        let is_not_term n = not (List.mem n terms) in
	let verify_ss (projfun,ntype) symboltable n = 
		let nd = SymbolTable.lookup_node_symbol symboltable n
		in  if List.length (projfun nd) > 0 then
			raise (Failure (ntype^" "^n^" does not have i/o type ()",nd.SymbolTable.where))
		    else ()
		in
	let get_conc_outputs n nd =
		match nd.SymbolTable.nodeoutputs with
			None -> raise (Failure ("can't verify sink "^n,nd.SymbolTable.where))
			| (Some x) -> x
		in
	let verify_sink symboltable n = 
		verify_ss ((fun x -> get_conc_outputs n x), "sink") symboltable n in
	let verify_source symboltable n = 
		verify_ss ((fun x -> x.SymbolTable.nodeinputs), "source") symboltable n in
	let flowmap = empty_flow_map in
	let flowmap = build_map flowmap node_decls in
	let debug = debug () in
	let _ = dprint_string "INITIAL FMAP\n"; pp_flow_map debug flowmap in
	let unified = TypeCheck.empty in
	let unified = List.fold_left (add_expr symboltable flowmap) unified (List.rev exprs) in
	let _ = dprint_string "AFTER EXPRs FMAP\n"; pp_flow_map debug flowmap in
	let unified = List.fold_left (add_main_fn symboltable flowmap) unified m_fns in
	let _ = dprint_string "AFTER MAINs FMAP\n"; pp_flow_map debug flowmap in
	let symboltable = List.fold_left add_conditionals symboltable exprs in
	let _ = dprint_string "AFTER CONDs FMAP\n"; pp_flow_map debug flowmap in
	let is_not_abstract n =
		let nd = SymbolTable.lookup_node_symbol symboltable n
		in  not (nd.SymbolTable.nodeoutputs = None) in
	let sinks = List.filter is_not_abstract (add_named symboltable flowmap) in
        let sinks = List.filter is_not_term sinks in
	let _ = dprint_string "AFTER ADD NAMED FMAP\n"; pp_flow_map debug flowmap in
	let _ = List.iter (verify_sink symboltable) sinks
                        (*| _ -> ()*) in
	let unified = List.fold_left (add_error symboltable flowmap) unified errs in
	let _ = dprint_string "AFTER ERRs FMAP\n"; pp_flow_map debug flowmap in
	let error_handlers = find_error_handlers flowmap
		(List.map (fun e -> strip_position e.handler) errs) in
	let sources = List.map (fun mf -> strip_position mf.sourcename) m_fns in
        let run_once_sources = List.map (fun mf -> strip_position mf.sourcename)
                (List.filter (fun x -> x.runonce) m_fns) in
	let _ = List.iter (verify_source symboltable) sources in
	let _ = check_guard_refs symboltable in
        (*
	let ulist = UnionFind.union_find (unified @ more_unified) in
	let ensure_each n nd (ulistconcat,ulist) =
		let do_one (ulistconcat,ulist) x =
			if List.mem x ulistconcat then (ulistconcat,ulist)
			else (x::ulistconcat,[x]::ulist)
		in  List.fold_left do_one (ulistconcat,ulist) 
			(if nd.SymbolTable.nodeoutputs = None then
				[n,true]
			else [n,true;n,false])
		in
	let _,ulist = SymbolTable.fold_nodes ensure_each symboltable 
		(List.concat ulist,ulist)
        *)
        let more_unified = TypeCheck.from_basic_nodes symboltable sinks sources in
        let ulist = TypeCheck.concat unified more_unified
	in  { sources = sources
	    ; fmap = flowmap
	    ; ulist = ulist
            ; symtable = symboltable
            ; errhandlers = error_handlers
	    ; modules = List.map (fun md -> strip_position md.modulename) mod_defs
            ; terminates = terms
            ; runoncesources = run_once_sources
            ; consequences = TypeCheck.consequences ulist symboltable
            ; guard_order_pairs = List.map (fun (b,a) -> (strip_position b, strip_position a)) order_decls
	    }

let make_compatible br_const br_change =
        let conseq = TypeCheck.make_compatible br_change.symtable br_const.consequences br_change.consequences
        in  { sources = br_change.sources
            ; fmap = br_change.fmap
            ; ulist = br_change.ulist
            ; symtable = br_change.symtable
            ; errhandlers = br_change.errhandlers
            ; modules = br_change.modules
            ; terminates = br_change.terminates
            ; runoncesources = br_change.runoncesources
            ; consequences = conseq
            ; guard_order_pairs = br_change.guard_order_pairs
            }
