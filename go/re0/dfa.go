package re0

type DFA struct {
	tran   [][128]int
	accept []bool
}

func (d *DFA) match(s string) bool {
	st := 0
	for i := 0; i < len(s); i++ {
		st = d.tran[st][s[i]]
	}
	return d.accept[st]
}

type dfa_state struct {
	states []*NFAState
	hash   int
	i      int
	accept bool
}
