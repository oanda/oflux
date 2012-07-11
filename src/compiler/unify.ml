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

open ParserTypes
open SymbolTable

module StringKey =
struct 
	type t = string
	let compare = compare
end

module StringMap = Map.Make(StringKey)

(* basic type unification *)

type success_type = Weak | Strong

type unify_result =
	Success of success_type
	| Fail of int * string 
		(* ith, reason *)

let strip_position3 df = SymbolTable.strip_position3 df

let unify_single (ctm1,t1,_) (ctm2,t2,_) =
	let rec cleanse_amp' l s =
		if l > 0 && (s.[l-1] = '&') 
		then cleanse_amp' (l-1) (String.sub s 0 (l-1))
		else s
		in
	let cleanse_amp s = cleanse_amp' (String.length s) s in
	match ((cleanse_amp t1) = (cleanse_amp t2)), ctm1, ctm2 with
		(false,_,_) -> Some "mismatched types"
		| (_,"const","") -> None
		| (_,"","const") -> Some ("cannot drop const")
		| _ -> 
			if ctm1 = ctm2 then None
			else Some ("mismatched type modifiers")

module StrongU =
struct

        let unify_type_in_out (tl_in: decl_formal list) (tl_out: decl_formal list) =
                let rec unify_type_in_out' i tl_in tl_out =
                        match tl_in, tl_out with
                                ([],[]) -> Success Strong
                                | (tinh::tintl,touh::toutl) ->
                                        (match unify_single
                                                        (strip_position3 tinh) 
                                                        (strip_position3 touh) with
                                                None -> unify_type_in_out' (i+1) tintl toutl
                                                | (Some reason) -> Fail (i,reason))
                                | _ -> Fail (i,"one more argument between unified parameter lists")
                in  unify_type_in_out' 0 tl_in tl_out
                                                

end

module WeakU =
struct

        let compare_df df1 df2 =
                let reorder (ctm,ct,n) = n,ct,ctm
                in  compare (reorder (strip_position3 df1))
                        (reorder (strip_position3 df2))

        let unify_type_in_out (tl_in: decl_formal list) 
                        (tl_out: decl_formal list) =
                let tlin = List.sort compare_df tl_in in
                let tlout = List.sort compare_df tl_out in
                let rec asym_merge (i,tin) tout =
                        match tin,tout with
                                (htin::ttin,htout::ttout) ->
                                        (match unify_single (strip_position3 htin) (strip_position3 htout) with
                                                None ->
                                                        asym_merge (i+1,ttin) ttout
                                                | _ ->
                                                        if (compare_df htin htout) < 0 then 
                                                                Fail (i,"this input parameter is unmatched")
                                                        else asym_merge (i,tin) ttout
                                        )
                                | (htin::_,[]) -> Fail (i,"this input parameter is unmatched")
                                | ([],_) -> Success Weak
                in  asym_merge (0,tlin) tlout
                
end


let choose (f1,f2) a1 a2 =
        if CmdLine.get_weak_unify () then
                match f1 a1 a2 with
                        (Fail _) -> f2 a1 a2
                        | r -> r
        else f1 a1 a2

let unify_type_in_out a1 a2 = 
        choose (StrongU.unify_type_in_out,WeakU.unify_type_in_out) a1 a2

let unify' (io1,io2) symtable node1 node2 =
        let pick io x = if io then Some x.nodeinputs else x.nodeoutputs
        in  try let nd1 = lookup_node_symbol symtable node1 in
                let nd2 = lookup_node_symbol symtable node2
                in  match pick io1 nd1, pick io2 nd2 with
                        (Some as1, Some as2) -> 
                                (*if io1 = io2 then
                                        StrongU.unify_type_in_out as1 as2
                                else*) unify_type_in_out as1 as2
                        | _ -> Success Weak
            with Not_found->
                Fail (-1,"node not found in symbol table")


let unify symtable node1 node2 = unify' (true,false) symtable node1 node2
