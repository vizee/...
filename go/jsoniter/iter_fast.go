package jsoniter

type IterFast struct {
	data []byte
	i    int
}

func (it *IterFast) skipuseless() {
	i := it.i
	for i < len(it.data) {
		c := it.data[i]
		if !iswhitespace(c) && c != ':' && c != ',' {
			break
		}
		i++
	}
	it.i = i
}

func (it *IterFast) readstring() (Kind, []byte) {
	i := it.i
	start := i
	i++
	for i < len(it.data) {
		if it.data[i] == '"' && it.data[i-1] != '\\' {
			it.i = i + 1
			return String, it.data[start:it.i]
		}
		i++
	}
	return Invalid, it.data[start:]
}

func (it *IterFast) readnumber() (Kind, []byte) {
	i := it.i
	start := i
	for i < len(it.data) {
		c := it.data[i]
		if !isdigit(c) && c != '-' && c != '.' && c != 'e' && c != 'E' {
			break
		}
		i++
	}
	it.i = i
	return Number, it.data[start:i]
}

func (it *IterFast) Next() (Kind, []byte) {
	it.skipuseless()
	if it.EOF() {
		return Invalid, nil
	}
	c := it.data[it.i]
	switch c {
	case 'n', 't':
		const expected = 4
		if it.i+expected >= len(it.data) {
			return Invalid, it.data[it.i:]
		}
		part := it.data[it.i : it.i+expected]
		it.i += expected
		if c == 'n' {
			return Null, part
		} else {
			return True, part
		}
	case 'f':
		const expected = 5
		if it.i+expected >= len(it.data) {
			return Invalid, it.data[it.i:]
		}
		part := it.data[it.i : it.i+expected]
		it.i += expected
		return False, part
	case '"':
		return it.readstring()
	case '{', '}', '[', ']':
		i := it.i
		it.i++
		var kind Kind
		switch c {
		case '{':
			kind = Object
		case '}':
			kind = CloseObject
		case '[':
			kind = Array
		case ']':
			kind = CloseArray
		default:
			kind = Invalid
		}
		return kind, it.data[i:it.i]
	default:
		if isdigit(c) || c == '-' {
			return it.readnumber()
		}
		return Invalid, it.data[it.i : it.i+1]
	}
}

func (it *IterFast) EOF() bool {
	return it.i >= len(it.data)
}

func (it *IterFast) Reset(data []byte) {
	it.data = data
	it.i = 0
}

func Fast(data []byte) *IterFast {
	return &IterFast{
		data: data,
	}
}
