(* take a flow, create XML *)


open Xml
open Flow

(**
 general program structure is as follows:
  <flow name=... ofluxversion=...>
   <guard name=... magicnumber=.../>
   <node name=... source=[true|false] iserrhandler=[true|false] detached=[true|false] inputunionnumber=... outputunionnumber=...> <!-- name of the node -->
    <guardref name=... wtype=... hash=.../>
    <errorhandler name=.../>
    <successorlist> <!-- a list of concurrent branches -- all are taken -->
      <successor name=...> <!-- a list of choices -->
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

  general plugin structure is as follows:
  <plugin ofluxversion=...>
   <library name="something.so">
    <depend name="somethingelse">
    <!-- can have a bunch of these -->
   </library>
   <guard .../>
   <node name=... external=[true|false] ...>  
     <!-- if external ="false" then this is a completely new node -- no merge ---->
     <!-- ... as before -->
   </node>
   <!-- for example: -->
   <node name=... external="true" ...>
     <!-- when external="true", then we are editting an existing node in the flow -->
     <successorlist>
      <successor name=...> 
        <add> <!-- new cases are prepended to the list for this successor -->
          <case ...> <!-- ... --> </case>
          <case ...> <!-- ... --> </case>
        </add>
      </successor>
     </successorlist>
   </node>
   <node name=... external="true" ...>
     <!-- when external="true", then we are editting an existing node in the flow -->
     <successorlist>
       <add>
         <successor name=...> <!-- new successor is prepended to the successor list -- order not important anyway -->
           <!-- ... -->
         </successor>
       </add>
     </successorlist>
   </node>
  </plugin>
  
  Note: you could load
 
   program.xml
   plugin1.xml
   plugin2.xml (which plugs into the (program+plugin1 flow) -- so order is critical)
*)

let xml_ofluxversion_str = "ofluxversion"
let xml_condition_str = "condition"
let xml_argument_str = "argument"
let xml_guard_str = "guard"
let xml_guardprecedence_str = "guardprecedence"
let xml_guardref_str = "guardref"
let xml_magicnumber_str = "magicnumber"
let xml_name_str = "name"
let xml_function_str = "function"
let xml_argno_str = "argno"
let xml_unionnumber_str = "unionnumber"
let xml_inputunionnumber_str = "inputunionnumber"
let xml_outputunionnumber_str = "outputunionnumber"
let xml_wtype_str = "wtype"
let xml_detached_str = "detached"
let xml_external_str = "external"
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
let xml_plugin_str = "plugin"
let xml_library_str = "library"
let xml_add_str = "add"
let xml_depend_str = "depend"
let xml_hash_str = "hash"
let xml_before_str = "before"
let xml_after_str = "after"

let depend el_name =
        Element (xml_depend_str
                , [ xml_name_str, el_name
                  ]
                , [])

let condition el_name el_argno el_isnegated el_unionnumber = 
	Element (xml_condition_str
		, [ xml_name_str,el_name
		  ; xml_argno_str,el_argno
		  ; xml_isnegated_str,el_isnegated
		  ; xml_unionnumber_str,el_unionnumber
		  ]
		, [])

let guard el_name =
	Element (xml_guard_str
		, [ xml_name_str, el_name
		  (*; xml_magicnumber_str, el_magicnumber*)
		  ]
		, [])

let guardprecedence el_before el_after =
        Element (xml_guardprecedence_str
                , [ xml_before_str, el_before
                  ; xml_after_str, el_after
                  ]
                , [])

let guardref el_name el_unionnumber el_hash el_wtype =
	Element (xml_guardref_str
		, [ xml_name_str, el_name
		  ; xml_unionnumber_str, el_unionnumber
                  ; xml_hash_str, el_hash
		  ; xml_wtype_str, el_wtype
		  ]
		, [])

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

let successor el_name cases =
	Element (xml_successor_str
		,[xml_name_str,el_name]
		,cases)

let successorlist successors =
	Element (xml_successorlist_str
		,[]
		,successors)

let node el_name el_function el_source el_iserrorhandler el_detached el_external el_inputunionnumber el_outputunionnumber guardrefs errorhandler_opt successorlist =
	Element(xml_node_str
		,[ xml_name_str,el_name
		 ; xml_function_str,el_function
		 ; xml_source_str,el_source
		 ; xml_iserrhandler_str,el_iserrorhandler
		 ; xml_detached_str,el_detached
		 ; xml_external_str,el_external
		 ; xml_inputunionnumber_str,el_inputunionnumber
		 ; xml_outputunionnumber_str,el_outputunionnumber
		 ]
		,match errorhandler_opt with
			None -> guardrefs @ [successorlist]
			| (Some errorhandler) -> 
				guardrefs @ [errorhandler;successorlist])

let el_ofluxversion = Vers.vers

let flow el_name nodes =
	Element(xml_flow_str
		,[ xml_name_str,el_name
                 ; xml_ofluxversion_str,el_ofluxversion
                 ]
		,nodes)

let library el_name depends =
        Element(xml_library_str
                ,[ xml_name_str,el_name ]
                ,depends)

let plugin library nodes_and_guards =
        Element(xml_plugin_str
                ,[ xml_ofluxversion_str,el_ofluxversion ]
                ,library::nodes_and_guards)

let add somethings = Element(xml_add_str,[],somethings)

exception XMLConversion of string * ParserTypes.position

let emit_program_xml programname br = 
        let conseq_res = br.Flow.consequences in
        let unionmap_find strio =
                let u_n = TypeCheck.get_union_from_strio conseq_res strio
                in  u_n in
	let stable = br.Flow.symtable in
	let fmap = br.Flow.fmap in
	let sourcelist = br.Flow.sources in
	let errhandlers = br.Flow.errhandlers in
	let is_detached n = SymbolTable.is_detached stable n in
	let is_external n = SymbolTable.is_external stable n in
	let is_abstract n = SymbolTable.is_abstract stable n in
	let is_source n = List.mem n sourcelist in
	let is_runonce_source n = List.mem n br.Flow.runoncesources in
        let is_terminate n = List.mem n br.Flow.terminates in
	let get_node n _ ll = if Flow.is_concrete stable fmap n then n::ll else ll in
	let get_guard n _ ll = (n::ll) in
	let guardlist = SymbolTable.fold_guards get_guard stable [] in 
        let guardlist = List.rev guardlist in
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
        let convert_argn t_u_n u_n j =
                if t_u_n = u_n then j
                else 
                try 
                        let gdecll = TypeCheck.get_decl_list_from_union conseq_res stable in
                        let t_is = gdecll t_u_n in
                        let is = gdecll u_n in
                        let rec pick j ll = 
                                match ll with
                                        (h::t) ->
                                                if j <= 1 then h
                                                else pick (j-1) t 
                                        | _ -> raise Not_found in
                        let df = (pick j t_is) in
                        let name = ParserTypes.strip_position df.ParserTypes.name in
                        let rec find i dfl =
                                match dfl with
                                        (h::t) -> 
                                                if (ParserTypes.strip_position h.ParserTypes.name)=name then i
                                                else find (i+1) t
                                        | _ -> raise (XMLConversion ("can't find arg conversion "^(string_of_int t_u_n)^" -> "^(string_of_int u_n),ParserTypes.noposition))
                        in  find 1 is 
                with Not_found -> j
                in
	let rec gen_cond t_u_n u_n i ccond =
		match ccond with
			(h::t) ->
				(List.map (fun (s,neg) -> 
                                        condition s 
                                                (let i = convert_argn t_u_n u_n i
                                                in  string_of_int i) 
                                                (if neg then "true" else "false")
                                                (string_of_int u_n)) h)
				@ (gen_cond t_u_n u_n (i+1) t)
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
		in  unionmap_find (nd.SymbolTable.functionname,io) in
	let rec gen_succ' n_out_u_n ccond fl = 
		(***********************************
		 * returns a ((case list) list)
		 * intention is  that each (case list) becomes a successor
		 * each successor is evaluated separately
		 * the first case to have all true conditions is used
		 * ccond is a (string * bool) list list where 
		 *     - each item (string * bool) is a condition_function & its "is negated" property
		 *     - a (string * bool) list is a conjunction of these on the same argument
		 *     - the full list is ordered and conjunctive so that the ith (string * bool) list
		 *       applies to the ith argument
		 ***********************************)
		let sfun n _ _ _ = 
			let u_n = find_union_number (n,true) in
                        let ist = (if CmdLine.get_abstract_termination() then is_abstract n else false ) || (is_terminate n) in
                        let _ = if (not ist) && (is_abstract n) then 
                                        raise (XMLConversion ("the node "^n^" is abstract, but it is not defined", ParserTypes.noposition))
                                else ()
			in  if ist || (is_condition_always_false ccond) then
                                []
                            else [[case n (gen_cond u_n n_out_u_n 1 ccond)]] in
			(***************************
			 * return a successor list with one successor in it on the given condition
		         * going to the given target
			 ***************************)
		let chefun choice_name solfl =
			(***************************
			 * solfl is a (string option list * flow ref ref) list
			 * each (string option list) denotes the parsed "[whatever1,whatever2,...]" line from the source
			 * the accompanying flow is the direction that takes
                         * Since choice has several of these, there is a list of them to process (order is important!)
			 ***************************)
                        let condunit = List.map (fun x -> []) 
                                (let sol,_ = List.hd solfl in sol) in
				(***************
				 * condunit is the nominal "true" (acts as "and" unit)
				 ***************)
                        let onfold (condneg,sofar) (sol,flr) =
                                let ccond' = canon_case sol in
				(***************
				 * ccond' : is the simple condition for this line
				 ***************)
                                let negccond' = 
                                        match negate_canon_condition ccond' with
                                                [] -> condunit
                                                | [x] -> x
                                                | _ ->
                                                        raise (XMLConversion ("not - implemented - talk to Mark "^choice_name,ParserTypes.noposition)) in
				(***************
				 * negccond' : try to obtain a condition for the negation of previous lines
				 ***************)
                                let ccondlocal =
                                        List.fold_left and_canon_condition 
                                                condunit
                                                [ ccond (* upper level condition *)
                                                ; ccond' (* local condition *)
                                                ; condneg (* accumulated negative condition, upto this case *)
						] in
                                let condneg = and_canon_condition condneg negccond'
                                in  condneg, ((gen_succ' n_out_u_n ccondlocal flr)::sofar) in
			let _,part_res = List.fold_left onfold (condunit,[]) solfl in
			let tmp = List.fold_left (prod (fun a -> (fun b -> b @ a))) [[]] part_res 
                        in  tmp in
                let coefun _ fll = 
                        let tmp = List.concat (List.map (gen_succ' n_out_u_n ccond) fll) 
			(****************************
			 * so basically concatenate the successor lists
			 ****************************)
                        in  tmp in
		let nfun _ = []
		in  Flow.flow_apply (sfun,chefun,coefun,sfun,nfun) fl
		in
	let gen_succ n_out_u_n ccond fl =
                let foldfun (resl,i) s =
                        (if s = [] then resl else ((successor (string_of_int i) s)::resl))
                        , i+1 in
                let resl,_ = List.fold_left foldfun ([],0) 
                        (List.rev (gen_succ' n_out_u_n ccond fl))
                in resl in
	let is_error_handler n = List.mem n errhandlers in
	(*let number_inputs (i,ll) df = (i+1,(ParserTypes.strip_position df.ParserTypes.name,i)::ll) in
	let one_arg get_argno sp =
		let argno = get_argno (ParserTypes.strip_position sp)
		in  argument (string_of_int argno) in *)
	let emit_one n =
		let is_dt = is_detached n in
		let is_ext = is_external n in
		let is_src = is_source n in
		let is_ro_src = is_runonce_source n in
		let nd = SymbolTable.lookup_node_symbol stable n in
		let f = nd.SymbolTable.functionname in
		(*let _,argmap = List.fold_left number_inputs (1,[]) nd.SymbolTable.nodeinputs in
		let get_argno n = 
			try List.assoc n argmap
			with Not_found -> raise (XMLConversion (n,ParserTypes.noposition)) in*)
		let do_gr gr = 
			(*let _,gr_pos,_ = gr.ParserTypes.guardname in*)
			guardref (ParserTypes.strip_position gr.ParserTypes.guardname)
			(string_of_int (unionmap_find (nd.SymbolTable.functionname,true)))
                        (HashString.hash (gr.ParserTypes.arguments,gr.ParserTypes.guardcond))
			(match gr.ParserTypes.modifiers with
				(ParserTypes.Read::_) -> "1"
				| (ParserTypes.Write::_) -> "2"
				| _ -> "3")
			in
		let is_eh = is_error_handler n in 
		let fl = Flow.flowmap_find n fmap in
		let sfun _ _ sc _ = sc in
		let chefun _ _ = Flow.null_flow in
		let coefun _ _ = Flow.null_flow in
		let nfun _ = Flow.null_flow in
                let n_in_u_n = find_union_number (n,true) in
                let n_out_u_n = find_union_number (n,false) in
		let succ = Flow.flow_apply (sfun,chefun,coefun,sfun,nfun) fl
		in  node n f 
                        (if is_src then "true" else "false")
			(if is_eh then "true" else "false")
			(if is_dt then "true" else "false")
			(if is_ext then "true" else "false")
                        (string_of_int n_in_u_n)
                        (string_of_int n_out_u_n)
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
			(successorlist ((gen_succ n_out_u_n [] succ)
				@ (if is_src && (not is_ro_src) then [successor "erste" [case n []]] else []))) in
	let guard_ff (ll,i) gname = 
		let element = guard gname (*(string_of_int i)*)
		in  (element::ll, i+1) in
        let rec order_node_list onfunl nl =
                match onfunl with
                        [] -> nl
                        | (onfunh::onfuntl) ->
                                let al,bl = List.partition onfunh nl
                                in  (order_node_list onfuntl al) @ bl
	in  let guard_elements, _ = List.fold_left guard_ff ([],1) guardlist in
            let _ = if CycleFind.detect br.guard_order_pairs then
                        raise (XMLConversion ("cycle detected in the guard precedence order specified",ParserTypes.noposition))
                    else () in
            let guardprecedence_elements = List.map 
                        (fun (b,a) -> guardprecedence b a)
                        br.guard_order_pairs in
	    let node_elements = List.map emit_one 
                        (let nodelist = order_node_list [is_source; is_runonce_source] nodelist
                        in  List.filter (fun n -> not (is_abstract n)) 
                                nodelist)
	    in  flow programname 
                        ( guard_elements
                        @ guardprecedence_elements
                        @ node_elements)


let emit_plugin_xml fn dependslist br_bef br_aft =
        let dependsxml = List.map depend dependslist in
        let get_key xml_node_or_guard = 
                let name = try List.assoc "name" (Xml.get_attributes xml_node_or_guard)
                        with Not_found -> ""
                in  (Xml.get_tag xml_node_or_guard)^name in
        let xml_bef = emit_program_xml "before" br_bef in
        let xml_aft = emit_program_xml "after" br_aft in
        let xml_compare xml1 xml2 = compare (get_key xml1) (get_key xml2) in
        let xml_sort xl = List.sort xml_compare xl in
        let xml_aft_contents = Xml.get_contents xml_aft in
        let xml_before_contents = Xml.get_contents xml_bef in
        let aft_old,aft_new = 
                let old_keys = List.map get_key xml_before_contents
                in  List.partition (fun norg -> List.mem (get_key norg) old_keys) xml_aft_contents in
        let _ = if not ((List.length aft_old) = (List.length xml_before_contents)) then
                        raise (XMLConversion ("something <internal> is wrong - please report this - number of nodes changed",ParserTypes.noposition))
                else () in
        let ordered_tags = (* tags whose content is ordered *)
                        [ xml_guardref_str
                        ; xml_successor_str
                        ; xml_case_str
                        ] in
        let same_val = Element ("samehere",[],[]) in
        let samehandler _ = same_val in
        let noposition = ParserTypes.noposition in
        let rec add_merge accumulated shorter longer =
                let equal x1 x2 =
                        ((Xml.get_tag x1) = (Xml.get_tag x2))
                        &&
                        ((Xml.get_attributes x1) = (Xml.get_attributes x2))
                        in
                let cond_add acc ll =
                        match acc with
                                [] -> ll
                                | _ -> (add (List.rev acc))::ll 
                in match shorter,longer with 
                        (sh::st,lh::lt) -> 
                                if equal sh lh then 
                                        cond_add accumulated (lh::(add_merge [] st lt))
                                else add_merge (lh::accumulated) shorter lt
                        | ([],_) -> cond_add ((List.rev longer) @ accumulated) []
                        | (_,[]) -> raise (XMLConversion ("something <internal> is wrong - add_merge failed",noposition))
                in
        let diffhandler befxml aftxml =
                let beftag = Xml.get_tag befxml in
                let befattribs = Xml.get_attributes befxml in
                let afttag = Xml.get_tag aftxml in
                let aftattribs = Xml.get_attributes aftxml in
                let sort_opt ll = if List.mem beftag ordered_tags then ll
                                else xml_sort ll in
                let _ = if not ((beftag=afttag) && (aftattribs = befattribs)) then
                                raise (XMLConversion ("plugin caused XML tag difference ["^beftag^"/"^afttag^"]",noposition))
                        else ()
                in  if (beftag = xml_successorlist_str) || (beftag = xml_successor_str) then
                        Element (beftag,befattribs,
                                add_merge [] 
                                        (sort_opt (Xml.get_contents befxml))
                                        (sort_opt (Xml.get_contents aftxml)))
                    else raise (XMLConversion ("plugin caused an unexpected tag to have a difference ["^beftag^"]",noposition))
                in
        let make_external xml_node =
                let on_attrib (k,v) = if k = "external" then (k,"true") else (k,v)
                in  Element (Xml.get_tag xml_node, List.map on_attrib (Xml.get_attributes xml_node), Xml.get_contents xml_node) in
        let simple_diff (nbef,naft) = Xml.diff ordered_tags diffhandler samehandler nbef naft in
        let before_changes = Xml.filter_list (fun x -> not (x=same_val))
                (List.map simple_diff (List.combine xml_before_contents aft_old)) in
        let pn = 
                let dotindex =  try String.rindex fn '.' 
                                with Not_found -> String.length fn 
                in  "lib"^(String.sub fn 0 dotindex)^".so"
        in  plugin (library pn dependsxml) 
                (aft_new @ (List.map make_external before_changes))

let write_xml xml filename =
	let xml_str = Xml.to_string_fmt xml in  
	let out = open_out filename in
	let _ = output_string out xml_str
	in  close_out out
	
