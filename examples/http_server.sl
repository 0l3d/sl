use("net", "errors", "io", "types")

var response =
"""HTTP/1.1 200 OK\r\n
Content-Type: text/html; charset=UTF-8\r\n\r\n
<!DOCTYPE html>\r\n
<html>\r\n
<head>\r\n
<title>Testing Basic HTTP-SERVER</title>\r\n
</head>\r\n
<body>\r\n
Hello, World!\r\n
</body>\r\n
</html>\r\n"""

var sock = errors.string(net.new_socket($AF_INET, $SOCK_STREAM, 0))

errors.string(net.bind($sock, "0.0.0.0", 8080))

errors.string(net.listen($sock, 128))

while true then
    var client = net.accept($sock)
    var request = net.recv($client, 1024)
    io.print("DEBUG: ", $request)
    io.fflush()
    net.send($client, $response, 0)
    net.close($client)

end
net.close($sock) 


