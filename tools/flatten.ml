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
                        (Unify.Success) -> ()
                        | (Unify.Fail (i,reason)) ->
                                raise (FlattenFailure ("could not unify guard types (argument"^(string_of_int i)^" "^reason^")", pos)))
            ; if match_dt a1.outputtype a2.outputtype then ()
              else raise (FlattenFailure ("could not unify guard types (output type)", pos))

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
                } 
        in      { cond_decl_list = pr.cond_decl_list
                ; atom_decl_list = ad::pr.atom_decl_list
                ; node_decl_list = pr.node_decl_list
                ; mainfun_list = pr.mainfun_list
                ; expr_list = pr.expr_list
                ; err_list = pr.err_list
                ; mod_def_list = pr.mod_def_list
                ; mod_inst_list = pr.mod_inst_list
                ; terminate_list = pr.terminate_list
                }

let add_self_guardrefs pr = 
        let add_self_guardref_nonabst nd =
                let _,p1,p2 = nd.nodename in
                { detached = nd.detached
                ; abstract = nd.abstract
                ; nodename = nd.nodename
                ; nodefunction = nd.nodefunction
                ; inputs = nd.inputs
                ; guardrefs = nd.guardrefs 
                        @ (if nd.abstract then [] 
                           else [ { guardname = ("self",p1,p2)
                                  ; arguments = []
                                  ; modifiers = [ Read ]
                                  ; localgname = None
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
                ; terminate_list = pr.terminate_list
                }

let program_append pr1 pr2 =
        { cond_decl_list = pr1.cond_decl_list @ pr2.cond_decl_list
        ; atom_decl_list = pr1.atom_decl_list @ pr2.atom_decl_list
        ; node_decl_list = pr1.node_decl_list @ pr2.node_decl_list
        ; mainfun_list = pr1.mainfun_list @ pr2.mainfun_list
        ; expr_list = pr1.expr_list @ pr2.expr_list
        ; err_list = pr1.err_list @ pr2.err_list
        ; mod_def_list = pr1.mod_def_list @ pr2.mod_def_list
        ; mod_inst_list = pr1.mod_inst_list @ pr2.mod_inst_list
        ; terminate_list = pr1.terminate_list @ pr2.terminate_list
        }

let context_for_module pr mn =
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
                ; terminate_list = prm.terminate_list
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
                ; nodename = nd.nodename
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
            ; terminate_list = pr.terminate_list
            }


let get_new_subst ip ipn old_subst guardaliases =
        let onga (xp,yp) = 
                let x = strip_position xp in
                let y = strip_position yp
                in  ipn^x,y
        in  (List.map onga guardaliases) @ old_subst
                

let flatten prog =
	(** instantiate modules  etc *)
	let prefix_sp pre (s,p1,p2) = (pre^s,p1,p2) in
	let prefix pre s = pre^s in
	let for_cond_decl pre_mi pre_md cd =
		{ condname = prefix_sp pre_mi cd.condname
		; condfunction = prefix pre_md cd.condfunction
		; condinputs = cd.condinputs }
		in
	(*let for_data_type _ pre_md dt =
		{ dctypemod = dt.dctypemod
		; dctype = prefix_sp pre_md dt.dctype }
		in*)
	let for_atom_decl pre_mi pre_md ad =
		{ atomname = prefix_sp pre_mi ad.atomname
		; atominputs = ad.atominputs
		; outputtype = (*for_data_type pre_mi pre_md*) ad.outputtype
		; atomtype = ad.atomtype }
		in
	let for_guard_ref pre_mi pre_md gr =
		{ guardname = prefix_sp pre_mi gr.guardname
		; arguments = gr.arguments
		; modifiers = gr.modifiers
		; localgname = gr.localgname } 
		in
	let for_node_decl pre_mi pre_md nd =
		{ detached = nd.detached
		; abstract = nd.abstract
		; nodename = prefix_sp pre_mi nd.nodename
		; nodefunction = prefix pre_md nd.nodefunction
		; inputs = nd.inputs
		; guardrefs = List.map 
			(for_guard_ref pre_mi pre_md) nd.guardrefs
		; outputs = nd.outputs }
		in
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
	let for_program pre_mi pre_md pr =
		{ cond_decl_list = List.map (for_cond_decl pre_mi pre_md) pr.cond_decl_list
		; atom_decl_list = List.map (for_atom_decl pre_mi pre_md) pr.atom_decl_list
		; node_decl_list = List.map (for_node_decl pre_mi pre_md) pr.node_decl_list
		; mainfun_list = List.map (for_mainfun pre_mi pre_md) pr.mainfun_list
		; expr_list = List.map (for_expr pre_mi pre_md) pr.expr_list
		; err_list = List.map (for_err pre_mi pre_md) pr.err_list
		; mod_def_list = []
		; mod_inst_list = []
                ; terminate_list = List.map (prefix_sp pre_mi) pr.terminate_list
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
				(program_append pr_sofar moddefpr)
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
	let add_mods modl pr =
		{ cond_decl_list = pr.cond_decl_list
		; atom_decl_list = pr.atom_decl_list
		; node_decl_list = pr.node_decl_list
		; mainfun_list = pr.mainfun_list
		; expr_list = pr.expr_list
		; err_list = pr.err_list
		; mod_def_list = pr.mod_def_list @ (List.map (fun (_,x) -> x) modl)
		; mod_inst_list = pr.mod_inst_list
                ; terminate_list = pr.terminate_list
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

