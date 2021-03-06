(*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *)
(* take a flow, create XML *)


open Xml
open Flow

(**
 general program structure is as follows:
  <flow name=... ofluxversion=...>
   <guard name=... magicnumber=.../>
   <node name=... source=[true|false] iserrhandler=[true|false] detached=[true|false] inputunionhash=... outputunionhash=...> <!-- name of the node -->
    <guardref name=... wtype=... hash=... late=.../>
    <errorhandler name=.../>
    <successorlist> <!-- a list of concurrent branches -- all are taken -->
      <successor name=...> <!-- a list of choices -->
       <case nodetarget=...> <!-- name of the node -->
		<!-- all must be satisfied to goto the node target -->
        <condition name=... argno=... isnegated=[true|false] unionnumberhash=.../>
        <condition name=... argno=... isnegated=[true|false] unionnumberhash=.../>
        <condition name=... argno=... isnegated=[true|false] unionnumberhash=.../>
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
let xml_unionhash_str = "unionhash"
let xml_inputunionhash_str = "inputunionhash"
let xml_outputunionhash_str = "outputunionhash"
let xml_wtype_str = "wtype"
let xml_detached_str = "detached"
let xml_external_str = "external"
let xml_isnegated_str = "isnegated"
let xml_iserrhandler_str = "iserrhandler"
let xml_case_str = "case"
let xml_successor_str = "successor"
let xml_successorlist_str = "successorlist"
let xml_source_str = "source"
let xml_door_str = "door"
let xml_node_str = "node"
let xml_nodetarget_str = "nodetarget"
let xml_flow_str = "flow"
let xml_errorhandler_str = "errorhandler"
let xml_plugin_str = "plugin"
let xml_library_str = "library"
let xml_add_str = "add"
let xml_remove_str = "remove"
let xml_depend_str = "depend"
let xml_hash_str = "hash"
let xml_before_str = "before"
let xml_after_str = "after"
let xml_late_str = "late"
let xml_gc_str = "gc"

let depend el_name =
        Element (xml_depend_str
                , [ xml_name_str, el_name
                  ]
                , [])

let condition el_name el_argno el_isnegated el_unionhash = 
	Element (xml_condition_str
		, [ xml_name_str,el_name
		  ; xml_argno_str,el_argno
		  ; xml_isnegated_str,el_isnegated
		  ; xml_unionhash_str,el_unionhash
		  ]
		, [])

let guard el_name el_gc =
	Element (xml_guard_str
		, [ xml_name_str, el_name
		  (*; xml_magicnumber_str, el_magicnumber*)
		  ; xml_gc_str, el_gc
		  ]
		, [])

let guardprecedence el_before el_after =
        Element (xml_guardprecedence_str
                , [ xml_before_str, el_before
                  ; xml_after_str, el_after
                  ]
                , [])

let guardref el_name el_unionhash el_hash el_wtype el_late =
	Element (xml_guardref_str
		, [ xml_name_str, el_name
		  ; xml_unionhash_str, el_unionhash
                  ; xml_hash_str, el_hash
		  ; xml_wtype_str, el_wtype
		  ; xml_late_str, el_late
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

let node el_name el_function el_source el_door el_iserrorhandler el_detached el_external el_inputunionhash el_outputunionhash guardrefs errorhandler_opt successorlist =
	Element(xml_node_str
		,[ xml_name_str,el_name
		 ; xml_function_str,el_function
		 ; xml_source_str,el_source
		 ; xml_door_str,el_door
		 ; xml_iserrhandler_str,el_iserrorhandler
		 ; xml_detached_str,el_detached
		 ; xml_external_str,el_external
		 ; xml_inputunionhash_str,el_inputunionhash
		 ; xml_outputunionhash_str,el_outputunionhash
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

let remove somethings = Element(xml_remove_str,[],somethings)

exception XMLConversion of string * ParserTypes.position

let rec break_dotted_name nsn =
	try let ind = String.index nsn '.' in
	    let len = String.length nsn in
	    let hstr = String.sub nsn 0 ind in
	    let tstr = String.sub nsn (ind+1) (len-(ind+1))
	    in  hstr::(break_dotted_name tstr)
	with Not_found -> [nsn]

let explain_union_number br u_n =
	(** nice string that tells you the story of u_n *)
        let conseq_res = br.Flow.consequences in
	let g_ins  nd = Some nd.SymbolTable.nodeinputs in
	let g_outs nd = nd.SymbolTable.nodeoutputs in
	let stable = br.Flow.symtable in
	let foldstr ll =
		let strconcat a b = a^b 
		in  List.fold_left strconcat "" ll in
	let df_tos df =
		(ParserTypes.strip_position df.ParserTypes.ctypemod)^" "
		^(ParserTypes.strip_position df.ParserTypes.ctype)^" "
		^(ParserTypes.strip_position df.ParserTypes.name)^", " in
	let rec get_dfl iol =
		match iol with
			((h,isi)::t) ->
				let f = if isi then g_ins else g_outs in
				(match f (SymbolTable.lookup_node_symbol_from_function 
					stable h) with
					(Some dfl) -> dfl
					| None -> get_dfl t)
			| _ -> raise Not_found
		in
	let describe_ios_struct ios = 
		foldstr (List.map df_tos (get_dfl ios)) in
	let synonyms_ios =
		List.map (fun (x,_) -> x)
		(List.filter (fun (_,n) -> n = u_n) conseq_res.TypeCheck.union_map) in
	let synonyms =
		List.map (fun (x,isi) -> x^(if isi then "_in" else "_out"))
			synonyms_ios 
	in  (" synonyms: "^(foldstr (List.map (fun s -> s^", ") synonyms))
		^"\n containing data members: "^(describe_ios_struct synonyms_ios))

let emit_program_xml' programname br usesmodel = 
        (*let conseq_res = br.Flow.consequences in*)
        (*let unionmap_find strio =
                let u_n = TypeCheck.get_union_from_strio conseq_res strio
                in  u_n in*)
	let stable = br.Flow.symtable in
	let fmap = br.Flow.fmap in
	let sourcelist = br.Flow.sources in
	let doorlist = br.Flow.doorsources in
	let errhandlers = br.Flow.errhandlers in
	let is_detached n = SymbolTable.is_detached stable n in
	let is_external n = SymbolTable.is_external stable n in
	let is_abstract n = SymbolTable.is_abstract stable n in
	let is_door n = List.mem n doorlist in
	let is_source n = (not (is_door n)) && (List.mem n sourcelist) in
	let is_runonce_source n = List.mem n br.Flow.runoncesources in
        let is_terminate n = List.mem n br.Flow.terminates in
	let get_node n _ ll = if Flow.is_concrete stable fmap n then n::ll else ll in
	let get_guard n gd ll = ((n,gd.SymbolTable.ggc)::ll) in
	let guardlist = SymbolTable.fold_guards get_guard stable [] in 
        let guardlist = List.rev guardlist in
	let nodelist = Flow.flowmap_fold get_node fmap [] in
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
				(Uniquify.uniq_discard compare (h1@h2))::(and_canon_condition t1 t2) in
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
	(*let gdecll = TypeCheck.get_decl_list_from_union conseq_res stable in*)
        let convert_argn (t_u_h,t_dfl) (u_h,dfl) j =
                if t_u_h = u_h then j
                else 
                try 
                        (*let t_is = gdecll t_u_n in
                        let is = gdecll u_n in*)
                        let rec pick j ll = 
                                match ll with
                                        (h::t) ->
                                                if j <= 1 then h
                                                else pick (j-1) t 
                                        | _ -> raise Not_found in
                        let df = (pick j t_dfl) in
                        let name = ParserTypes.strip_position df.ParserTypes.name in
                        let rec find i dfl =
                                match dfl with
                                        (h::t) -> 
                                                if (ParserTypes.strip_position h.ParserTypes.name)=name then i
                                                else find (i+1) t
                                        | _ -> raise (XMLConversion ("can't find arg conversion "
							^(t_u_h)^" -> "
							^(u_h),ParserTypes.noposition))
                        in  find 1 dfl 
                with Not_found -> j
                in
	let rec gen_cond' (t_u_h,t_dfl) (u_h,dfl) i ccond =
		match ccond with
			(h::t) ->
				(List.map (fun (s,neg) -> 
                                        condition s 
                                                (let i = convert_argn (t_u_h,t_dfl) (u_h,dfl) i 
                                                in  string_of_int i) 
                                                (if neg then "true" else "false")
                                                (u_h)) h)
				@ (gen_cond' (t_u_h,t_dfl) (u_h,dfl) (i+1) t)
			| _ -> [] in
	let gen_cond (t_u_h,t_dfl) (u_h,dfl) i ccond =
		List.rev (gen_cond' (t_u_h,t_dfl) (u_h,dfl) i ccond) in
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
	(*let find_union_number (y,io) = 
		let nd = SymbolTable.lookup_node_symbol stable y
		in  unionmap_find (nd.SymbolTable.functionname,io) in*)
	let find_decl_formal_opt (y,io) =
		let nd = SymbolTable.lookup_node_symbol stable y in
		let dfo = if io then
			Some nd.SymbolTable.nodeinputs
			else nd.SymbolTable.nodeoutputs
		in  dfo in
	(*let find_union_hash (y,io) = 
		match find_decl_formal_opt (y,io) with
			(Some df) -> ParserTypes.hash_decl_formal_list df
			| _ -> "" in*)
	let rec gen_succ' (n_out_uh,n_out_dfl) ccond fl = 
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
			(*let pp_ccond ll =
				let str_c (s,isn) = (if isn then "!" else "")^s in
				let pp_cs ll = 
					begin
					List.iter (fun x -> print_string ((str_c x)^", ")) ll ; 
					print_string "; "
					end
				in  List.iter pp_cs ll in*)
			let u_h,u_dfl = (*FIXME*)
				if (List.length ccond) = (List.length n_out_dfl)
				then (n_out_uh, n_out_dfl)
				else 
					let dfo = find_decl_formal_opt (n,true) 
					in  match dfo with
						None -> ("",[])
						| (Some dfl) -> (ParserTypes.hash_decl_formal_list dfl, dfl) in
                        let ist = (if CmdLine.get_abstract_termination() then is_abstract n else false ) || (is_terminate n) in
                        let _ = if (not ist) && (is_abstract n) then 
				    raise (XMLConversion ("the node "
					^n^" is abstract, but it is not defined"
					, ParserTypes.noposition))
                                else ()
			in  if ist || (is_condition_always_false ccond) then
                                []
                            else  (*let _ = (print_string "sfun - gencond on "
					; pp_ccond ccond
					; print_string "\n"
					; print_string ("n="^n^" u_n="^(string_of_int u_n)
						^" n_out_u_n="^(string_of_int n_out_u_n)
						^"\n"))
				in*)
				[[case n (gen_cond (u_h,u_dfl) (n_out_uh,n_out_dfl) 1 ccond)]] in
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
                                in  condneg, ((gen_succ' (n_out_uh,n_out_dfl) ccondlocal flr)::sofar) in
			let _,part_res = List.fold_left onfold (condunit,[]) solfl in
			let tmp = List.fold_left 
					(if CmdLine.get_exclusive_tests() 
					then List.append 
					else (prod (fun a -> (fun b -> b @ a))))
				[[]] part_res 
                        in  tmp in
                let coefun _ fll = 
                        let tmp = List.concat (List.map (gen_succ' (n_out_uh,n_out_dfl) ccond) fll) 
			(****************************
			 * so basically concatenate the successor lists
			 ****************************)
                        in  tmp in
		let nfun _ = []
		in  Flow.flow_apply (sfun,chefun,coefun,sfun,nfun) fl
		in
	let min_ns ns1 ns2 =
		if (List.mem (ns1,ns2) usesmodel)
                || ((List.mem (ns1,"") usesmodel) && (List.mem ("",ns2) usesmodel)) then
			ns2
		else ns1 in
	let make_successor imap caselist =
		let get_ns str =
			let ns = match break_dotted_name str with
					(h::_::_) -> h
					| _ -> "" 
			in  try let mid = SymbolTable.lookup_module_instance stable ns
			        in  mid.SymbolTable.modulesource
			    with Not_found -> ns in
		let rec find_earliest_ns nsopt cl =
			match nsopt,cl with
				(None,((Element (_,[_,ntargetstr],_))::t)) ->
					find_earliest_ns
						(Some (get_ns ntargetstr))
						t
				|(Some ns,((Element (_,[_,ntargetstr],_))::t)) ->
					let ns = min_ns ns (get_ns ntargetstr)
					in  find_earliest_ns (Some ns) t
				|(Some ns,[]) -> ns
				|(None,[]) -> ""
				| _ -> raise (XMLConversion ("make_successor:unexpected element structure", ParserTypes.noposition)) in
		let get_iv ns = 
			try let iref = List.assoc ns imap in
			    let _ = iref := (!iref) +1
			    in imap,!iref
			with Not_found -> 
				((ns,ref 0)::imap),0 in
		let imap,nsprefix_succname = 
			let ns = find_earliest_ns None (List.rev caselist) in
			let imap,i = get_iv ns
			in  imap,((if (String.length ns) > 0 then (ns^".") else "")^(string_of_int i))
		in (successor (nsprefix_succname) caselist), imap in
	let gen_succ (n_out_uh,n_out_dfl) ccond fl =
                let foldfun (resl,imap) s =
                        if s = [] then resl,imap 
			else 
			    let r,imap = make_successor imap s
			    in (r::resl),imap
                        in
                let resl,_ = List.fold_left foldfun ([],[]) 
                        (List.rev (gen_succ' (n_out_uh,n_out_dfl) ccond fl))
                in resl in
	let is_error_handler n = List.mem n errhandlers in
	(*let number_inputs (i,ll) df = (i+1,(ParserTypes.strip_position df.ParserTypes.name,i)::ll) in
	let one_arg get_argno sp =
		let argno = get_argno (ParserTypes.strip_position sp)
		in  argument (string_of_int argno) in *)
	(*let rec determine_wtype gtype_str modifiers =
		match gtype_str, modifiers with
			("readwrite",(ParserTypes.Read::_)) -> "1"
			| ("readwrite",(ParserTypes.Write::_)) -> "2"
			| ("readwrite",(ParserTypes.Upgradeable::_)) -> "4"
			| ("pool",[]) -> "5" 
			| ("exclusive",[]) -> "3"
			| ("free",[]) -> "6"
			| ("readwrite",(_::t)) -> determine_wtype gtype_str t
			| _ -> raise Not_found in *)
	let determine_wtype gty gmlist =
		string_of_int (WType.wtype_of gty gmlist) in
	let emit_one n =
		let is_dt = is_detached n in
		let is_ext = is_external n in
		let is_src = is_source n in
		let is_dr = is_door n in
		let is_ro_src = is_runonce_source n in
		let nd = SymbolTable.lookup_node_symbol stable n in
		let f = nd.SymbolTable.functionname in
		(*let _,argmap = List.fold_left number_inputs (1,[]) nd.SymbolTable.nodeinputs in
		let get_argno n = 
			try List.assoc n argmap
			with Not_found -> raise (XMLConversion (n,ParserTypes.noposition)) in*)
		let do_gr gr = 
			let gname,gr_pos,_ = gr.ParserTypes.guardname in
			let gd = SymbolTable.lookup_guard_symbol stable gname in
			let has_gargs = List.exists
				(fun xl -> List.exists
					(fun une ->
						(match une with
							(ParserTypes.GArg _) -> true
							| _ -> false)) xl) 
				(gr.ParserTypes.guardcond::gr.ParserTypes.arguments)
			in
			try 
			(*let _,gr_pos,_ = gr.ParserTypes.guardname in*)
			guardref 
				(ParserTypes.strip_position gr.ParserTypes.guardname)
				(ParserTypes.hash_decl_formal_list nd.SymbolTable.nodeinputs)
				(HashString.hash (n,gr.ParserTypes.arguments,gr.ParserTypes.guardcond))
				(determine_wtype gd.SymbolTable.gtype gr.ParserTypes.modifiers)
				(if has_gargs then "true" else "false")
			with Not_found ->
				raise (XMLConversion ("bad guard mode (Read/Write/Exclusive/None/Pool) on " ^ gname,gr_pos))
			in
		let is_eh = is_error_handler n in 
		let fl = Flow.flowmap_find n fmap in
		let sfun _ _ sc _ = sc in
		let chefun _ _ = Flow.null_flow in
		let coefun _ _ = Flow.null_flow in
		let nfun _ = Flow.null_flow in
                (*let n_in_u_n = find_union_number (n,true) in
                let n_out_u_n = find_union_number (n,false) in*)
		let n_in_uh = ParserTypes.hash_decl_formal_list nd.SymbolTable.nodeinputs in
		let n_out_uh,n_out_dfl = match nd.SymbolTable.nodeoutputs with
					None -> "",[]
					| (Some dfl) -> (ParserTypes.hash_decl_formal_list dfl)
						, dfl in
		let succ = Flow.flow_apply (sfun,chefun,coefun,sfun,nfun) fl
		in  node n f 
                        (if is_src then "true" else "false")
                        (if is_dr then "true" else "false")
			(if is_eh then "true" else "false")
			(if is_dt then "true" else "false")
			(if is_ext then "true" else "false")
                        (n_in_uh)
                        (n_out_uh)
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
			(successorlist ((gen_succ (n_out_uh,n_out_dfl) [] succ)
				@ (if is_src && (not is_ro_src) then [successor "erste" [case n []]] else []))) in
	let guard_ff (ll,i) (gname,gc) = 
		let element = guard gname (*(string_of_int i)*) 
				(if gc then "true" else "false")
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

let emit_program_xml programname br = 
	emit_program_xml' programname br []

let emit_plugin_xml fn dependslist br_bef br_aft usesmodel =
        let dependsxml = List.map depend dependslist in
        let get_key xml_node_or_guard = 
                let name = 
			let attribs = (Xml.get_attributes xml_node_or_guard)
			in
			try List.assoc "name" attribs
                        with Not_found -> 
				(try let prec_bef = List.assoc "before" attribs in
				    let prec_aft = List.assoc "after" attribs
				    in prec_bef^"<"^prec_aft
				with Not_found -> "")
                in  (Xml.get_tag xml_node_or_guard)^name in
        let key_diff xmlcontents1 xmlcontents2 =
                let kxml x = (get_key x, x) in
                let sort kxmlcontents = List.sort (fun (k1,_) -> (fun (k2,_) -> compare k1 k2)) kxmlcontents in
                let rec diff_str kxc1 kxc2 =
                        match kxc1,kxc2 with
                                ((k1,v1)::t1,(k2,v2)::t2) ->
                                        let cres = compare k1 k2 in
                                        if cres = 0 then diff_str t1 t2
                                        else if cres < 0 then ("- "^(Xml.to_string_fmt v1)^"\n"^(diff_str t1 kxc2))
                                        else ("+ "^(Xml.to_string_fmt v2)^"\n"^(diff_str kxc1 t2))
                                | ([],(k2,v2)::t2) -> ("+ "^(Xml.to_string_fmt v2)^"\n"^(diff_str kxc1 t2))
                                | ((k1,v1)::t1,[]) -> ("- "^(Xml.to_string_fmt v1)^"\n"^(diff_str t1 kxc2)) 
                                | _ -> "" in
                let oneachside xmlc = sort (List.map kxml xmlc)
                in  diff_str (oneachside xmlcontents1) (oneachside xmlcontents2) in

        let xml_bef = emit_program_xml' "before" br_bef usesmodel in
        let xml_aft = emit_program_xml' "after" br_aft usesmodel in
        let xml_compare xml1 xml2 = compare (get_key xml1) (get_key xml2) in
        let xml_sort xl = List.sort xml_compare xl in
        let xml_aft_contents = Xml.get_contents xml_aft in
        let xml_before_contents = Xml.get_contents xml_bef in
        let aft_old,aft_new = 
                let old_keys = List.map get_key xml_before_contents
                in  List.partition (fun norg -> List.mem (get_key norg) old_keys) xml_aft_contents in
        let _ = if not ((List.length aft_old) = (List.length xml_before_contents)) then
			let expressive_error_context = key_diff aft_old xml_before_contents
                        in  raise (XMLConversion (("something <internal> is wrong - please report this - number of nodes changed:\n"^expressive_error_context),ParserTypes.noposition))
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
                        | (_,[]) -> 
				(remove shorter)::(cond_add accumulated [])
				(*raise (XMLConversion ("something <internal> is wrong - add_merge failed",noposition))*)
                in
	let rec findattribdiff ll1 ll2 =
		match ll1,ll2 with
			([],[]) -> raise Not_found
			| ((a1,v1)::t1,(a2,v2)::t2) ->
				if not (a1 = a2) then
					a1,v1,"<unknown1>"
				else if (a1,v1) = (a2,v2) then
					findattribdiff t1 t2
				else a1,v1,v2
			| ([],(a2,v2)::_) ->
				a2,"<unknown2>",v2
			| ((a1,v1)::_,[]) ->
				a1,v1,"<unknown3>"
			in
        let diffhandler befxml aftxml =
                let beftag = Xml.get_tag befxml in
                let befattribs = Xml.get_attributes befxml in
                let afttag = Xml.get_tag aftxml in
                let aftattribs = Xml.get_attributes aftxml in
                let sort_opt ll = if List.mem beftag ordered_tags then ll
                                else xml_sort ll in
		let get_name_ifthere attribs =
			(try    let na = List.assoc xml_name_str attribs
				in ("(name="^na^")")
			with Not_found -> "") in
		let bef_name_ifthere = get_name_ifthere befattribs in
		let aft_name_ifthere = get_name_ifthere aftattribs in
		(*let is_unionhash attrib =
			List.mem attrib [ xml_unionhash_str
					; xml_inputunionhash_str
					; xml_outputunionhash_str ] in*)
		(*let explain_un br unstr = 
			explain_union_number br (int_of_string unstr) in*)
                let _ = if not (beftag=afttag) then
                                raise (XMLConversion ("plugin caused XML tag difference ["
					^beftag^"/"^afttag^"]"
					^"(name attributes "
					^bef_name_ifthere^"/"^aft_name_ifthere^")"
					,noposition))
			else if not (aftattribs = befattribs) then
				let attr_name,bef_val,aft_val = 
					findattribdiff befattribs aftattribs
                                in  raise (XMLConversion ("plugin caused XML attribute difference on a "
					^beftag^" tagged node "
					^bef_name_ifthere
					^"\n   before/after attribute "^attr_name
					^" ["^bef_val^"/"^aft_val^"]"
					(*^(if is_unionnumber attr_name then
						"\nbefore: "^bef_val
						^"\n"^(explain_un br_bef bef_val)
						^"\nafter: "^aft_val
						^"\n"^(explain_un br_aft aft_val)
					 else "")*)
					,noposition))
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
	
