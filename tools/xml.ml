(** simple xml code for generation *)

type xml = Element of (string * (string * string) list * xml list)
        (* PCData section is not needed *)

let to_string_fmt xmlinp = (* returns a string *)
        let buff = Buffer.create 1024 in
        let add_to_buffer b s = Buffer.add_string b s; b in
        let add_strlist b sl = 
                let _ = List.fold_left add_to_buffer b sl
                in  () in
        let write_attrib (n,v) =
                let _ = add_strlist buff
                        [ " "
                        ; n
                        ; "=\""
                        ; v
                        ; "\"" ]
                in  () in
        let rec write tab xmlinp =
                match xmlinp with
                        (Element (tag,alist,ll)) ->
                                begin
                                add_strlist buff
                                        [ tab
                                        ; "<"
                                        ; tag ];
                                List.iter write_attrib alist;
                                (match ll with 
                                        [] -> Buffer.add_string buff "/>\n"
                                        | _ ->
                                        begin
                                        Buffer.add_string buff ">\n";
                                        List.iter (write (tab^" ")) ll;
                                        add_strlist buff
                                                [ tab
                                                ; "</"
                                                ; tag
                                                ; ">\n" ];
                                        ()
                                        end
                                )
                                end
        in  begin
            write "" xmlinp; 
            Buffer.contents buff
            end

