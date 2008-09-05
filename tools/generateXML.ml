(* take a flow, create XML *)


open Xml
open Flow

(**
 general structure is as follows:
  <flow name=...>
   <guard name=... magicnumber=.../>
   <node name=... source=[true|false] iserrhandler=[true|false] detached=[true|false]> <!-- name of the node -->
    <guardref name=... wtype=...>
      <argument argno=.../>
      <argument argno=.../>
    </guardref>
    <errorhandler name=.../>
    <successorlist> <!-- a list of concurrent branches -- all are taken -->
      <successor> <!-- a list of choices -->
       <case nodetarget=...> <!-- name of the node -->
		<!-- all must be satisfied to goto the node target -->
        <condition name=... argno=... isnegated=[true|false] unionnumber=.../>
        <condition name=... argno=... isnegated=[true|false] unionnumber=.../>
        <condition name=... argno=... isnegated=[true|false] unionnumber=.../>
       </case>
      </successor>
    </successorlist>
   </node>
  </flow>
*)

let xml_condition_str = "condition"
let xml_argument_str = "argument"
let xml_guard_str = "guard"
let xml_guardref_str = "guardref"
let xml_magicnumber_str = "magicnumber"
let xml_name_str = "name"
let xml_function_str = "function"
let xml_argno_str = "argno"
let xml_unionnumber_str = "unionnumber"
let xml_wtype_str = "wtype"
let xml_detached_str = "detached"
let xml_isnegated_str = "isnegated"
let xml_iserrhandler_str = "iserrhandler"
let xml_case_str = "case"
let xml_successor_str = "successor"
let xml_successorlist_str = "successorlist"
let xml_source_str = "source"
let xml_node_str = "node"
let xml_nodetarget_str = "nodetarget"
let xml_flow_str = "flow"
let xml_errorhandler_str = "errorhandler"


let condition el_name el_argno el_isnegated el_unionnumber = 
	Element (xml_condition_str
		, [ xml_name_str,el_name
		  ; xml_argno_str,el_argno
		  ; xml_isnegated_str,el_isnegated
		  ; xml_unionnumber_str,el_unionnumber
		  ]
		, [])

let guard el_name el_magicnumber =
	Element (xml_guard_str
		, [ xml_name_str, el_name
		  ; xml_magicnumber_str, el_magicnumber
		  ]
		, [])

let guardref el_name el_unionnumber el_wtype arguments =
	Element (xml_guardref_str
		, [ xml_name_str, el_name
		  ; xml_unionnumber_str, el_unionnumber
		  ; xml_wtype_str, el_wtype
		  ]
		, arguments)

let argument el_argno =
	Element (xml_argument_str
		, [xml_argno_str, el_argno]
		, [])

let errorhandler el_name = 
	Element (xml_errorhandler_str
		,[xml_name_str,el_name]
		,[])

let case el_nodetarget conditions =
	Element (xml_case_str
		,[xml_nodetarget_str,el_nodetarget]
		,conditions)

let successor cases =
	Element (xml_successor_str
		,[]
		,cases)

let successorlist successors =
	Element (xml_successorlist_str
		,[]
		,successors)

let node el_name el_function el_source el_iserrorhandler el_detached guardrefs errorhandler_opt successorlist =
	Element(xml_node_str
		,[ xml_name_str,el_name
		 ; xml_function_str,el_function
		 ; xml_source_str,el_source
		 ; xml_iserrhandler_str,el_iserrorhandler
		 ; xml_detached_str,el_detached
		 ]
		,match errorhandler_opt with
			None -> guardrefs @ [successorlist]
			| (Some errorhandler) -> 
				guardrefs @ [errorhandler;successorlist])

let flow el_name nodes =
	Element(xml_flow_str
		,[xml_name_str,el_name]
		,nodes)

exception XMLConversion of string * ParserTypes.position

let emit_xml programname br unionmap = 
	let stable = br.Flow.symtable in
	let fmap = br.Flow.fmap in
	let sourcelist = br.Flow.sources in
	let errhandlers = br.Flow.errhandlers in
	let is_detached n = SymbolTable.is_detached stable n in
	let is_abstract n = SymbolTable.is_abstract stable n in
	let is_source n = List.mem n sourcelist in
	let is_runonce_source n = List.mem n br.Flow.runoncesources in
        let is_terminate n = List.mem n br.Flow.terminates in
	let get_node n _ ll = if Flow.is_concrete stable fmap n then n::ll else ll in
	let get_guard n _ ll = (n::ll) in
	let guardlist = SymbolTable.fold_guards get_guard stable [] in 
	let nodelist = Flow.flowmap_fold get_node fmap [] in
        let uniq ll =
                let lls = List.sort compare ll in       
                let rec uniq' ll =
                        match ll with 
                                (h1::h2::tl) -> 
                                        if h1 = h2 then uniq' (h1::tl) 
                                        else h1::(uniq' (h2::tl))
                                | _ -> ll
                in  uniq' lls
                in
	let canon_case sol =
		List.map (fun so ->
				(match so with
					None -> []
					| (Some s) -> [s,false])) sol in
	let rec and_canon_condition c1 c2 =
		match c1,c2 with 
			([],_) -> c2
			| (_,[]) -> c1
			| (h1::t1,h2::t2) -> 
				(uniq (h1@h2))::(and_canon_condition t1 t2) in
        let rec negate_canon_condition c = (* returns list of conditions *)
                match c with
                        [] -> []
                        | ([]::tl) -> 
                                List.map (fun ll -> []::ll)
                                (negate_canon_condition tl)
                        | (((h,neg)::htl)::tl) -> 
                                ([h,not neg]::tl)
                                ::(negate_canon_condition (htl::tl))
                in
        let is_condition_always_false c =
                let rec one_false ll =
                        match ll with
                                ((h,neg)::t) -> 
                                        if List.mem (h,not neg) t then
                                                true
                                        else one_false t
                                | _ -> false
                in List.exists one_false c in
	let gen_eh ehfl =
		if Flow.is_null_node ehfl then None
		else let n = get_name ehfl
                     in Some(errorhandler n, n) in
	let rec gen_cond u_n i ccond =
		match ccond with
			(h::t) ->
				(List.map (fun (s,neg) -> 
                                        condition s (string_of_int i) 
                                                (if neg then "true" else "false")
                                                u_n) h)
				@ (gen_cond u_n (i+1) t)
			| _ -> [] in
	(*let rec prod f ll1 ll2 =
		match ll2 with
			(h::t) -> (List.map (fun x -> f x h) ll1)
                                @ (prod f ll1 t)
			| _ -> []
		in*)
        let rec prod f ll1 ll2 =
                match ll1,ll2 with
                        (h1::t1,h2::t2) -> (f h1 h2)::(prod f t1 t2)
                        | ([],_) -> ll2
                        | (_,[]) -> ll1
                in
	let find_union_number (y,io) = 
		let nd = SymbolTable.lookup_node_symbol stable y
		in  string_of_int (List.assoc (nd.SymbolTable.functionname,io) unionmap) in
	let rec gen_succ' ccond fl = 
		let sfun n _ _ _ = 
			let u_n = find_union_number (n,true) in
                        let ist = is_terminate n in
                        let _ = if (not ist) && (is_abstract n) then 
                                        raise (XMLConversion ("the node "^n^" is abstract, but it is not defined", ParserTypes.noposition))
                                else ()
			in  if ist || (is_condition_always_false ccond) then
                                []
                            else [[case n (gen_cond u_n 1 ccond)]] in
		let chefun _ solfl =
                        let condunit = List.map (fun x -> []) 
                                (let sol,_ = List.hd solfl in sol) in
                        let onfold (condneg,sofar) (sol,flr) =
                                let ccond' = canon_case sol in
                                let negccond' = 
                                        match negate_canon_condition ccond' with
                                                [] -> condunit
                                                | [x] -> x
                                                | _ ->
                                                        raise (XMLConversion ("not - implemented - talk to Mark",ParserTypes.noposition)) in
                                let ccondlocal =
                                        List.fold_left and_canon_condition 
                                                condunit
                                                [ ccond
                                                ; ccond'
                                                ; condneg ] in
                                let condneg = and_canon_condition condneg negccond'
                                in  condneg, ((gen_succ' ccondlocal flr)::sofar) in
			let _,part_res = List.fold_left onfold (condunit,[]) solfl in
                                (*List.map 
				(fun (sol,flr) ->
					let ccond' = canon_case sol
					in  gen_succ' (and_canon_condition ccond ccond') flr) solfl
                                in*)
			let tmp = List.fold_left (prod (fun a -> (fun b -> b @ a))) [[]] part_res 
                        in  tmp in
                let coefun _ fll = 
                        let tmp = List.concat (List.map (gen_succ' ccond) fll) 
                        in  tmp in
		let nfun _ = []
		in  Flow.flow_apply (sfun,chefun,coefun,sfun,nfun) fl
		in
	let gen_succ ccond fl =
		List.map successor (gen_succ' ccond fl) in
	let is_error_handler n = List.mem n errhandlers in
	let number_inputs (i,ll) df = (i+1,(ParserTypes.strip_position df.ParserTypes.name,i)::ll) in
	let one_arg get_argno sp =
		let argno = get_argno (ParserTypes.strip_position sp)
		in  argument (string_of_int argno) in 
	let emit_one n =
		let is_dt = is_detached n in
		let is_src = is_source n in
		let is_ro_src = is_runonce_source n in
		let nd = SymbolTable.lookup_node_symbol stable n in
		let f = nd.SymbolTable.functionname in
		let _,argmap = List.fold_left number_inputs (1,[]) nd.SymbolTable.nodeinputs in
		let get_argno n = 
			try List.assoc n argmap
			with Not_found -> raise (XMLConversion (n,ParserTypes.noposition)) in
		let do_gr gr = 
			let _,gr_pos,_ = gr.ParserTypes.guardname in
			guardref (ParserTypes.strip_position gr.ParserTypes.guardname)
			(string_of_int (List.assoc (nd.SymbolTable.functionname,true) unionmap))
			(match gr.ParserTypes.modifiers with
				(ParserTypes.Read::_) -> "1"
				| (ParserTypes.Write::_) -> "2"
				| _ -> "0")
			(try List.map (one_arg get_argno) gr.ParserTypes.arguments
			with (XMLConversion (n,_)) ->
				raise (XMLConversion ("guard reference argument "^n^" not found",gr_pos)))
			in
		let is_eh = is_error_handler n in 
		let fl = Flow.flowmap_find n fmap in
		let sfun _ _ sc _ = sc in
		let chefun _ _ = Flow.null_flow in
		let coefun _ _ = Flow.null_flow in
		let nfun _ = Flow.null_flow in
		let succ = Flow.flow_apply (sfun,chefun,coefun,sfun,nfun) fl
		in  node n f (if is_src then "true" else "false")
			(if is_eh then "true" else "false")
			(if is_dt then "true" else "false")
			(List.map do_gr nd.SymbolTable.nodeguardrefs)
			(let sfun _ _ _ eh = 
                                (match gen_eh eh with
                                        (Some (eh,ehn)) -> 
                                                if is_terminate ehn then None 
                                                else Some eh
                                        | _ -> None) in
			let chefun _ _ = None in
			let coefun _ _ = None in
			let nfun _ = None
			in  Flow.flow_apply (sfun, chefun, coefun, sfun, nfun) fl)
			(successorlist ((gen_succ [] succ)
				@ (if is_src && (not is_ro_src) then [successor [case n []]] else []))) in
	let guard_ff (ll,i) gname = 
		let element = guard gname (string_of_int i)
		in  (element::ll, i+1) in
        let rec order_node_list onfunl nl =
                match onfunl with
                        [] -> nl
                        | (onfunh::onfuntl) ->
                                let al,bl = List.partition onfunh nl
                                in  (order_node_list onfuntl al) @ bl
	in  let guard_elements, _ = List.fold_left guard_ff ([],1) guardlist in
	    let node_elements = List.map emit_one 
                        (let nodelist = order_node_list [is_source; is_runonce_source] nodelist
                        in  List.filter (fun n -> not (is_abstract n)) 
                                nodelist)
	    in  flow programname (guard_elements @ node_elements)

let write_xml xml filename =
	let xml_str = Xml.to_string_fmt xml in  
	let out = open_out filename in
	let _ = output_string out xml_str
	in  close_out out
	
