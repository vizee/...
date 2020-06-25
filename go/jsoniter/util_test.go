package jsoniter

import (
	"testing"
)

func TestUnescapeString(t *testing.T) {
	s, err := UnescapeString([]byte(`"123\tabc"`))
	if err != nil {
		t.Fatal(err)
	}
	t.Log(string(s))
	s, err = UnescapeString([]byte(`"\u4f60\u597d"`))
	if err != nil {
		t.Fatal(err)
	}
	t.Log(string(s))
}

func TestValid(t *testing.T) {
	ok := Valid(jsondata)
	if !ok {
		t.Fatal("fail")
	}
}
