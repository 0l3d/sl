# THE CODE IS ONLY FOR FUN AND NOT SUITABLE FOR PRODUCTION.
# This example is for Linux.
# The dyn module is cross platform, so the same approach works on other platforms.
# For example, on Windows:
# var openssl = dyn.open_lib("libssl-3-x64.dll")
use("dyn", "io", "net", "errors", "string")

# libc for buffer allocation and printf
var libc = dyn.open_lib("libc.so.6")
var malloc = dyn.find_symbol($libc, "malloc")
var printf = dyn.find_symbol($libc, "printf")
var free = dyn.find_symbol($libc, "free")

# Load ssl functions we need.
var openssl = dyn.open_lib("libssl.so.3")
var TLS_method = dyn.find_symbol($openssl, "TLS_method")
var SSL_CTX_new = dyn.find_symbol($openssl, "SSL_CTX_new")
var SSL_new = dyn.find_symbol($openssl, "SSL_new")
var SSL_set_fd = dyn.find_symbol($openssl, "SSL_set_fd")
var SSL_connect = dyn.find_symbol($openssl, "SSL_connect")
var SSL_write = dyn.find_symbol($openssl, "SSL_write")
var SSL_read = dyn.find_symbol($openssl, "SSL_read")
var SSL_free = dyn.find_symbol($openssl, "SSL_free")
var SSL_CTX_free = dyn.find_symbol($openssl, "SSL_CTX_free")
var TLS_mtd = dyn.call($TLS_method, 4096, $DYN_POINTER)

# Lets go
var serverfd = net.new_socket($AF_INET, $SOCK_STREAM, 0)
errors.string(net.connect($serverfd, "8.8.8.8", 443))

var ctx = dyn.call($SSL_CTX_new, 4096, $DYN_POINTER, $TLS_mtd)
var ssl = dyn.call($SSL_new, 4096, $DYN_POINTER, $ctx)
var setfd = dyn.call($SSL_set_fd, 4096, $DYN_INTEGER, $ssl, $serverfd)
var request = "GET /\r\n\r\n"
dyn.call($SSL_connect, 4096, $DYN_INTEGER, $ssl)
dyn.call($SSL_write, 4096, $DYN_INTEGER, $ssl, $request,
					string.len($request))

var buff = dyn.call($malloc, 4096, $DYN_POINTER, 1024)

dyn.call($SSL_read, 4096, $DYN_INTEGER, $ssl, $buff, 1023)
dyn.call($printf, 4096, $DYN_INTEGER, "Response: %s\n", $buff)

dyn.call($SSL_free, 4096, $DYN_NORETURN, $ssl)
dyn.call($SSL_CTX_free, 4096, $DYN_NORETURN, $ctx)

dyn.call($free, 4096, $DYN_NORETURN, $buff)

net.close($serverfd)

dyn.free($openssl)
dyn.free($libc)

