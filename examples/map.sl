# Simple map implementation.

use("collections", "list", "errors", "io", "types")

def Map_add -> key, value then 
	var list_var = Collections.get_attr($self, "key_list")
	var value_var = Collections.get_attr($self, "value_list")
	if not(types.is_list($list_var)) then 
		Collections.set_attr($self, "key_list", List.new())
	end
	if not(types.is_list($value_var)) then 
		Collections.set_attr($self, "value_list", List.new())
	end
	$list_var = Collections.get_attr($self, "key_list")
	$value_var = Collections.get_attr($self, "value_list")
	List.push($list_var, $key)
	List.push($value_var, $value)
end

def Map_get -> key then 
	var list_var = Collections.get_attr($self, "key_list")
	var value_var = Collections.get_attr($self, "value_list")
	if not(types.is_list($list_var)) then 
		return errors.return("No items found in HashMap")
	end
	var index = List.find($list_var, $key)
	return List.get($value_var, $index)
end

def Map_set -> key, new_val then 
	var list_var = Collections.get_attr($self, "key_list")
	var value_var = Collections.get_attr($self, "value_list")
	if not(types.is_list($list_var)) then 
		return errors.return("No items found in HashMap")
	end
	var index = List.find($list_var, $key)
	List.set($value_var, $index, $new_val)
end

Collections.create_collection("Map", 
				"v:key_list", 
				"v:value_list", 
				"f:Map_add:add", 
				"f:Map_get:get",
				"f:Map_set:set"
)

var map = Map:new()

map:add("Country", "United States")
map:add("State", "California")
map:add("City", "Berkeley")

map:set("City", "San Francisco")

io.print(
	"Country: ", map:get("Country"), "\n", 
	"State: ", map:get("State"), "\n",
	"City: ", map:get("City"), "\n")
	
# Output:
# Country: United States
# State: California
# City: San Francisco
