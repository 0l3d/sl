# Simple hashmap implementation.

use("collections", "list", "errors", "io", "types")

def Hash_add -> key, value then 
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

def Hash_get -> key then 
	var list_var = Collections.get_attr($self, "key_list")
	var value_var = Collections.get_attr($self, "value_list")
	if not(types.is_list($list_var)) then 
		return errors.return("No items found in HashMap")
	end
	var index = List.find($list_var, $key)
	return List.get($value_var, $index)
end

def Hash_set -> key, new_val then 
	var list_var = Collections.get_attr($self, "key_list")
	var value_var = Collections.get_attr($self, "value_list")
	if not(types.is_list($list_var)) then 
		return errors.return("No items found in HashMap")
	end
	var index = List.find($list_var, $key)
	List.set($value_var, $index, $new_val)
end

Collections.create_collection("HashMap", 
				"v:key_list", 
				"v:value_list", 
				"f:Hash_add:add", 
				"f:Hash_get:get",
				"f:Hash_set:set"
)

var hashmap = HashMap:new()

hashmap:add("Country", "United States")
hashmap:add("State", "California")
hashmap:add("City", "Berkeley")

hashmap:set("City", "San Francisco")

io.print(
	"Country: ", hashmap:get("Country"), "\n", 
	"State: ", hashmap:get("State"), "\n",
	"City: ", hashmap:get("City"), "\n")
	
# Output:
# Country: United States
# State: California
# City: San Francisco
