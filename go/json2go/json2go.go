package main

import (
	"encoding/json"
	"errors"
	"io"
	"strings"
)

var (
	errInvalidDelim = errors.New("invalid delim")
	errInvalidKey   = errors.New("invalid key")
	errInvalidType  = errors.New("invalid type")
)

type converter struct {
	dec    *json.Decoder
	w      io.Writer
	indent string
}

func (c *converter) nextType() error {
	t, err := c.dec.Token()
	if err != nil {
		if err == io.EOF {
			err = nil
		}
		return err
	}
	switch v := t.(type) {
	case json.Delim:
		switch v {
		case '{':
			c.indent += "\t"
			if err := c.nextStruct(); err != nil {
				return err
			}
		case '[':
			if err := c.nextArray(); err != nil {
				return err
			}
		default:
			return errInvalidDelim
		}
	case string:
		if _, err := c.w.Write([]byte("string")); err != nil {
			return err
		}
	case bool:
		if _, err := c.w.Write([]byte("bool")); err != nil {
			return err
		}
	case json.Number:
		if strings.IndexByte(v.String(), '.') >= 0 {
			if _, err := c.w.Write([]byte("float64")); err != nil {
				return err
			}
		} else {
			if _, err := c.w.Write([]byte("int")); err != nil {
				return err
			}
		}
	case nil:
		if _, err := c.w.Write([]byte("interface{}")); err != nil {
			return err
		}
	default:
		return errInvalidType
	}
	return nil
}

func (c *converter) nextStruct() error {
	if _, err := c.w.Write([]byte("struct {\n")); err != nil {
		return err
	}
	keyname := ""
	for {
		if keyname == "" {
			t, err := c.dec.Token()
			if err != nil {
				return err
			}
			if delim, ok := t.(json.Delim); ok {
				if delim != '}' {
					return errInvalidDelim
				}
				c.indent = c.indent[:len(c.indent)-1]
				c.w.Write([]byte(c.indent + "}"))
				break
			} else if key, ok := t.(string); ok {
				keyname = key
				c.w.Write([]byte(c.indent + strings.Title(key) + " "))
			} else {
				return errInvalidKey
			}
		} else {
			if err := c.nextType(); err != nil {
				return err
			}
			c.w.Write([]byte(" `json:\"" + keyname + "\"`\n"))
			keyname = ""
		}
	}
	return nil
}

func (c *converter) nextArray() error {
	if _, err := c.w.Write([]byte("[]")); err != nil {
		return err
	}
	if err := c.nextType(); err != nil {
		return err
	}
	deep := 1
	for deep > 0 {
		t, err := c.dec.Token()
		if err != nil {
			return err
		}
		if delim, ok := t.(json.Delim); ok {
			if delim == '[' {
				deep++
			} else if delim == ']' {
				deep--
			}
		}
	}
	return nil
}

func json2go(r io.Reader, w io.Writer, pkgname, name string) error {
	w.Write([]byte("package " + pkgname + "\n"))
	w.Write([]byte("\n"))
	w.Write([]byte("type " + name + " "))
	dec := json.NewDecoder(r)
	dec.UseNumber()
	c := converter{
		dec: dec,
		w:   w,
	}
	c.nextType()
	return nil
}
