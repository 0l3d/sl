var file_name = sys.get_arg(2)
if errors.bool($file_name) then 
    print("Program got an Error: \n");
    errors.string($file_name)
end

# WIP
