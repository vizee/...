package main

import "flag"

func main() {
	var (
		addr string
	)
	flag.StringVar(&addr, "l", ":9249", "listen address")
	flag.Parse()
	server_start(addr)
	server_eventloop()
}
