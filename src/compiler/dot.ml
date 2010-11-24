(* a DOT generator for flows *)

open Flow
open ParserTypes

type dot = string list

let lightgreen = "\"#90ff90\""
let lightpink = "\"#ff9090\""
let darkgrey = "\"#202020\""

let concat a b = b^a
let concat' a b = a^b

let to_string d = List.fold_right concat d ""

let string_replace (from_c,to_c) str =
	let s = String.copy str in  
	let rec s_r str =
		try let ind = String.index str from_c
		    in  s_r (String.set str ind to_c; str)
		with Not_found -> str 
	in  s_r s

let rec dedoubledot fp =
        let len = String.length fp 
        in  try let ind = String.rindex_from fp (len-1) '.'
                in  if (String.get fp (ind-1)) = '.'
                            && (String.get fp (ind-2)) = '/' then 
                        let ind2 = try String.rindex_from fp (ind-3) '/'
                                with Not_found -> 0
                        in  (String.sub fp 0 ind2)
                                ^(String.sub fp (ind+1) (len - (ind+1)))
                    else fp
            with Not_found -> fp

let mpos_to_url name_opt =
        match name_opt with 
                None -> None
                | (Some n) ->
                        let s = String.copy n in
                        let ind = String.rindex s '.' in
			let fn = String.sub s 0 ind in
			let fn = 
				try let slashind = (String.rindex fn '/')+1
				    in  String.sub s slashind 
					((String.length fn)-slashind)
				with Not_found -> fn
				in
                        let fp = (dedoubledot ((CmdLine.get_uribase_path())^fn))^".svg"
                        in  Some fp
        

let sanitize_name n = 
	let n = string_replace ('.','_') n in
	let nlen = String.length n in
	let n = if (nlen > 0) && (n.[0] = '&') then
			String.sub n 1 (nlen-1)
		else n
	in  n

let generate_flow flowmap =
	let rec nspcs n = 
		if n = 0 then ""
		else " "^(nspcs (n-1)) in
	let do_label s = 
		let n = (String.length s) / 6 in
		let nsp = nspcs n
		in  "label=\""^nsp^s^nsp^"\"" in
	let d = ref [] in
	let o_string och s = och := s::(!och) in
	let so_tostr so =
		match so with
			None -> "*"
			| (Some x) -> x in
	let comma_so_tostr f so = f^","^(so_tostr so) in
	let sol_tostr sol =
		(match sol with 
			(h::tl) -> ("["^(so_tostr h)
				^(List.fold_left comma_so_tostr "" tl)^"]")
			| _ -> "[]") in
	let sourcefn s _ fl efl =
		begin
		o_string d ((sanitize_name s)
                        ^" [style=bold, rank=1,"^(do_label s)^"];\n");
		if Flow.is_null_node fl then ()
		else let fln = Flow.get_name fl
		    in o_string d ((sanitize_name s)^" -> "
                        ^(sanitize_name fln)^";\n");
		if Flow.is_null_node efl then ()
		else let efln = Flow.get_name efl
		    in o_string d ((sanitize_name s)^" -> "
                        ^(sanitize_name efln)^" [style=dotted];\n");
		end
		in
	let do_solf s (sol,fl) =
		let fln = Flow.get_name fl
		in  o_string d 
			((sanitize_name s)^" -> "^(sanitize_name fln)
                        ^" [color=blue,label=\""
			^(sol_tostr sol)^"\"];\n") in
	let do_fll s fl =
		let fln = Flow.get_name fl
		in  o_string d 
			((sanitize_name s)^" -> "^(sanitize_name fln)
                        ^" [color=red];\n") in
	let chexprfn s solfl =
		begin
		o_string d ((sanitize_name s)^" [color=blue,"^(do_label s)^"];\n");
		List.iter (do_solf s) solfl
		end in
	let coexprfn s fll =
		begin
		o_string d ((sanitize_name s)^" [color=red,"^(do_label s)^"];\n");
		List.iter (do_fll s) fll
		end in
	let cnodefn s _ fl efl =
		begin
		o_string d ((sanitize_name s)^"["^(do_label s)^"];\n");
		if Flow.is_null_node fl then ()
		else let fln = Flow.get_name fl
		    in o_string d ((sanitize_name s)^" -> "
                                ^(sanitize_name fln)^";\n");
		if Flow.is_null_node efl then ()
		else let efln = Flow.get_name efl
		    in o_string d ((sanitize_name s)^" -> "
                        ^(sanitize_name efln)^" [style=dotted];\n");
		end
		in
	let nullfn () = () in
	let onItem _ fl () =
		Flow.flow_apply (sourcefn,chexprfn,coexprfn,cnodefn,nullfn) fl in
	let _ = o_string d "digraph G {\nnode [fontsize=14];\n" in
	let _ = Flow.flowmap_fold onItem flowmap () in
	let _ = o_string d "}\n"
	in  !d

let rec break_dotted_name nsn =
	try let ind = String.index nsn '.' in
	    let len = String.length nsn in
	    let hstr = String.sub nsn 0 ind in
	    let tstr = String.sub nsn (ind+1) (len-(ind+1))
	    in  hstr::(break_dotted_name tstr)
	with Not_found -> [nsn]

let generate_program pname p =
        let p = Flatten.remove_reductions p in
        let is_plugin pn = List.exists (fun p -> (strip_position p.pluginname) = pn) p.plugin_list in
        let pname = sanitize_name pname in
        let strip x = strip_position x in
        let lookup_module p m =
                List.find (fun md -> (strip md.modulename) = m) p.mod_def_list in
        let lookup_instance p i =
                List.find (fun mi -> (strip mi.modinstname) = i) p.mod_inst_list in
        let rec lookup_node p n =
                try List.find (fun nd -> (strip nd.nodename) = n) 
                        p.node_decl_list 
                with Not_found ->
                        (match break_dotted_name n with
                                (h1::h2::t) ->
                                        let mi = lookup_instance p h1 in
                                        let md = lookup_module p (strip mi.modsourcename) in
                                        let nameremains = List.fold_left concat' "" (h2::t)
                                        in  lookup_node md.programdef nameremains
                                | _ -> raise Not_found)
                in
	let d = ref [] in
	let o_string och s = och := s::(!och) in
        let nmap = ref [] in
        let lookup_mod_inst_nodes mi = 
                List.find_all (fun (a,_,_) -> a = mi) (!nmap) in
        let add_mod_inst_node mi n color_opt = nmap := (mi,n,color_opt)::(!nmap) in
        let remove_prefix mi fullname =
                let s = String.copy fullname in
                let lenpre = (String.length mi) in
                let len = String.length fullname
                in  String.sub s lenpre (len-lenpre)
                in
        let external_nodes = ref [] in
        let for_node context is_source name color style_opt =
                if (List.mem name (!external_nodes)) then ()
                else
                let pres_name = remove_prefix 
                        (if String.length context > 0 then context^"." else "")
                        name in
                let color = if color = "black" then
                                try let nd = lookup_node p pres_name
                                    in  if nd.outputs = None || nd.abstract then "blue" else color
                                with _ -> color
                            else color
                in  o_string d ((sanitize_name name)^"[label=\" "^pres_name
                        ^" \""^(if is_source then ",style=bold" else "")
                        ^",color="^color
                        ^(match style_opt with
                                None -> ""
                                | (Some c) -> (",style="^c))
                        ^"];\n") in
        let for_term_node n =
                o_string d ((sanitize_name n)
                   ^"[label=\" \",style=filled,color=black,shape=point];\n")
                in
        let for_invis_node n =
                o_string d ((sanitize_name n)
                   ^"[label=\" \",style=invis,shape=circle];\n")
                in
        let for_outputs iserrhandler nn =
                let paren_list (p1,p2) (t1,t2) ll =
                        match ll with
                                (a,b,c)::t -> 
                                        (p1^a,b,c^p2)::(List.map
                                                (fun (x,y,z) -> t1^x,y,t2^z)
                                                t)
                                |_ -> ll in
                let rparen_list (p1,p2) (t1,t2) ll = 
                        List.rev (paren_list (p1,p2) (t1,t2) (List.rev ll)) in
                let format_decl o = strip o.ctypemod,strip o.ctype,strip o.name in
                let format_out (a,b,c) =
                        "<tr>"
                        ^"<td align=\"left\">"^a^"</td>"
                        ^"<td align=\"left\">"^b^"</td>"
                        ^"<td align=\"left\">"^c^"</td>"
                        ^"</tr>" in
                let format_output_list ol = 
                        "<<table border=\"0\" align=\"left\"> ("
                        ^(List.fold_left concat' ""
                                (let ll = List.map format_decl ol in
                                 let ll = paren_list ("(","") (",","") ll in
                                 let ll = rparen_list ("",")") ("","") ll
                                 in List.map format_out ll))
                        ^") </table>>" in
                let is_pass_through ins outs =
                    match outs with    
                        (Some os) -> os = ins
                        | _ -> true in
                try let nd = lookup_node p nn 
                    in  if is_pass_through nd.inputs nd.outputs then ""
                        else (match if iserrhandler then Some nd.inputs else nd.outputs with
                                        (Some []) -> ""
                                        | (Some os) -> format_output_list os
                                        | _ -> raise Not_found) 
                with Not_found -> "" in
        let held_edge_strings = ref [] in
        let deplugin_name nsn =
                let reassemble y x =
                        if (String.length y) = 0 then x
                        else y^"."^x
                in  match break_dotted_name nsn with
                        (h1::h2::t) -> List.fold_left reassemble ""
                                (if is_plugin h1 then h2::t else h1::h2::t)
                        | [n] -> n 
                        | _ -> "~blech~" in
        let for_edge from_name to_name color cond_opt style_opt =
                let from_name = deplugin_name from_name in
                let edge_string =
                        ((sanitize_name from_name)^" -> "
                        ^(sanitize_name to_name)^"[color="^color
                        ^(match cond_opt with 
                                None -> 
                                 (if CmdLine.get_avoid_dot_ios () then ""
                                  else  let lab = for_outputs (style_opt = (Some "dotted")) from_name
                                        in  if lab = "" then "" else ",label="^lab)
                                |(Some "") -> ""
                                |(Some c) -> (",label=\"["^c^"]\""))
                        ^(match style_opt with
                                None -> ""
                                |(Some c) -> (",style="^c))
                        ^"];\n") 
                in  if (List.mem from_name (!external_nodes)) || (List.mem to_name (!external_nodes)) then
                        held_edge_strings := edge_string::(!held_edge_strings)
                    else o_string d edge_string in
        let on_mainfun mf =
                let n = strip mf.sourcename in
                let c = if mf.runoncetype = 1 then "lightgrey" 
			else if mf.runoncetype = 2 then "purple" (* door *)
			else "black"
                in  for_node "" true n c (Some "bold");
                    (match mf.successor with
                        None -> ()
                        | (Some s) -> 
                            for_edge n (strip s) "black" None None)
                in
        let on_err er =
                let h = strip er.handler in
                let on_each onn =
                        let n = strip onn
                        in  for_edge n h "black" None (Some "dotted")
                in  List.iter on_each er.onnodes
                in
        let on_mod_inst mi =
                let name = strip mi.modinstname in
                let ms = strip mi.modsourcename in
                let md = lookup_module p ms in
                let _,mpos,_ = md.modulename in
                let urlopt = mpos_to_url mpos.file in
                let _ = o_string d ("subgraph cluster"^name^" {\n"
                                ^"style=filled;\ncolor=lightgrey;\n"
                                ^"label=\""^ms^" "^name^"\";\n"
                                ^(match urlopt with 
                                        None -> ""
                                        | (Some url) -> ("URL=\""^url^"\";\n"))) in
                let mins = lookup_mod_inst_nodes name in
                let on_each nl (_,nn,copt) =
                          let c = match copt with
                                        None -> darkgrey
                                        | (Some c) -> c
                          in  for_node name false nn c None;
                                nn::nl
                        in
                let nsofar = List.fold_left on_each [] mins in
                let on_each_hook nl nd =
                        let nname = strip nd.nodename 
                        in
                        if (nd.outputs = None || nd.abstract) 
                                        && not (List.mem nname nl) then
                                (for_node name false (name^"_"^nname) "blue" None;
                                nname::nl)
                        else nl
                        in
                let not_an_expr nd = 
                        let nn = strip nd.nodename
                        in
                        try  let _ = List.find (fun e -> (strip e.exprname) = nn) md.programdef.expr_list
                             in false
                        with Not_found -> true
                        in
                let nsofar = List.fold_left on_each_hook nsofar 
                        (List.filter not_an_expr md.programdef.node_decl_list) in
                let _ = if nsofar = [] then for_invis_node (name^"_invis")
                in  o_string d "}\n"
                in
        let on_expr ex =
                let on_comma_item ci =
                        match ci with
                                Star -> "*,"
                                | (Ident sp) -> ((strip sp)^",") in
                let on_cil cil = List.fold_left concat "" 
                                (List.map on_comma_item (List.rev cil)) in
                let cb = on_cil ex.condbinding in
                let n = strip ex.exprname in
                let c = match ex.etype with
                                Choice -> "blue"
                                | Concurrent -> "red" in
                let emit_h h =
                        let h = strip h 
                        in  match break_dotted_name h with
                                (hmi::(_::_)) -> add_mod_inst_node hmi h None
                                | _ -> for_node "" false h "black" None in
                let rec seq_edge_succ ll =
                        match ll with
                            (h1::h2::t) ->
                                begin
                                for_edge (strip h1) (strip h2) c None None;
                                seq_edge_succ (h2::t);
                                emit_h h1
                                end
                            | [h] -> emit_h h
                            | _ -> ()
                        in
                let concur_edge_succ n ll =
                        let on_each h =
                                begin
                                for_edge n (strip h) c None None;
                                emit_h h
                                end
                        in  List.iter on_each ll
                in  
                    begin
                    (match break_dotted_name n with
                        (hmi::(_::_)) -> add_mod_inst_node hmi n (Some c)
                        | _ -> for_node "" false n c None);
                    (match ex.successors with
                        (h::t) ->
                            (match ex.etype with
                                Choice -> 
                                        (for_edge n (strip h) c (Some cb) None;
                                        seq_edge_succ)
                                | Concurrent -> concur_edge_succ n) ex.successors
                        | _ -> ())
                    end in
        let on_term t =
                let t = strip t in
                let tname = t^"_term" in
                let _ = for_edge t tname "black" None None ;
                        for_term_node tname
                in  match break_dotted_name t with
                        (h::(_::_)) -> add_mod_inst_node h t None
                        | _ -> ()
                in
        let on_plugin pl =
                let name = strip pl.pluginname in
                (*let _,mpos,_ = pl.pluginname in*)
                let p = pl.pluginprogramdef in
                let ext_nodes = List.map (fun nd -> strip nd.nodename)
                        (List.filter (fun nd -> nd.externalnode) 
                                p.node_decl_list) in
                let _ = external_nodes := ext_nodes @ (!external_nodes) in
                (*let urlopt = mpos_to_url mpos.file in*)
                let col = match CmdLine.get_plugin_name () with
                                None -> "pink"
                                | (Some pn) -> if pn = name then lightgreen else lightpink in
                let _ = o_string d ("subgraph cluster"^name^" {\n"
                                ^"style=filled;\ncolor="^col^";\n"
                                ^"label=\""^name^"\";\n"
                                (*^(match urlopt with 
                                        None -> ""
                                        | (Some url) -> ("URL=\""^url^"\";\n"))*)) in
                let _ = begin
                        List.iter on_mainfun p.mainfun_list;
                        List.iter on_err p.err_list;
                        List.iter on_expr p.expr_list;
                        List.iter on_term p.terminate_list
                        end
                in      begin
                        o_string d "}\n";
                        List.iter (o_string d) (!held_edge_strings);
                        held_edge_strings := []
                        end
                in
        let on_program p =
                begin
                List.iter on_mainfun p.mainfun_list;
                List.iter on_err p.err_list;
                List.iter on_expr p.expr_list;
                List.iter on_term p.terminate_list;
                List.iter on_mod_inst p.mod_inst_list;
                List.iter on_plugin p.plugin_list
                end in
        let _ = begin
                o_string d ("digraph "^pname^" {\n");
                o_string d "node [fontsize=14]\n";
                on_program p;
                o_string d "}\n"
                end
        in  !d
        

