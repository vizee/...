package jsonpb

import (
	"testing"
)

func TestProtoEmit(t *testing.T) {
	e := pbencoder{}
	e.emitVarint(1, 150)
	e.emitLenDelim(2, []byte(`testing`))
	e2 := pbencoder{}
	e2.emitLenDelim(3, e.s.Bytes())
	t.Logf("% x", e2.s.Bytes())
}
