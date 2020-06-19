package jsonpb

import (
	"fmt"
	"io"
	"unicode/utf8"
)

type jsonkind int

const (
	jsoninvalid jsonkind = iota
	jsonnull
	jsonfalse
	jsontrue
	jsonnumber
	jsonstring
	jsonarray
	jsonobject
)

type jsoniter struct {
	data []byte
	i    int
	cur  int
	top  []int
	err  error
}

func (p *jsoniter) skipwhitespace() bool {
	i := p.i
	for i < len(p.data) && iswhitespace(p.data[i]) {
		i++
	}
	p.i = i
	if i == len(p.data) {
		p.err = io.ErrUnexpectedEOF
		return true
	}
	return false
}

func (p *jsoniter) readstring() (jsonkind, []byte) {
	i := p.i
	start := i
	if p.data[i] != '"' {
		p.err = fmt.Errorf("expect '\"', got %c", p.data[i])
		return jsoninvalid, nil
	}
	i++
	for i < len(p.data) {
		if p.data[i] == '"' && p.data[i-1] != '\\' {
			p.i = i + 1
			return jsonstring, p.data[start:p.i]
		}
		i++
	}
	p.err = io.ErrUnexpectedEOF
	return jsoninvalid, nil
}

func (p *jsoniter) readnumber() (jsonkind, []byte) {
	i := p.i
	start := i
	if p.data[i] == '-' {
		i++
	}
	isfrac := false
	isexp := false
parsenum:
	if i >= len(p.data) {
		goto eof
	}
	if !isdigit(p.data[i]) {
		p.err = fmt.Errorf("expect digit, got '%c'", p.data[i])
		return jsoninvalid, nil
	}
	i++
	for i < len(p.data) {
		c := p.data[i]
		switch {
		case isdigit(c):
			i++
		case !isfrac && c == '.':
			isfrac = true
			i++
			goto parsenum
		case !isexp && (c == 'e' || c == 'E'):
			isfrac = true
			isexp = true
			i++
			if i < len(p.data) && p.data[i] == '-' {
				i++
			}
			goto parsenum
		default:
			goto retok
		}
	}
retok:
	p.i = i
	return jsonnumber, p.data[start:i]
eof:
	p.err = io.ErrUnexpectedEOF
	return jsoninvalid, nil
}

func (p *jsoniter) readvalue() (jsonkind, []byte) {
	c := p.data[p.i]
	switch c {
	case 'n':
		const expected = "null"
		if p.i+len(expected) > len(p.data) {
			p.err = io.ErrUnexpectedEOF
			return jsoninvalid, nil
		}
		part := p.data[p.i : p.i+len(expected)]
		for i := 0; i < len(expected); i++ {
			if part[i] != expected[i] {
				p.err = fmt.Errorf("expect '%c', got '%c'", expected[i], part[i])
				return jsoninvalid, nil
			}
		}
		p.i += len(expected)
		return jsonnull, part
	case 't':
		const expected = "true"
		if p.i+len(expected) > len(p.data) {
			p.err = io.ErrUnexpectedEOF
			return jsoninvalid, nil
		}
		part := p.data[p.i : p.i+len(expected)]
		for i := 0; i < len(expected); i++ {
			if part[i] != expected[i] {
				p.err = fmt.Errorf("expect '%c', got '%c'", expected[i], part[i])
				return jsoninvalid, nil
			}
		}
		p.i += len(expected)
		return jsontrue, part
	case 'f':
		const expected = "false"
		if p.i+len(expected) > len(p.data) {
			p.err = io.ErrUnexpectedEOF
			return jsoninvalid, nil
		}
		part := p.data[p.i : p.i+len(expected)]
		for i := 0; i < len(expected); i++ {
			if part[i] != expected[i] {
				p.err = fmt.Errorf("expect '%c', got '%c'", expected[i], part[i])
				return jsoninvalid, nil
			}
		}
		p.i += len(expected)
		return jsonfalse, part
	case '[':
		if p.cur != 0 {
			p.top = append(p.top, p.cur)
		}
		p.cur = 1
		i := p.i
		p.i++
		return jsonarray, p.data[i:p.i]
	case '{':
		if p.cur != 0 {
			p.top = append(p.top, p.cur)
		}
		p.cur = 3
		i := p.i
		p.i++
		return jsonobject, p.data[i:p.i]
	case '"':
		return p.readstring()
	default:
		if isdigit(c) || c == '-' {
			return p.readnumber()
		}
		p.err = fmt.Errorf("invalid character '%c'", c)
		return jsoninvalid, nil
	}
}

func (p *jsoniter) close(kind jsonkind) (jsonkind, []byte) {
	if len(p.top) > 0 {
		p.cur = p.top[len(p.top)-1]
		p.top = p.top[:len(p.top)-1]
	} else {
		p.cur = 0
	}
	i := p.i
	p.i++
	return kind, p.data[i:p.i]
}

func (p *jsoniter) next() (jsonkind, []byte) {
	if p.err != nil {
		return jsoninvalid, nil
	}
	if p.skipwhitespace() {
		if p.cur == 0 {
			p.err = io.EOF
		}
		return jsoninvalid, nil
	}
	// parser state machine
	switch p.cur {
	case 0: // value
	case 1: // value | "]"
		if p.data[p.i ] == ']' {
			return p.close(jsonarray)
		}
		p.cur = 2
	case 2: // ("," value) | "]"
		switch p.data[p.i] {
		case ',':
			p.i++
			if p.skipwhitespace() {
				return jsoninvalid, nil
			}
		case ']':
			return p.close(jsonarray)
		default:
			p.err = fmt.Errorf("expect ',' or ']', got '%c'", p.data[p.i])
			return jsoninvalid, nil
		}
	case 3: // key | "}"
		if p.data[p.i ] == '}' {
			return p.close(jsonobject)
		}
		p.cur = 4
		return p.readstring()
	case 4: // ":" value
		if p.data[p.i] == ':' {
			p.i++
			if p.skipwhitespace() {
				return jsoninvalid, nil
			}
		} else {
			p.err = fmt.Errorf("expect ':', got '%c'", p.data[p.i])
			return jsoninvalid, nil
		}
		p.cur = 5
	case 5: // ( "," key ) | "}"
		switch p.data[p.i] {
		case ',':
			p.i++
			if p.skipwhitespace() {
				return jsoninvalid, nil
			}
			p.cur = 4
			return p.readstring()
		case '}':
			return p.close(jsonobject)
		default:
			p.err = fmt.Errorf("expect ',' or '}', got %c", p.data[p.i])
			return jsoninvalid, nil
		}
	}
	return p.readvalue()
}

func (p *jsoniter) error() error {
	return p.err
}

func iswhitespace(c byte) bool {
	return c == ' ' || c == '\n' || c == '\r' || c == '\t'
}

func isdigit(c byte) bool {
	return '0' <= c && c <= '9'
}

const escapetable = "0000000000000000000000000000000000\"000000000000/00000000000000000000000000000000000000000000\\00000\b000\f0000000\n000\r0\tu0000000000"

func unescape(s []byte) ([]byte, error) {
	b := make([]byte, 0, len(s))
	i := 1
	for i < len(s)-1 {
		c := s[i]
		if c == '\\' {
			i++
			c = s[i]
			if int(c) >= len(escapetable) || escapetable[c] == '0' {
				return nil, fmt.Errorf("invalid escape character: '%c'", c)
			}
			if c == 'u' {
				i++
				uc := rune(0)
				for k := 0; k < 4; k++ {
					c = s[i]
					i++
					if isdigit(c) {
						uc = uc<<4 | rune(c-'0')
					} else if 'A' <= c && c <= 'F' {
						uc = uc<<4 | rune(c-'A'+10)
					} else if 'a' <= c && c <= 'f' {
						uc = uc<<4 | rune(c-'a'+10)
					} else {
						return nil, fmt.Errorf("invalid unicode escape sequence: '%c'", c)
					}
				}
				var u8 [6]byte
				n := utf8.EncodeRune(u8[:], uc)
				b = append(b, u8[:n]...)
				continue
			} else {
				c = escapetable[c]
			}
		}
		b = append(b, c)
		i++
	}
	return b, nil
}
