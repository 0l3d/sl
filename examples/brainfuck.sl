var file_name = sys.get_arg(2)
if errors.bool($file_name) then 
    print("Program got an Error: \n");
    errors.string($file_name)
    sys.exit(-1)
end

var file_content = read.tostr($file_name)
print($file_content)

var mylist = List.new(100)
List.push($mylist, "Hello")
print(List.get($mylist, 0))
List.set($mylist, 0, ", World")
print(List.get($mylist, 0))
print("\nList.len: ", List.len($mylist), "\n")
# WIP
