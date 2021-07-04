package main

import (
	"bufio"
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

type iomsg struct {
	cmd  uint32
	seq  uint32
	data []byte
}

func write_loop(conn net.Conn, msgs <-chan iomsg) {
	defer conn.Close()
	bw := bufio.NewWriterSize(conn, clientnet.WBUF_SIZE_LIMIT)
	for {
		msg, ok := <-msgs
		if !ok {
			return
		}
		eoc := false
		for !eoc {
			err := clientnet.WritePacketBuf(bw, msg.cmd, msg.seq, msg.data)
			if err != nil {
				return
			}
			select {
			case msg = <-msgs:
			default:
				eoc = true
			}
		}
		err := bw.Flush()
		if err != nil {
			return
		}
	}
}

func handle_conn(conn net.Conn) {
	conn.SetDeadline(time.Now().Add(LOGIN_TIMEOUT))
	err := handle_login(conn)
	if err != nil {
		conn.Close()
		return
	}
	conn.SetDeadline(time.Time{})
	c := &client{
		conn:   conn,
		lasthb: time.Now().UnixNano(),
	}
	set_timeout(c, HB_CHECK_INTERVAL)
	msgs := make(chan iomsg, 64)
	defer close(msgs)
	go write_loop(conn, msgs)
	for {
		cmd, seq, data, err := clientnet.ReadPacket(conn)
		if err != nil {
			return
		}
		if cmd == clientnet.CMD_PING {
			atomic.StoreInt64(&c.lasthb, time.Now().UnixNano())
			msgs <- iomsg{cmd: clientnet.CMD_PING, seq: seq, data: data}
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
