package jsoniter

import (
	"testing"
)

func TestIterFast(t *testing.T) {
	it := Fast(jsondata)
	for !it.EOF() {
		kind, part := it.Next()
		if kind == Invalid {
			t.Fatal(string(part))
		}
	}
}
