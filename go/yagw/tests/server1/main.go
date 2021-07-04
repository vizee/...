package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net"
	"runtime"
	"strconv"
	"sync/atomic"
	"time"

	"github.com/vizee/timer"
	"github.com/vizee/yagw/tests/clientnet"
)

const (
	LOGIN_TIMEOUT     = 3 * time.Second
	HB_CHECK_INTERVAL = 5 * time.Second
	HEARTBEAT_TIMEOUT = int64(9 * time.Second)
)

var (
	timeridx uint64
	timers   = make([]timer.Timer, runtime.NumCPU())
)

func set_timeout(h timer.Handler, after time.Duration) {
	idx := atomic.AddUint64(&timeridx, 1) % uint64(len(timers))
	timers[idx].Add(h, time.Now().UnixNano()+int64(after))
}

type client struct {
	conn   net.Conn
	lasthb int64
}

func (c *client) OnTime() {
	if time.Now().UnixNano()-atomic.LoadInt64(&c.lasthb) > HEARTBEAT_TIMEOUT {
		c.conn.Close()
	} else {
		set_timeout(c, HB_CHECK_INTERVAL)
	}
}

func rpc_auth_user(user string, token string) (bool, int32, error) {
	time.Sleep(50 * time.Millisecond)
	uid, err := strconv.Atoi(user)
	return true, int32(uid), err
}

func handle_login(conn net.Conn) error {
	cmd, seq, data, err := clientnet.ReadPacket(conn)
	if cmd != clientnet.CMD_LOGIN {
		return fmt.Errorf("unexpected cmd: %d", cmd)
	}
	var req clientnet.MessageLoginReq
	err = json.Unmarshal(data, &req)
	if err != nil {
		return err
	}
	ok, _, err := rpc_auth_user(req.User, req.Token)
	if err != nil {
		return err
	}
	data, _ = json.Marshal(&clientnet.MessageLoginResp{
		OK: ok,
	})
	err = clientnet.WritePacket(conn, clientnet.CMD_LOGIN, seq, data)
	if !ok {
		return clientnet.ErrLoginFailed
	}
	return err
}

func handle_conn(conn net.Conn) {
	defer conn.Close()
	conn.SetDeadline(time.Now().Add(LOGIN_TIMEOUT))
	err := handle_login(conn)
	if err != nil {
		return
	}
	conn.SetDeadline(time.Time{})
	c := &client{
		conn:   conn,
		lasthb: time.Now().UnixNano(),
	}
	set_timeout(c, HB_CHECK_INTERVAL)
	for {
		cmd, seq, data, err := clientnet.ReadPacket(conn)
		if err != nil {
			return
		}
		if cmd == clientnet.CMD_PING {
			atomic.StoreInt64(&c.lasthb, time.Now().UnixNano())
			err = clientnet.WritePacket(conn, clientnet.CMD_PING, seq, data)
			if err != nil {
				return
			}
		}
	}
}

func main() {
	var (
		listen string
	)
	flag.StringVar(&listen, "l", ":9249", "listen")
	flag.Parse()
	ln, err := net.Listen("tcp", listen)
	if err != nil {
		panic(err)
	}
	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Printf("accept: %v", err)
			time.Sleep(time.Second * 2)
			continue
		}
		go handle_conn(conn)
	}
}
