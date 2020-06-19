package jsonpb

import (
	"bytes"
	"encoding/binary"
)

type wiretype int

const (
	pbvariant  wiretype = iota // Varint: int32, int64, uint32, uint64, sint32, sint64, bool, enum
	pb64bit                    // 64-bit: fixed64, sfixed64, double
	pblendelim                 // Length-delimited: string, bytes, embedded messages, packed repeated fields
	_                          // Start group: groups (deprecated)
	_                          // End group: groups (deprecated)
	pb32bit                    // 32-bit: fixed32, sfixed32, float
)

type pbencoder struct {
	s bytes.Buffer
}

func (e *pbencoder) emitKey(field int, wt wiretype) {
	var buf [binary.MaxVarintLen64]byte
	n := binary.PutUvarint(buf[:], uint64(field)<<3|uint64(wt))
	e.s.Write(buf[:n])
}

func (e *pbencoder) emitVarint(field int, v uint64) {
	e.emitKey(field, pbvariant)
	var buf [binary.MaxVarintLen64]byte
	n := binary.PutUvarint(buf[:], v)
	e.s.Write(buf[:n])
}

func (e *pbencoder) emitVarintZigzag(field int, v int64) {
	e.emitKey(field, pbvariant)
	var buf [binary.MaxVarintLen64]byte
	n := binary.PutVarint(buf[:], v)
	e.s.Write(buf[:n])
}

func (e *pbencoder) emitFixed64(field int, v uint64) {
	e.emitKey(field, pb64bit)
	var buf [8]byte
	binary.LittleEndian.PutUint64(buf[:], v)
	e.s.Write(buf[:])
}

func (e *pbencoder) emitFixed32(field int, v uint32) {
	e.emitKey(field, pb32bit)
	var buf [4]byte
	binary.LittleEndian.PutUint32(buf[:], v)
	e.s.Write(buf[:])
}

func (e *pbencoder) emitLenDelim(field int, v []byte) {
	e.emitKey(field, pblendelim)
	var buf [binary.MaxVarintLen64]byte
	n := binary.PutUvarint(buf[:], uint64(len(v)))
	e.s.Write(buf[:n])
	e.s.Write(v)
}
