package clientnet

import (
	"bufio"
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"sync/atomic"
)

const (
	CMD_NOP   = 0
	CMD_PING  = 100
	CMD_CLOSE = 101
	CMD_LOGIN = 200
)

type MessageLoginReq struct {
	User  string `json:"user"`
	Token string `json:"token"`
}

type MessageLoginResp struct {
	OK bool `json:"ok"`
}

const (
	PACKET_HEADER_LEN = 12

	PACKET_MAX_BODY_LEN = 1024

	WBUF_SIZE_LIMIT = 4096
)

var (
	ErrBadPacketLen = errors.New("bad packet length")
	ErrLoginFailed  = errors.New("login failed")
)

var nseq uint32

func NextSeq() uint32 {
	return atomic.AddUint32(&nseq, 1)
}

/*
packet format:
00-03: BODY-LENGTH
04-07: COMMAND-ID
08-0b: SEQ-NUM
0c-??: BODY DATA
*/

func ReadPacket(c net.Conn) (uint32, uint32, []byte, error) {
	var h [PACKET_HEADER_LEN]byte
	_, err := io.ReadFull(c, h[:])
	if err != nil {
		return 0, 0, nil, err
	}
	bodylen := uint(binary.LittleEndian.Uint32(h[:]))
	if bodylen >= PACKET_MAX_BODY_LEN {
		return 0, 0, nil, ErrBadPacketLen
	}
	buf := make([]byte, bodylen)
	_, err = io.ReadFull(c, buf)
	if err != nil {
		return 0, 0, nil, err
	}
	return binary.LittleEndian.Uint32(h[4:]), binary.LittleEndian.Uint32(h[8:]), buf, nil
}

func WritePacket(conn net.Conn, cmd uint32, seq uint32, body []byte) error {
	buf := make([]byte, PACKET_HEADER_LEN+len(body))
	h := buf[:PACKET_HEADER_LEN] // Bounds-checking elimination
	binary.LittleEndian.PutUint32(h[:], uint32(len(body)))
	binary.LittleEndian.PutUint32(h[4:], cmd)
	binary.LittleEndian.PutUint32(h[8:], seq)
	copy(buf[PACKET_HEADER_LEN:], body)
	_, err := conn.Write(buf)
	return err
}

func WritePacketBuf(w *bufio.Writer, cmd uint32, seq uint32, body []byte) error {
	var h [PACKET_HEADER_LEN]byte
	binary.LittleEndian.PutUint32(h[:], uint32(len(body)))
	binary.LittleEndian.PutUint32(h[4:], cmd)
	binary.LittleEndian.PutUint32(h[8:], seq)
	_, err := w.Write(h[:])
	if err != nil {
		return err
	}
	_, err = w.Write(body)
	return err
}

func Login(addr string, user string, token string) (net.Conn, error) {
	c, err := net.Dial("tcp", addr)
	if err != nil {
		return nil, err
	}
	ok := false
	defer func() {
		if !ok {
			c.Close()
		}
	}()
	logindata, _ := json.Marshal(&MessageLoginReq{
		User:  user,
		Token: token,
	})
	loginseq := NextSeq()
	err = WritePacket(c, CMD_LOGIN, loginseq, logindata)
	if err != nil {
		return nil, err
	}
	cmd, seq, data, err := ReadPacket(c)
	if err != nil {
		return nil, err
	}
	if cmd != CMD_LOGIN || seq != loginseq {
		return nil, fmt.Errorf("unexpected reply: cmd(%d)/seq(%d)", cmd, seq)
	}
	loginresp := &MessageLoginResp{}
	err = json.Unmarshal(data, loginresp)
	if err != nil {
		return nil, err
	}
	if !loginresp.OK {
		return nil, ErrLoginFailed
	}
	ok = true
	return c, nil
}
