var file_name = sys.get_arg(2)
if errors.bool($file_name) then 
    print("Program get an Error: \n");
    errors.string($file_name)
end

# WIP
