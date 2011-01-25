
type position = { file: string option
		; lineno: int 
		; characterno: int }

let noposition = { file=None; lineno=0 ; characterno=0 }

let position_to_string p =
	if (p.lineno + p.characterno = 0) && p.file = None then ""
	else    let file = match p.file, CmdLine.get_root_filename () with
				((Some f,_) | (_,Some f)) -> f
				| _ -> "?"
		in  ("[f:"^file
		    ^" l:"^(string_of_int p.lineno)
		    ^" c:"^(string_of_int p.characterno)^"]")

type 'a positioned = 'a * position * position

type mainfun =  { sourcename: string positioned
		; sourcefunction : string 
		; successor: string positioned option 
                ; runonce: bool
                }

type data_type = { dctypemod: string positioned
		; dctype: string positioned }

type decl_formal = 
		{ ctypemod: string positioned 
		; ctype: string positioned 
		; name: string positioned
		}

type guardmod = Read | Write | Upgradeable | NullOk

type uninterp_expr =
        Arg of string (* normal node argument *)
        | GArg of string (* guard argument *)
        | Context of string (* context stuff *)


type guardref = { guardname: string positioned
                ; arguments: uninterp_expr list list
                ; modifiers: guardmod list
                ; localgname: string positioned option
                ; guardcond: uninterp_expr list
                }

type node_decl = 
	{ detached: bool
	; abstract: bool
	; ismutable: bool
        ; externalnode: bool
	; nodename: string positioned 
	; nodefunction: string
	; inputs: decl_formal list 
	; guardrefs: guardref list
	; outputs: decl_formal list option
	}

type cond_decl =
	{ externalcond: bool
        ; condname: string positioned
	; condfunction: string
	; condinputs: decl_formal list
	}

type atom_decl =
	{ atomname: string positioned
	; atominputs: decl_formal list
	; outputtype: data_type
	; atomtype: string
        ; externalatom: bool
        ; atommodifiers : string list
	}

type ident = string positioned 

type comma_item = Star | Ident of string positioned

type expr_type = Choice | Concurrent

type expr = 
	{ exprname: string positioned 
	; condbinding: comma_item list 
	; successors: ident list
        ; etype: expr_type
	}

type err = 
	{ onnodes: ident list 
	; handler: string positioned
	}

type mod_inst = 
	{ modsourcename : string positioned 
        ; externalinst : bool
	; modinstname : string positioned
        ; guardaliases : (string positioned * string positioned) list
	}

type order_decl = string positioned * string positioned

type program = 
	{ cond_decl_list: cond_decl list 
	; atom_decl_list: atom_decl list
	; node_decl_list: node_decl list
	; mainfun_list: mainfun list
	; expr_list: expr list 
	; err_list: err list
	; mod_def_list: mod_def list
	; mod_inst_list: mod_inst list
        ; plugin_list: plugin_def list
	; plugin_depend_list: string positioned list
        ; terminate_list: string positioned list
        ; order_decl_list: order_decl list
	}
and mod_def = 
	{ modulename : string positioned 
	; isstaticmodule : bool
	; programdef : program
	}
and plugin_def =
        { pluginname : string positioned
        ; pluginprogramdef : program
        }

let trace_thing _ = ()

let strip_position (x,_,_) = x

let hash_decl_formal_list dfl =
	let break df = (strip_position df.ctypemod),(strip_position df.ctype),(strip_position df.name) in
	let noposl = List.map break dfl
	in  HashString.hash noposl
