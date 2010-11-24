
let wtype_of guarddeclstr gmlist =
	let rec determine_wtype gtype_str modifiers =
		match gtype_str, modifiers with
			("readwrite",(ParserTypes.Read::_)) -> 1
			| ("readwrite",(ParserTypes.Write::_)) -> 2
			| ("readwrite",(ParserTypes.Upgradeable::_)) -> 4
			| ("pool",[]) -> 5 
			| ("exclusive",[]) -> 3
			| ("free",[]) -> 6
			| ("readwrite",(_::t)) -> determine_wtype gtype_str t
			| _ -> raise Not_found 
	in  determine_wtype guarddeclstr gmlist


let wtype_for stable gname gmlist =
	let gd = SymbolTable.lookup_guard_symbol stable gname 
	in wtype_of gd.SymbolTable.gtype gmlist
