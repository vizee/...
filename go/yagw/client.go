package main

import (
	"encoding/json"
	"fmt"
	"log"
	"sync/atomic"
	"time"
)

const (
	DELEVER_CHAN_DEEP = 64
	LOGIN_TIMEOUT     = 3 * time.Second
	HB_CHECK_INTERVAL = 5 * time.Second
	HEARTBEAT_TIMEOUT = int64(9 * time.Second)
)

type nonameconn netconn

func (c *nonameconn) OnTime() {
	if atomic.LoadInt32(&c.auth) == 0 {
		log.Printf("login timeout")
		((*netconn)(c)).Close()
	}
}

type client struct {
	deliver chan []byte
	wbuf    []byte
	uid     int32
	bad     int32
	lasthb  int64
	conn    *netconn
}

func (c *client) OnTime() {
	if atomic.LoadInt32(&c.bad) != 0 {
		return
	}
	if time.Now().UnixNano()-atomic.LoadInt64(&c.lasthb) > HEARTBEAT_TIMEOUT {
		log.Printf("heartbeat timeout")
		c.conn.Close()
	} else {
		set_timeout(c, HB_CHECK_INTERVAL)
	}
}

func (c *client) post(msg []byte) bool {
	select {
	case c.deliver <- msg:
		return true
	default:
	}
	return false
}

func (c *client) handle_login(pr *packet_reader) (bool, int32, error) {
	cmd, seq := pr.get_header()
	if cmd != CMD_LOGIN {
		return false, 0, fmt.Errorf("invalid cmd: %d", cmd)
	}
	var req message_login_req
	err := json.Unmarshal(pr.get_body(), &req)
	if err != nil {
		return false, 0, err
	}
	if req.User == "" || req.Token == "" {
		return false, 0, fmt.Errorf("invalid request data: %v", req)
	}
	ok, uid, err := rpc_auth_user(req.User, req.Token)
	if err != nil {
		return false, 0, fmt.Errorf("rpc_auth_user() failed: %v", err)
	}
	data, _ := json.Marshal(&message_login_resp{
		OK: ok,
	})
	c.wbuf = write_packet(c.wbuf, CMD_LOGIN, seq, data)
	return ok, uid, nil
}

func (c *client) handle(pr *packet_reader) error {
	cmd, seq := pr.get_header()
	switch cmd {
	case CMD_PING:
		atomic.StoreInt64(&c.lasthb, time.Now().UnixNano())
		c.wbuf = write_packet(c.wbuf, CMD_PING, seq, pr.get_body())
	}
	return nil
}

func (c *client) auth_init(uid int32) {
	c.deliver = make(chan []byte, DELEVER_CHAN_DEEP)
	c.uid = uid
	c.lasthb = time.Now().UnixNano()
	set_timeout(c, HB_CHECK_INTERVAL)
}

func new_client(conn *netconn) *client {
	return &client{
		wbuf: make([]byte, 0, 1024),
		conn: conn,
	}
}
