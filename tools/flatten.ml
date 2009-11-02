open ParserTypes

exception FlattenFailure of string * position

let rec find_mod_def modl modnp =
	let modn,p,_ = modnp
	in  match modl with 
		((pre,md)::tl) ->
			if (strip_position md.modulename) = modn 
			then pre,md.programdef
			else find_mod_def tl modnp
		| _ -> raise (FlattenFailure ("cannot find module "^modn,p))

let unify_atoms pos a1 a2 =
        let match_dt dt1 dt2 =
                (strip_position dt1.dctypemod) = (strip_position dt2.dctypemod)
                &&
                (strip_position dt1.dctype) = (strip_position dt2.dctype)
        in  (match Unify.unify_type_in_out a1.atominputs a2.atominputs with
                        (Unify.Success Unify.Strong) -> ()
                        | (Unify.Success Unify.Weak) -> 
                                raise (FlattenFailure ("could not unify guard types (they unify only weakly)", pos))
                        | (Unify.Fail (i,reason)) ->
                                raise (FlattenFailure ("could not unify guard types (argument"^(string_of_int i)^" "^reason^")", pos)))
            ; if match_dt a1.outputtype a2.outputtype then ()
              else raise (FlattenFailure ("could not unify guard types (output type)", pos))

let unify_nodes pos n1 n2 =
        let check dl1 dl2 =
                match Unify.unify_type_in_out dl1 dl2 with
                        (Unify.Success Unify.Strong) -> ()
                        | (Unify.Success Unify.Weak) -> 
                                raise (FlattenFailure ("could not unify guard types (they unify only weakly)", pos))
                        | (Unify.Fail (i,reason)) ->
                                raise (FlattenFailure ("could not unify guard types (argument"^(string_of_int i)^" "^reason^")", pos)) in  
        let _ = check n1.inputs n2.inputs 
        in  match n1.outputs, n2.outputs with
                (Some o1,Some o2) -> check o1 o2
                | _ -> ()
        

let proper_name_space mp =
        if 0 = String.length mp then true
        else try (String.rindex mp ':') = ((String.length mp)-1)
             with Not_found -> false

let add_atom_decl ip mp pr = 
        let _ = if proper_name_space mp then ()
                else raise (FlattenFailure ("internal - "^mp^" bad namespace", noposition)) in
        let ad =
                { atomname = (ip^"self",noposition,noposition)
                ; atominputs = []
                ; outputtype = 
                        { dctypemod = ("",noposition,noposition)
                        ; dctype = (mp^"ModuleConfig *",noposition,noposition) }
                ; atomtype = "readwrite"
                ; externalatom=false
                ; atommodifiers=[]
                } 
        in      { cond_decl_list = pr.cond_decl_list
                ; atom_decl_list = ad::pr.atom_decl_list
                ; node_decl_list = pr.node_decl_list
                ; mainfun_list = pr.mainfun_list
                ; expr_list = pr.expr_list
                ; err_list = pr.err_list
                ; mod_def_list = pr.mod_def_list
                ; mod_inst_list = pr.mod_inst_list
                ; plugin_list = pr.plugin_list
                ; terminate_list = pr.terminate_list
                ; order_decl_list = pr.order_decl_list
                }

let add_self_guardrefs pr = 
        let add_self_guardref_nonabst nd =
                let _,p1,p2 = nd.nodename in
                { detached = nd.detached
                ; abstract = nd.abstract
                ; ismutable = false
                ; externalnode = nd.externalnode
                ; nodename = nd.nodename
                ; nodefunction = nd.nodefunction
                ; inputs = nd.inputs
                ; guardrefs = nd.guardrefs 
                        @ (if nd.abstract then [] 
                           else [ { guardname = ("self",p1,p2)
                                  ; arguments = []
                                  ; modifiers = [ if nd.ismutable 
                                                then Write else Read ]
                                  ; localgname = None
                                  ; guardcond = []
                                  } ]
                           )
                ; outputs = nd.outputs
                } 
        in      { cond_decl_list = pr.cond_decl_list
                ; atom_decl_list = pr.atom_decl_list
                ; node_decl_list = List.map add_self_guardref_nonabst pr.node_decl_list
                ; mainfun_list = pr.mainfun_list
                ; expr_list = pr.expr_list
                ; err_list = pr.err_list
                ; mod_def_list = pr.mod_def_list
                ; mod_inst_list = pr.mod_inst_list
                ; plugin_list = pr.plugin_list
                ; terminate_list = pr.terminate_list
                ; order_decl_list = pr.order_decl_list
                }

let program_append keepextatoms pr1 pr2 =
        let is_ext_node n = n.externalnode in
        let is_ext_atom n = n.externalatom in
        let is_ext_inst n = n.externalinst in
        let ext_nodes1,nodes1 = List.partition is_ext_node pr1.node_decl_list in
        let ext_nodes2,nodes2 = List.partition is_ext_node pr2.node_decl_list in
        let ext_atoms1,atoms1 = List.partition is_ext_atom pr1.atom_decl_list in
        let ext_atoms2,atoms2 = List.partition is_ext_atom pr2.atom_decl_list in
        let ext_inst1,inst1 = List.partition is_ext_inst pr1.mod_inst_list in
        let ext_inst2,inst2 = List.partition is_ext_inst pr2.mod_inst_list in
        let check_ext_node nodes extnode =
                let ename,pos,_ = extnode.nodename in
                try let _ = List.find (fun n-> (strip_position n.nodename) = ename) nodes 
                    in ()
                with Not_found -> 
			raise (FlattenFailure ("external node reference "
				^ename^" not found",pos)) in
        let check_ext_atom atoms extatom =
                let ename,pos,_ = extatom.atomname in
                try let _ = List.find (fun n-> (strip_position n.atomname) = ename) atoms
                in  ()
                with Not_found -> 
			raise (FlattenFailure ("external guard reference "
				^ename^" not found",pos)) in
        let check_ext_inst insts extinst =
                let ename,pos,_ = extinst.modinstname in
                try let _ = List.find (fun n-> (strip_position n.modinstname) = ename) insts
                in  ()
                with Not_found -> 
			raise (FlattenFailure ("external module instance "
				^ename^" not found",pos)) in
        let _ = List.iter (check_ext_inst inst1) ext_inst2 in
        let _ = List.iter (check_ext_inst inst2) ext_inst1 in
        let _ = List.iter (check_ext_node nodes1) ext_nodes2 in
        let _ = List.iter (check_ext_node nodes2) ext_nodes1 in
        let _ = List.iter (check_ext_atom atoms1) ext_atoms2 in
        let _ = List.iter (check_ext_atom atoms2) ext_atoms1 in
        let keep_ext extl h =
                try let hname = strip_position h.atomname in
                    let h = List.find (fun n -> (strip_position n.atomname) = hname) extl
                    in  h
                with Not_found -> h
        in
        { cond_decl_list = pr1.cond_decl_list @ pr2.cond_decl_list
        ; atom_decl_list = 
                        if keepextatoms then
                                (List.map (keep_ext ext_atoms2) atoms1) 
                                @ (List.map (keep_ext ext_atoms1) atoms2)
                        else atoms1 @ atoms2
        ; node_decl_list = nodes1 @ nodes2
        ; mainfun_list = pr1.mainfun_list @ pr2.mainfun_list
        ; expr_list = pr1.expr_list @ pr2.expr_list
        ; err_list = pr1.err_list @ pr2.err_list
        ; mod_def_list = pr1.mod_def_list @ pr2.mod_def_list
        ; mod_inst_list = inst1 @ inst2
        ; plugin_list = pr1.plugin_list @ pr2.plugin_list
        ; terminate_list = pr1.terminate_list @ pr2.terminate_list
        ; order_decl_list = pr1.order_decl_list @ pr2.order_decl_list
        }

let remove_reductions prog =
        let ampers,otherwise = List.partition (fun exp -> (strip_position exp.exprname).[0] = '&') prog.expr_list in
        let toexprname e = 
                let en,pos,_ = e.exprname in
                let len = String.length en
                in  String.sub en 1 (len-1), pos in
        let put_in ll ampexpr =
                let ename,pos = toexprname ampexpr in
                try let expr,remainder = 
                        match List.partition (fun e -> (strip_position e.exprname) = ename) ll with 
                                ([x],y) -> x,y
				| ([],y) -> { ParserTypes.exprname = (ename,pos,pos)
					    ; ParserTypes.condbinding = []
					    ; ParserTypes.successors = []
					    ; ParserTypes.etype = ParserTypes.Concurrent }
					, y
					
                                | _ -> raise Not_found in
                    let _ = match expr.etype,expr.successors with
                                (Concurrent, _) -> ()
                                | (_,[]) -> ()
                                | (_,[_]) -> ()
                                | _ -> raise Not_found in
                    let newexpr =
                        { exprname=expr.exprname
                        ; condbinding=expr.condbinding
                        ; successors=ampexpr.successors @ expr.successors
                        ; etype=Concurrent
                        }
                    in  newexpr::remainder
                with Not_found -> raise (FlattenFailure("reduction statement here has no original expression or it is not unique "
			^"\n\tor its not the right type of expression"
			^"\n\t (ensure the hook point node is not choice point and has only a single successor)",pos))
        in  { cond_decl_list=prog.cond_decl_list
            ; atom_decl_list=prog.atom_decl_list
            ; node_decl_list=prog.node_decl_list
            ; mainfun_list=prog.mainfun_list
            ; expr_list=List.fold_left put_in otherwise ampers
            ; err_list=prog.err_list
            ; mod_def_list=prog.mod_def_list
            ; mod_inst_list=prog.mod_inst_list
            ; plugin_list=prog.plugin_list
            ; terminate_list=prog.terminate_list
            ; order_decl_list=prog.order_decl_list
            }


let context_for_module pr mn =
        let pr = remove_reductions pr in
        let md = List.find (fun md -> (strip_position md.modulename) = mn) pr.mod_def_list in
        let prm = md.programdef
        in      { cond_decl_list = prm.cond_decl_list
                ; atom_decl_list = prm.atom_decl_list
                ; node_decl_list = prm.node_decl_list 
                ; mainfun_list = prm.mainfun_list
                ; expr_list = prm.expr_list
                ; err_list = prm.err_list
                ; mod_def_list = pr.mod_def_list @ prm.mod_def_list
                ; mod_inst_list = prm.mod_inst_list
                ; plugin_list = prm.plugin_list
                ; terminate_list = prm.terminate_list
                ; order_decl_list = prm.order_decl_list
                }

let subst_guardrefs subst grl =
        let single_sub gr (x,y) = 
                let gn,p1,p2 = gr.guardname in
                if gn = x then 
                        let newlgn = 
                                match gr.localgname with
                                        None -> Some gr.guardname
                                        | (Some _) -> gr.localgname
                        in  { guardname = (y,p1,p2)
                            ; arguments = gr.arguments
                            ; modifiers = gr.modifiers
                            ; localgname = newlgn
                            ; guardcond = gr.guardcond
                            }
                else gr in
        let list_subst ll gr =
               List.fold_left single_sub gr ll
        in  List.map (list_subst subst) grl

let apply_guardref_subst gsubst pr =
        let _ = let dpsubst (x,y) = Debug.dprint_string ("    "^x^" -> "^y^"\n")
                in  List.iter dpsubst gsubst
                in
        let remove_subst_decl ll ad =
                let atomname = strip_position ad.atomname
                in  try  let _ = List.assoc atomname gsubst
                         in  ll
                    with Not_found -> ad::ll
                in
        let apply_nd nd =
                let _ = 
                        let dpgr gr = 
                                Debug.dprint_string ("      g:"^(strip_position gr.guardname)^"\n")
                        in  Debug.dprint_string ("   n:"^(strip_position nd.nodename)^"\n"); List.iter dpgr nd.guardrefs
                in
                { detached = nd.detached
                ; abstract = nd.abstract
                ; ismutable = nd.ismutable
                ; nodename = nd.nodename
                ; externalnode = nd.externalnode
                ; nodefunction = nd.nodefunction
                ; inputs = nd.inputs
                ; guardrefs = subst_guardrefs gsubst nd.guardrefs
                ; outputs = nd.outputs
                }
        in  { cond_decl_list = pr.cond_decl_list
            ; atom_decl_list = List.fold_left remove_subst_decl [] pr.atom_decl_list
            ; node_decl_list = List.map apply_nd pr.node_decl_list
            ; mainfun_list = pr.mainfun_list
            ; expr_list = pr.expr_list
            ; err_list = pr.err_list
            ; mod_def_list = pr.mod_def_list
            ; mod_inst_list = pr.mod_inst_list
            ; plugin_list = pr.plugin_list
            ; terminate_list = pr.terminate_list
            ; order_decl_list = pr.order_decl_list
            }


let get_new_subst ip ipn old_subst guardaliases =
        let onga (xp,yp) = 
                let x = strip_position xp in
                let y = strip_position yp
                in  ipn^x,y
        in  (List.map onga guardaliases) @ old_subst
                
let prefix_sp pre (s,p1,p2) = 
	let slen = String.length s
	in
	(       (if slen > 0 && s.[0] = '&' then
			("&"^pre^(String.sub s 1 (slen-1)))
		else pre^s)
	,p1,p2)

let prefix pre s = pre^s

let flatten prog =
	(** instantiate modules  etc *)
        let prog = remove_reductions prog in
	let for_cond_decl pre_mi pre_md cd =
		{ externalcond = cd.externalcond
                ; condname = prefix_sp pre_mi cd.condname
		; condfunction = prefix pre_md cd.condfunction
		; condinputs = cd.condinputs 
                } in
	(*let for_data_type _ pre_md dt =
		{ dctypemod = dt.dctypemod
		; dctype = prefix_sp pre_md dt.dctype }
		in*)
	let for_order_decl pre_mi _ (od1,od2) = 
                ( prefix_sp pre_mi od1, prefix_sp pre_mi od2) in
	let for_atom_decl pre_mi pre_md ad =
		{ atomname = prefix_sp pre_mi ad.atomname
		; atominputs = ad.atominputs
		; outputtype = (*for_data_type pre_mi pre_md*) ad.outputtype
		; atomtype = ad.atomtype 
                ; externalatom = ad.externalatom
                ; atommodifiers = ad.atommodifiers
                } in
	let for_guard_ref pre_mi pre_md gr =
		{ guardname = prefix_sp pre_mi gr.guardname
		; arguments = gr.arguments
		; modifiers = gr.modifiers
		; localgname = gr.localgname 
                ; guardcond = gr.guardcond
                } in
	let for_node_decl pre_mi pre_md nd =
		{ detached = nd.detached
		; abstract = nd.abstract
                ; ismutable = nd.ismutable
                ; externalnode = nd.externalnode
		; nodename = prefix_sp pre_mi nd.nodename
		; nodefunction = prefix pre_md nd.nodefunction
		; inputs = nd.inputs
		; guardrefs = List.map 
			(for_guard_ref pre_mi pre_md) nd.guardrefs
		; outputs = nd.outputs 
                } in
	let for_mainfun pre_mi pre_md mf =
		{ sourcename = prefix_sp pre_mi mf.sourcename
		; sourcefunction = prefix pre_md mf.sourcefunction
		; successor = 
			(match mf.successor with
				None -> None
				| (Some sp) -> 
					Some (prefix_sp pre_mi sp)) 
                ; runonce = mf.runonce
                } 
		in
	let for_comma_item pre_mi pre_md ci =
		match ci with
			Star -> Star
			| (Ident sp) -> Ident (prefix_sp pre_md sp)
		in
	let for_expr pre_mi pre_md exp =
		{ exprname = prefix_sp pre_mi exp.exprname
		; condbinding = List.map (for_comma_item pre_mi pre_md) exp.condbinding
		; successors = List.map (prefix_sp pre_mi) exp.successors 
                ; etype = exp.etype
                }
		in
	let for_err pre_mi pre_md err =
		{ onnodes = List.map (prefix_sp pre_mi) err.onnodes
		; handler = prefix_sp pre_mi err.handler } 
		in
	let get_implicit_gr_orderings ndl =
		let get_one nd =
			let localgname_map gr =
				match gr.localgname with
					(Some n) -> 
						( strip_position n
						, gr.guardname)
					| _ ->  ( strip_position gr.guardname
						, gr.guardname) in
			let lgn_map = List.map localgname_map nd.guardrefs in
			let filt_garg ue =
				match ue with
					(GArg ga) -> true
					| _ -> false in
			let on_garg gname ue =
				match ue with
					(GArg ga) -> 
						( List.assoc ga lgn_map
						, gname)
					| _ -> raise Not_found in
			let on_gr gr =
				let orderl = List.map (on_garg gr.guardname)
					(List.filter filt_garg
					(List.concat gr.arguments))
				in  orderl
			in  List.concat (List.map on_gr nd.guardrefs)
		in  List.concat (List.map get_one ndl) in
	let for_program pre_mi pre_md pr =
		{ cond_decl_list = List.map (for_cond_decl pre_mi pre_md) pr.cond_decl_list
		; atom_decl_list = List.map (for_atom_decl pre_mi pre_md) pr.atom_decl_list
		; node_decl_list = List.map (for_node_decl pre_mi pre_md) pr.node_decl_list
		; mainfun_list = List.map (for_mainfun pre_mi pre_md) pr.mainfun_list
		; expr_list = List.map (for_expr pre_mi pre_md) pr.expr_list
		; err_list = List.map (for_err pre_mi pre_md) pr.err_list
		; mod_def_list = []
		; mod_inst_list = []
                ; plugin_list = []
                ; terminate_list = List.map (prefix_sp pre_mi) pr.terminate_list
                ; order_decl_list = 
			let explicit_odl = pr.order_decl_list in
			let implicit_odl = get_implicit_gr_orderings pr.node_decl_list
			in  List.map (for_order_decl pre_mi pre_md) (explicit_odl @ implicit_odl)
			
		}
		in
	let add_mod_prefix modprefix md = (modprefix, md) in
        let check_atom_assignment local_ads external_ads guardaliases =
                let rec find np ads =
                        match ads with 
                                (h::t) ->
                                        if (strip_position h.atomname) =
                                              (strip_position np) then h
                                        else find np t
                                | _ -> 
                                        let gn,gpos,_ = np
                                        in  raise (FlattenFailure ("can't find guard reference "^gn,gpos))
                        in
                let check_one (x,y) =
                        let _,pos,_ = x in
                        let ad_local = find x local_ads in
                        let ad_external = find y external_ads 
                        in  unify_atoms pos ad_local ad_external
                in  List.iter check_one guardaliases
                in
	let rec flt instprefix modprefix modl gsubst pr =
		let oninst modl instprefix modprefix pr_sofar mi = 
			let srcname,srcpos,_ = mi.modsourcename in
                        let _ = Debug.dprint_string ("flatten->oninst "^srcname^" "
                                ^(strip_position mi.modinstname)^"\n") in
                        let _ = 
                                let dpp atom = 
                                        Debug.dprint_string ("  has atom "^(strip_position atom.atomname)^"\n")
                                in List.iter dpp pr_sofar.atom_decl_list
                                in
			let mp,moddefpr = find_mod_def modl mi.modsourcename in
                        let localize (glocal,(gglobal,gp1,gp2)) =
                                (glocal,(instprefix^gglobal,gp1,gp2)) in
                        let guardaliases = List.map localize mi.guardaliases in
                        let _ = check_atom_assignment moddefpr.atom_decl_list pr_sofar.atom_decl_list guardaliases in
			let moddefpr = add_self_guardrefs moddefpr in
			let ip = instprefix^(strip_position mi.modinstname)^"." in
                        let gsubst = get_new_subst instprefix ip gsubst guardaliases in
			let mp = srcname^"::"^mp in
                        let mp = 
                                if proper_name_space mp then mp
                                else mp^"::"
                                in
                        let moddefpr = flt ip mp modl gsubst moddefpr in
                        let moddefpr = apply_guardref_subst gsubst moddefpr
			in  add_atom_decl ip mp
				(program_append true pr_sofar moddefpr)
			in
		let modl = (List.map (add_mod_prefix modprefix) pr.mod_def_list) @ modl
		in  List.fold_left (oninst modl instprefix modprefix) 
			(for_program instprefix modprefix pr) pr.mod_inst_list
	in  flt "" "" [] [] prog


let rec break_namespaced_name nsn =
	try let ind = String.index nsn ':' in
	    let len = String.length nsn in
	    let hstr = String.sub nsn 0 ind in
	    let tstr = String.sub nsn (ind+2) (len-(ind+2))
	    in  hstr::(break_namespaced_name tstr)
	with Not_found -> [nsn]

let flatten_module module_name pr =
        let pr = remove_reductions pr in
	let add_mods modl pr =
		{ cond_decl_list = pr.cond_decl_list
		; atom_decl_list = pr.atom_decl_list
		; node_decl_list = pr.node_decl_list
		; mainfun_list = pr.mainfun_list
		; expr_list = pr.expr_list
		; err_list = pr.err_list
		; mod_def_list = pr.mod_def_list @ (List.map (fun (_,x) -> x) modl)
		; mod_inst_list = pr.mod_inst_list
                ; plugin_list = pr.plugin_list
                ; terminate_list = pr.terminate_list
                ; order_decl_list = pr.order_decl_list
		} in
	let nsnbl = break_namespaced_name module_name in
	let findfun n md = (strip_position md.modulename) = n in
	let rec flt ip mp modl pr nbl =
		match nbl with
			[] ->   let pr = add_atom_decl "" (module_name^"::") pr in
                                let pr = add_self_guardrefs pr 
                                in  add_mods modl pr
			| (h::t) ->
				let mpos,mneg = List.partition (findfun h) pr.mod_def_list in
                                let mneg = List.map (fun m -> (strip_position m.modulename,m)) mneg
				in  (match mpos with
					[amod] ->
						flt ip mp (modl @ mneg) amod.programdef t
					| _ -> raise (FlattenFailure ("cannot find module "^h,noposition)))
                in
	let pr = flt "" "" [] pr (List.rev nsnbl)
        in  flatten pr

let flatten_plugin' plugin_name prog = 
        (** returns before program, after program *)
        (*let prog = remove_reductions prog in*)
        let pre = plugin_name^"." in
        let pref = plugin_name^"::" in
        let for_ref is_en node_str_pos =
                if is_en (strip_position node_str_pos) then node_str_pos
                else prefix_sp pre node_str_pos in
        let for_cond_ref is_ec cond_str_pos =
                if is_ec (strip_position cond_str_pos) then cond_str_pos
                else prefix_sp pref cond_str_pos in
        let for_cond_decl cd =
                let isext = cd.externalcond
                in
                { externalcond = cd.externalcond
                ; condname = for_ref (fun _ -> isext) cd.condname
                ; condfunction = (if isext then "" else pref)^cd.condfunction
                ; condinputs = cd.condinputs
                } in
        let for_order_decl is_ea (od1,od2) =
                ( for_ref is_ea od1
                , for_ref is_ea od2 ) in
        let for_atom_decl ad =
                let isext = ad.externalatom 
                in
                { atomname = if isext then ad.atomname else prefix_sp pre ad.atomname
                ; atominputs = ad.atominputs
                ; outputtype = ad.outputtype
                ; atomtype = ad.atomtype
                ; externalatom = ad.externalatom
                ; atommodifiers = ad.atommodifiers
                } in
        let for_node_decl is_ea nd =
                let for_guardref gr =
                        { guardname = for_ref is_ea gr.guardname
                        ; arguments = gr.arguments
                        ; modifiers = gr.modifiers
                        ; localgname = gr.localgname
                        ; guardcond = gr.guardcond
                        } in
                let isext = nd.externalnode
                in
                { detached = nd.detached
                ; abstract = nd.abstract
                ; ismutable = nd.ismutable
                ; externalnode = nd.externalnode
                ; nodename = if isext then nd.nodename else prefix_sp pre nd.nodename
                ; nodefunction = (if isext then "" else pref)^nd.nodefunction
                ; inputs = nd.inputs
                ; guardrefs = List.map for_guardref nd.guardrefs
                ; outputs = nd.outputs
                } in
        let for_mainfun is_en mainfn =
                let isext = is_en (strip_position mainfn.sourcename) in
                let onsucc spopt =
                        match spopt with
                                None -> None 
                                | (Some sp) -> Some (for_ref is_en sp)
                in
                { sourcename = for_ref is_en mainfn.sourcename
		; sourcefunction = (if isext then "" else pref)^mainfn.sourcefunction
		; successor = onsucc mainfn.successor
                ; runonce = mainfn.runonce
                } in
        let for_expr is_en is_ec expr =
                let for_comma_item ci =
                        match ci with
                                Star -> Star
                                | (Ident sp) -> 
                                        Ident (for_cond_ref is_ec sp)
                in
                { exprname = for_ref is_en expr.exprname
                ; condbinding = List.map for_comma_item expr.condbinding
                ; successors = List.map (for_ref is_en ) expr.successors
                ; etype = expr.etype
                } in
        let for_err is_en err =
                { onnodes = List.map (for_ref is_en ) err.onnodes
                ; handler = for_ref is_en err.handler
                } in
        let for_mod_inst is_en is_ea minst =
		let for_guard_alias is_ea (a,b) =
			(a, for_ref is_ea b)
		in
                { modsourcename = minst.modsourcename
                ; modinstname = for_ref is_en minst.modinstname
                ; guardaliases = List.map (for_guard_alias is_ea) minst.guardaliases
                ; externalinst = minst.externalinst
                } in
        let for_terminate = for_ref in
	let remove_amper_name n =
		let nlen = String.length n
		in
		if nlen > 0 && n.[0] = '&' then
			String.sub n 1 (nlen-1)
		else n
		in
	let for_program pr =
                let ext_nodes = List.map (fun n -> strip_position n.nodename) (List.filter (fun n -> n.externalnode) pr.node_decl_list) in
                let ext_atoms = List.map (fun a -> strip_position a.atomname) (List.filter (fun a -> a.externalatom) pr.atom_decl_list) in
                let ext_conds = List.map (fun c -> strip_position c.condname) (List.filter (fun c -> c.externalcond) pr.cond_decl_list) in
                let ext_insts = List.map (fun c -> strip_position c.modinstname) (List.filter (fun c -> c.externalinst) pr.mod_inst_list) in
                let is_ext_node n = List.mem (remove_amper_name n) ext_nodes in
                let is_ext_inst i = List.mem i ext_insts in
                let is_ext_atom a = List.mem a ext_atoms in
                let is_ext_cond c = List.mem c ext_conds
                in
		{ cond_decl_list = List.map for_cond_decl pr.cond_decl_list
		; atom_decl_list = List.map for_atom_decl pr.atom_decl_list
		; node_decl_list = List.map (for_node_decl is_ext_atom) pr.node_decl_list
		; mainfun_list = List.map (for_mainfun is_ext_node) pr.mainfun_list
		; expr_list = List.map (for_expr is_ext_node is_ext_cond) pr.expr_list
		; err_list = List.map (for_err is_ext_node) pr.err_list
		; mod_def_list = []
		; mod_inst_list = List.map (for_mod_inst is_ext_inst is_ext_atom) pr.mod_inst_list
                ; plugin_list = []
                ; terminate_list = List.map (for_terminate is_ext_node) pr.terminate_list
                ; order_decl_list = List.map (for_order_decl is_ext_atom) pr.order_decl_list
		} in
        (*let local_prog = remove_reductions prog in*)
        let local_prog = for_program prog
        in  local_prog


let flatten_plugin plugin_name prog = 
        let append_but n prog pd =
                if (strip_position pd.pluginname) = n then prog
                else let pn = strip_position pd.pluginname
                     in  program_append 
                        ((n=plugin_name) && (pn = n))
                        (flatten_plugin' pn pd.pluginprogramdef) prog in
        let append_all_but n prog pdl =
                List.fold_left (append_but n) prog pdl in
        let flt_with = flatten (append_all_but "" prog prog.plugin_list) in
        let _ = List.iter Debug.dprint_string 
                (List.map (fun x -> " "^(strip_position x.pluginname)^"\n") prog.plugin_list) in
        let flt_without = flatten (append_all_but plugin_name prog prog.plugin_list)
        in  flt_without, flt_with
        
        

