package jsonpb

import (
	"fmt"
	"math"
	"strconv"
)

func mapnumber(field int, s []byte, pbenc *pbencoder) error {
	t := string(s)
	x, err := strconv.ParseInt(t, 10, 64)
	if err == nil {
		pbenc.emitVarint(field, uint64(x))
		return nil
	}
	f, err := strconv.ParseFloat(t, 64)
	if err != nil {
		return err
	}
	pbenc.emitFixed64(field, math.Float64bits(f))
	return nil
}

func MapJsonPB(it *jsoniter, fields map[string]int) ([]byte, error) {
	kind, _ := it.next()
	if kind != jsonobject {
		return nil, fmt.Errorf("unexpected kind: %d", kind)
	}
	var pbenc pbencoder
	for {
		keykind, keydata := it.next()
		if keykind == jsonobject {
			break
		}
		valkind, valdata := it.next()
		if keykind == jsoninvalid || valkind == jsoninvalid {
			return nil, it.err
		}
		field, ok := fields[string(keydata[1:len(keydata)-1])]
		if !ok {
			continue
		}
		switch valkind {
		case jsonstring:
			b, err := unescape(valdata)
			if err != nil {
				return nil, err
			}
			if len(b) != 0 {
				pbenc.emitLenDelim(field, b)
			}
		case jsonnumber:
			err := mapnumber(field, valdata, &pbenc)
			if err != nil {
				return nil, err
			}
		case jsontrue:
			pbenc.emitVarint(field, 1)
		case jsonarray:
			return nil, fmt.Errorf("unsupport array kind")
		case jsonobject:
			return nil, fmt.Errorf("unsupport object kind")
		}
	}
	return pbenc.s.Bytes(), nil
}
