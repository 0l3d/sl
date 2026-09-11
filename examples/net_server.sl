use("io", "net", "errors")

var server_sock = net.new_socket($AF_INET, $SOCK_STREAM, 0)

net.bind($server_sock, "0.0.0.0", 8080)

net.listen($server_sock, 128)

while true then 
    var client_sock = net.accept($server_sock)
    io.print("Client connected!\n")
    io.fflush()
    var data = net.recv($client_sock, 1024)
    io.print("Client says: ", $data, "\n")
    io.fflush()
    net.close($client_sock)
end

net.close($server_sock)
