package jsoniter

import (
	"fmt"
	"io"
)

type Error struct {
	Pos    int
	Reason string
}

func (e *Error) Error() string {
	return e.Reason
}

type Kind int

const (
	Invalid Kind = iota
	Null
	False
	True
	Number
	String
	Opener
	Object
	Array
	Closer
	CloseObject
	CloseArray
)

type Iter struct {
	data []byte
	i    int
	cur  int
	top  []int
	err  error
}

func (it *Iter) skipwhitespace() bool {
	i := it.i
	for i < len(it.data) && iswhitespace(it.data[i]) {
		i++
	}
	it.i = i
	if i == len(it.data) {
		it.err = io.ErrUnexpectedEOF
		return true
	}
	return false
}

func (it *Iter) readstring() (Kind, []byte) {
	i := it.i
	start := i
	if it.data[i] != '"' {
		return it.errorf("expect '\"', got %c", it.data[i])
	}
	i++
	for i < len(it.data) {
		if it.data[i] == '"' && it.data[i-1] != '\\' {
			it.i = i + 1
			return String, it.data[start:it.i]
		}
		i++
	}
	it.err = io.ErrUnexpectedEOF
	return Invalid, nil
}

func (it *Iter) readnumber() (Kind, []byte) {
	i := it.i
	start := i
	if it.data[i] == '-' {
		i++
	}
	isfrac := false
	isexp := false
parsenum:
	if i >= len(it.data) {
		goto eof
	}
	if !isdigit(it.data[i]) {
		return it.errorf("expect digit, got '%c'", it.data[i])
	}
	i++
	for i < len(it.data) {
		c := it.data[i]
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
			if i < len(it.data) && it.data[i] == '-' {
				i++
			}
			goto parsenum
		default:
			goto retok
		}
	}
retok:
	it.i = i
	return Number, it.data[start:i]
eof:
	it.err = io.ErrUnexpectedEOF
	return Invalid, nil
}

func (it *Iter) readvalue() (Kind, []byte) {
	c := it.data[it.i]
	switch c {
	case 'n':
		const expected = "null"
		if it.i+len(expected) > len(it.data) {
			it.err = io.ErrUnexpectedEOF
			return Invalid, nil
		}
		part := it.data[it.i : it.i+len(expected)]
		for i := 0; i < len(expected); i++ {
			if part[i] != expected[i] {
				return it.errorf("expect '%c', got '%c'", expected[i], part[i])
			}
		}
		it.i += len(expected)
		return Null, part
	case 't':
		const expected = "true"
		if it.i+len(expected) > len(it.data) {
			it.err = io.ErrUnexpectedEOF
			return Invalid, nil
		}
		part := it.data[it.i : it.i+len(expected)]
		for i := 0; i < len(expected); i++ {
			if part[i] != expected[i] {
				return it.errorf("expect '%c', got '%c'", expected[i], part[i])
			}
		}
		it.i += len(expected)
		return True, part
	case 'f':
		const expected = "false"
		if it.i+len(expected) > len(it.data) {
			it.err = io.ErrUnexpectedEOF
			return Invalid, nil
		}
		part := it.data[it.i : it.i+len(expected)]
		for i := 0; i < len(expected); i++ {
			if part[i] != expected[i] {
				return it.errorf("expect '%c', got '%c'", expected[i], part[i])
			}
		}
		it.i += len(expected)
		return False, part
	case '"':
		return it.readstring()
	case '{':
		if it.cur != 0 {
			it.top = append(it.top, it.cur)
		}
		it.cur = 3
		i := it.i
		it.i++
		return Object, it.data[i:it.i]
	case '[':
		if it.cur != 0 {
			it.top = append(it.top, it.cur)
		}
		it.cur = 1
		i := it.i
		it.i++
		return Array, it.data[i:it.i]
	default:
		if isdigit(c) || c == '-' {
			return it.readnumber()
		}
		return it.errorf("invalid character '%c'", c)
	}
}

func (it *Iter) close(kind Kind) (Kind, []byte) {
	if len(it.top) > 0 {
		it.cur = it.top[len(it.top)-1]
		it.top = it.top[:len(it.top)-1]
	} else {
		it.cur = 0
	}
	i := it.i
	it.i++
	return kind, it.data[i:it.i]
}

func (it *Iter) Next() (Kind, []byte) {
	if it.err != nil {
		return Invalid, nil
	}
	if it.skipwhitespace() {
		if it.cur == 0 {
			// EOF
			it.err = nil
		}
		return Invalid, nil
	}
	// parser state machine
	switch it.cur {
	case 0: // value
	case 1: // value | "]"
		if it.data[it.i] == ']' {
			return it.close(CloseArray)
		}
		it.cur = 2
	case 2: // ("," value) | "]"
		switch it.data[it.i] {
		case ',':
			it.i++
			if it.skipwhitespace() {
				return Invalid, nil
			}
		case ']':
			return it.close(CloseArray)
		default:
			return it.errorf("expect ',' or ']', got '%c'", it.data[it.i])
		}
	case 3: // key | "}"
		if it.data[it.i] == '}' {
			return it.close(CloseObject)
		}
		it.cur = 4
		return it.readstring()
	case 4: // ":" value
		if it.data[it.i] == ':' {
			it.i++
			if it.skipwhitespace() {
				return Invalid, nil
			}
		} else {
			return it.errorf("expect ':', got '%c'", it.data[it.i])
		}
		it.cur = 5
	case 5: // ( "," key ) | "}"
		switch it.data[it.i] {
		case ',':
			it.i++
			if it.skipwhitespace() {
				return Invalid, nil
			}
			it.cur = 4
			return it.readstring()
		case '}':
			return it.close(CloseObject)
		default:
			return it.errorf("expect ',' or '}', got %c", it.data[it.i])
		}
	}
	return it.readvalue()
}

func (it *Iter) errorf(format string, args ...interface{}) (Kind, []byte) {
	it.err = &Error{Pos: it.i, Reason: fmt.Sprintf(format, args...)}
	return Invalid, nil
}

func (it *Iter) EOF() bool {
	return it.i >= len(it.data)
}

func (it *Iter) Error() error {
	return it.err
}

func (it *Iter) Reset(data []byte) {
	it.data = data
	it.i = 0
	it.cur = 0
	it.top = it.top[:0]
	it.err = nil
}

func New(data []byte) *Iter {
	return &Iter{
		data: data,
	}
}
