package jsonpb

import (
	"bytes"
	"github.com/golang/protobuf/proto"
	"testing"
	"toys/jsonpb/testdata"
)

func TestMapJsonPB(t *testing.T) {
	source := `{"a":1,"b":"hello","c":true}`
	it := &jsoniter{data: []byte(source)}
	out, err := MapJsonPB(it, map[string]int{
		"a": 1,
		"b": 2,
		"c": 3,
	})
	if err != nil {
		t.Fatal(err)
	}
	t.Log(out)
	out2, err := proto.Marshal(&testdata.Foo{
		A: 1,
		B: "hello",
		C: true,
	})
	if err != nil {
		t.Fatal(err)
	}
	t.Log(out2)
	if !bytes.Equal(out, out2) {
		t.Error("bad answer")
	}
}

func BenchmarkMapJsonPB(b *testing.B) {
	source := []byte(`{"a":1,"b":"hello","c":true}`)
	fields := map[string]int{
		"a": 1,
		"b": 2,
		"c": 3,
	}
	it := &jsoniter{}
	for i := 0; i < b.N; i++ {
		it.data = source
		it.i = 0
		it.err = nil
		MapJsonPB(it, fields)
	}
}
