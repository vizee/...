package main

import (
	"flag"
	"log"
	"strconv"
	"sync"
	"time"

	"github.com/vizee/yagw/tests/clientnet"
)

var (
	addr string

	wg sync.WaitGroup
)

func userproc(uid int) {
	defer wg.Done()
	conn, err := clientnet.Login(addr, strconv.Itoa(uid), "test")
	if err != nil {
		log.Printf("login: %v", err)
		return
	}
	defer conn.Close()
	for {
		ts := time.Now()
		wseq := clientnet.NextSeq()
		err := clientnet.WritePacket(conn, clientnet.CMD_PING, wseq, nil)
		if err != nil {
			log.Printf("write ping: %v", err)
			break
		}
		cmd, seq, _, err := clientnet.ReadPacket(conn)
		if err != nil {
			log.Printf("read packet: %v", err)
			break
		}
		if cmd != clientnet.CMD_PING || wseq != seq {
			log.Printf("bad reply: cmd(%d)/seq(%d)", cmd, seq)
			break
		}
		d := time.Second*5 - time.Now().Sub(ts)
		if d > 0 {
			time.Sleep(d)
		}
	}
}

func main() {
	var (
		base int
		nums int
	)
	flag.StringVar(&addr, "c", "127.0.0.1:9249", "server address")
	flag.IntVar(&base, "u", 1, "uid base")
	flag.IntVar(&nums, "n", 1000, "nums")
	flag.Parse()
	for i := base; i < base+nums; i++ {
		wg.Add(1)
		go userproc(i)
	}
	wg.Wait()
}
