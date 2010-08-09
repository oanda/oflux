
open ParserTypes

module StringKey =
struct 
	type t = string
	let compare = compare
end

module StringMap = Map.Make(StringKey)

type node_data = 
                { functionname: string
		; nodeinputs: decl_formal list 
		; nodeoutputs: decl_formal list option
		; nodeguardrefs: guardref list
		; where: position 
		; nodedetached: bool 
                ; nodeabstract: bool
                ; nodeexternal: bool
                }

type conditional_data = 
		{ arguments: decl_formal list 
		; cfunction: string
		}

type guard_data = 
                { garguments: decl_formal list
		; gtype: string
                ; gexternal: bool
		; return: data_type 
		; magicnumber: int 
                ; gunordered: bool
                ; ggc: bool
                }

type module_inst_data =
		{ modulesource : string }

type symbol = 
	Node of node_data
	| Conditional of conditional_data
	| Guard of guard_data
	| ModuleDef of module_def_data
	| ModuleInst of module_inst_data
and module_def_data = { modulesymbols : symbol_table }
and symbol_table = symbol StringMap.t

let empty_symbol_table = StringMap.empty

(* conditional functions *)

let add_conditional symtable c =
	StringMap.add (strip_position c.condname)
		(Conditional { arguments=c.condinputs; cfunction=c.condfunction }) symtable

let lookup_conditional_symbol symtable name =
	match StringMap.find name symtable with
		(Conditional x) -> x
		| _ -> raise Not_found

let fold_conditionals f symtable w =
	let smf n sym w =
		match sym with
			(Conditional nd) -> f n nd w
			| _ -> w
	in  StringMap.fold smf symtable w

(* guard functions *)

let next_magic_no = ref 1

let add_guard symtable g =
	let mn = !next_magic_no in
	let gname = strip_position g.atomname in
	let _ = Debug.dprint_string ("guard "^gname^" added to sym table \n") in
	let res = StringMap.add gname (Guard 
		{ garguments=g.atominputs
		; return=g.outputtype
		; gtype=g.atomtype
                ; gexternal=g.externalatom
		; magicnumber = mn
		; gunordered = List.mem "unordered" g.atommodifiers
                ; ggc = List.mem "gc" g.atommodifiers
                }) symtable in
	let _ = next_magic_no := mn+1
	in  res

let lookup_guard_symbol symtable name =
	match StringMap.find name symtable with
		(Guard x) -> x
		| _ -> raise Not_found

let fold_guards f symtable w =
	let smf n sym w =
		match sym with
			(Guard g) -> f n g w
			| _ -> w
	in  StringMap.fold smf symtable w

(* node symbols *)

let add_node_symbol symtable n =
	let name,pos,_ = n.nodename
	in  StringMap.add name (Node 
		{ functionname=n.nodefunction
		; nodeinputs=n.inputs
		; nodeoutputs=n.outputs
		; nodeguardrefs=n.guardrefs
		; where=pos
		; nodedetached=n.detached 
                ; nodeabstract=(n.abstract || n.outputs = None)
                ; nodeexternal=n.externalnode
                }) symtable

let lookup_node_symbol symtable name =
	match StringMap.find name symtable with
		(Node x) -> x
		| _ -> raise Not_found
		
let fold_nodes f symtable w =
	let smf n sym w =
		match sym with
			(Node nd) -> f n nd w
			| _ -> w
	in  StringMap.fold smf symtable w

let lookup_node_symbol_from_function symtable name =
	let ffun n nd son =
		match son with
			None -> if nd.functionname = name then Some nd else None
			| _ -> son
	in  match fold_nodes ffun symtable None with
		None -> raise Not_found
		| (Some x) -> x

let is_detached symtable name =
	let x = lookup_node_symbol symtable name
	in  x.nodedetached

let is_abstract symtable name =
	let x = lookup_node_symbol symtable name
	in  x.nodeabstract

let is_external symtable name =
	let x = lookup_node_symbol symtable name
	in  x.nodeexternal


exception DeclarationLookup of string

let get_decls stable (name,isin) =
	try let nd = lookup_node_symbol_from_function stable name
	    in  if isin then nd.nodeinputs 
	    	else (match nd.nodeoutputs with
			None -> raise (DeclarationLookup ("node "^name^" is abstract and cannot be used for code generation"))
			| (Some x) -> x)
	with Not_found -> raise (DeclarationLookup ("could not resolve node "^name))

(* deprecated
* so called typedefs *

let add_type_def symtable ((typename,tnamepos,_),def) =
	StringMap.add typename (Typedef (def,ref None,tnamepos)) symtable

let update_arg_type symtable typename argtype =
	let _,arg_type,_ = (StringMap.find typename symtable) in
	let _ = arg_type := Some argtype
	in ()

let lookup_typedef_symbol symtable name =
	match StringMap.find name symtable with
		(Typedef (n1,n2,n3)) -> (n1,n2,n3)
		| _ -> raise Not_found
*)



(** modules *)

let add_module_instance symtable mi =
	let name,pos,_ = mi.modinstname
	in  StringMap.add name (ModuleInst 
		{ modulesource= strip_position mi.modsourcename }) symtable

let rec add_module_definition symtable md =
	let name, pos, _ = md.modulename in
	let msymtable = add_program empty_symbol_table md.programdef
	in  StringMap.add name (ModuleDef
		{ modulesymbols = msymtable }) symtable
(** programs *)
and add_program symboltable p =
        let node_decls = p.node_decl_list in
        let cond_decls = p.cond_decl_list in
        let atom_decls = p.atom_decl_list in
	let module_insts = p.mod_inst_list in
	let module_decls = p.mod_def_list in
        let symboltable = List.fold_left add_node_symbol symboltable                node_decls in
        let symboltable = List.fold_left add_conditional symboltable                cond_decls in
        let symboltable = List.fold_left add_guard symboltable
                atom_decls in
        let symboltable = List.fold_left add_module_instance symboltable
                module_insts in
        let symboltable = List.fold_left add_module_definition symboltable
                module_decls 
	in  symboltable

let lookup_module_definition symtable name =
	match StringMap.find name symtable with
		(ModuleDef x) -> x
		| _ -> raise Not_found

let lookup_module_instance symtable name =
	match StringMap.find name symtable with
		(ModuleInst x) -> x
		| _ -> raise Not_found

let fold_module_definitions f symtable w =
	let smf n sym w =
		match sym with
			(ModuleDef nd) -> f n nd w
			| _ -> w
	in  StringMap.fold smf symtable w

let fold_module_instances f symtable w =
	let smf n sym w =
		match sym with
			(ModuleInst nd) -> f n nd w
			| _ -> w
	in  StringMap.fold smf symtable w

let strip_position3 df = 
	( strip_position df.ctypemod
	, strip_position df.ctype
	, strip_position df.name) 

(* basic type unification *)


let get_module_uses_model symtable =
        (** reflective transitive closure of the USES relation 
                i.e A USES B means (A,B) is in the result
        *)
        let find_inst depth _ midata res =
                (List.map (fun x -> x,midata.modulesource) depth)
                @ res in
        let rec find_def mname mdef (depth,result) =
                let depth = mname::depth in
                let result = (mname,mname)::result in
                let result = fold_module_instances (find_inst depth) mdef.modulesymbols result 
                in  fold_module_definitions find_def 
                        mdef.modulesymbols (depth,result) in  
        let find_def' mname mdef (depth,result) =
                let _, r = find_def mname mdef (depth,result)
                in  ([],r) in
        let _,result = fold_module_definitions find_def' symtable ([],[])
        in result

