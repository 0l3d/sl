use("net", "io", "errors")

var client_sock = errors.string(net.new_socket($AF_INET, $SOCK_STREAM, 0))
errors.string(net.connect($client_sock, "127.0.0.1", 8080))
net.send($client_sock, "Hello, World!", 0)
net.close($client_sock)
