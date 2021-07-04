package main

import (
	"encoding/binary"
	"errors"
	"io"
	"syscall"
)

const (
	CMD_NOP   = 0
	CMD_PING  = 100
	CMD_CLOSE = 101
	CMD_LOGIN = 200
)

type message_login_req struct {
	User  string `json:"user"`
	Token string `json:"token"`
}

type message_login_resp struct {
	OK bool `json:"ok"`
}

const (
	PACKET_HEADER_LEN = 12

	PACKET_MAX_BODY_LEN = 1024

	WBUF_SIZE_LIMIT = 4096
)

var ERR_BAD_PACKET_LEN = errors.New("bad packet length")

/*
packet format:
00-03: BODY-LENGTH
04-07: COMMAND-ID
08-0b: SEQ-NUM
0c-??: BODY DATA
*/

type packet_reader struct {
	n      uint
	total  uint
	header [PACKET_HEADER_LEN]byte
	body   [PACKET_MAX_BODY_LEN]byte
}

func (r *packet_reader) reset() {
	r.n = 0
	r.total = 0
}

func (r *packet_reader) get_header() (cmd uint32, seq uint32) {
	return binary.LittleEndian.Uint32(r.header[4:]), binary.LittleEndian.Uint32(r.header[8:])
}

func (r *packet_reader) get_body() []byte {
	return r.body[:r.total-PACKET_HEADER_LEN]
}

func (r *packet_reader) readfrom(fd int) (bool, error) {
	for r.n < PACKET_HEADER_LEN {
		n, err := sys_read(fd, r.header[r.n:])
		if err != 0 {
			if err == syscall.EAGAIN {
				return false, nil
			} else {
				return false, err
			}
		}
		if n == 0 {
			if r.n == 0 {
				return false, io.EOF
			} else {
				return false, io.ErrUnexpectedEOF
			}
		}
		r.n += uint(n)
		if r.n == PACKET_HEADER_LEN {
			bodylen := uint(binary.LittleEndian.Uint32(r.header[:4]))
			if bodylen >= PACKET_MAX_BODY_LEN {
				return false, ERR_BAD_PACKET_LEN
			}
			r.total = PACKET_HEADER_LEN + bodylen
			break
		}
	}
	for r.n < r.total {
		n, err := sys_read(fd, r.body[r.n-PACKET_HEADER_LEN:r.total-PACKET_HEADER_LEN])
		if err != 0 {
			if err == syscall.EAGAIN {
				break
			}
			return false, err
		}
		if n == 0 {
			return false, io.ErrUnexpectedEOF
		}
		r.n += uint(n)
	}
	return r.n == uint(r.total), nil
}

func write_packet(buf []byte, cmd uint32, seq uint32, body []byte) []byte {
	addition := PACKET_HEADER_LEN + len(body)
	p := len(buf)
	if p+addition > cap(buf) {
		t := make([]byte, p+addition, cap(buf)+addition)
		copy(t, buf)
		buf = t
	} else {
		buf = buf[:p+addition]
	}
	h := buf[p : p+PACKET_HEADER_LEN]
	h = h[:PACKET_HEADER_LEN] // Bounds-checking elimination
	binary.LittleEndian.PutUint32(h[:], uint32(len(body)))
	binary.LittleEndian.PutUint32(h[4:], cmd)
	binary.LittleEndian.PutUint32(h[8:], seq)
	copy(buf[p+PACKET_HEADER_LEN:], body)
	return buf
}
