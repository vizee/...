package jsoniter

import (
	"fmt"
	"unicode/utf8"
)

func iswhitespace(c byte) bool {
	return c == ' ' || c == '\n' || c == '\r' || c == '\t'
}

func isdigit(c byte) bool {
	return '0' <= c && c <= '9'
}

const escapetable = "0000000000000000000000000000000000\"000000000000/00000000000000000000000000000000000000000000\\00000\b000\f0000000\n000\r0\tu0000000000"

func UnescapeString(s []byte) ([]byte, error) {
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

func Valid(data []byte) bool {
	it := New(data)
	for !it.EOF() {
		k, _ := it.Next()
		if k == Invalid && it.err != nil {
			return false
		}
	}
	return true
}
