# TODO LIST
use("io", "file","list", "db", "errors", "types")

var todos = List.new() # cache 
var description = List.new() # cache 

def add_new_item -> name, desc then 
        List.push($todos, $name)
        List.push($description, $desc)
end


var db_out = file.read_to_str("todo.db")
if errors.bool($db_out) equ false then 
        var todo_db = db.to_lists($db_out)
        # TODO DB ARCH: 0 -> TODOS, 1 -> DESC
        var todo_db_todos = List.get($todo_db, 0)
        var todo_db_descs = List.get($todo_db, 1)
 
        while List.iter($todo_db_todos) and List.iter($todo_db_descs) then 
                var todo_n = List.next($todo_db_todos)
                var desc_s = List.next($todo_db_descs)
                List.push($todos, $todo_n)
                List.push($description, $desc_s)
        end
end

while true then 
        var user_input = types.str_to_int(io.input("Enter an operation [ LIST: 1 | ADD: 2 | REMOVE: 3 | 4 QUIT]:"))
        if $user_input equ 2 then 
                var mission = io.input("Enter mission:")
                var desc = io.input("Enter description:")
                add_new_item($mission, $desc)                 
        elif $user_input equ 1 then
                io.print("-----------------------------------------\n")
                while List.iter($todos) and List.iter($description) then
                        var todo_n = List.next($todos)
                        var desc_s = List.next($description)
                        io.print($todo_n, ": ", $desc_s, "\n")
                end
                io.print("-----------------------------------------\n")
        elif $user_input equ 3 then 
                io.print("Items are indexed from 0, increasing from top to bottom.\n")
                var index = types.str_to_int(io.input("Select an index: "))
                List.remove($todos, $index)
                List.remove($description, $index)
        elif $user_input equ 4 then
                break
        end                         
end

var db_str = db.from_lists($todos, $description)
file.write_from_str("todo.db", $db_str)
