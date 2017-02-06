package re0

type nfaEdge struct {
	next     *NFAState
	epsilion bool
	ch       byte
}

type NFAState struct {
	edges  []*nfaEdge
	id     int
	accept bool
}

func (s *NFAState) AddE(next *NFAState) {
	s.edges = append(s.edges, &nfaEdge{epsilion: true, next: next})
}

func (s *NFAState) AddChar(c byte, next *NFAState) {
	s.edges = append(s.edges, &nfaEdge{ch: c, next: next})
}

type NFA struct {
	Start *NFAState
	End   *NFAState
}

func nfa_e() *NFA {
	start := &NFAState{}
	end := &NFAState{}
	start.AddE(end)
	return &NFA{start, end}
}

func nfa_char(c byte) *NFA {
	start := &NFAState{}
	end := &NFAState{}
	start.AddChar(c, end)
	return &NFA{start, end}
}

func nfa_cat(a *NFA, b *NFA) *NFA {
	*a.End = *b.Start
	return &NFA{a.Start, b.End}
}

func nfa_or(a *NFA, b *NFA) *NFA {
	start := &NFAState{}
	end := &NFAState{}
	start.AddE(a.Start)
	a.End.AddE(end)
	start.AddE(b.Start)
	b.End.AddE(end)
	return &NFA{start, end}
}

func nfa_star(a *NFA) *NFA {
	start := &NFAState{}
	end := &NFAState{}
	start.AddE(a.Start)
	start.AddE(end)
	a.End.AddE(end)
	a.End.AddE(a.Start)
	return &NFA{start, end}
}

func nfa_alloc_id(n *NFAState, id int) int {
	if n.id != 0 {
		return id
	}
	n.id = id
	id++
	for _, e := range n.edges {
		id = nfa_alloc_id(e.next, id)
	}
	return id
}

func nfa_move(states []*NFAState, c byte) []*NFAState {
	set := make(map[*NFAState]struct{})
	for _, st := range states {
		for _, e := range st.edges {
			if !e.epsilion && e.ch == c {
				set[e.next] = struct{}{}
			}
		}
	}
	next := make([]*NFAState, 0, len(set))
	for k, _ := range set {
		next = append(next, k)
	}
	return next
}

func e_closure(t []*NFAState) []*NFAState {
	stack := make([]*NFAState, len(t))
	set := make(map[*NFAState]struct{}, len(t))
	for i := 0; i < len(t); i++ {
		set[t[i]] = struct{}{}
		stack[i] = t[i]
	}
	for len(stack) > 0 {
		t := stack[len(stack)-1]
		stack = stack[:len(stack)-1]
		for _, e := range t.edges {
			if !e.epsilion {
				continue
			}
			if _, ok := set[e.next]; !ok {
				set[e.next] = struct{}{}
				stack = append(stack, e.next)
			}
		}
	}
	ec := make([]*NFAState, 0, len(set))
	for st, _ := range set {
		ec = append(ec, st)
	}
	return ec
}

func nfa_dstate(ec []*NFAState) *dfa_state {
	hash := 0
	accept := false
	for _, st := range ec {
		if !accept {
			accept = st.accept
		}
		hash += st.id
		for i := len(ec) - 2; i >= 0; i-- {
			if ec[i].id <= st.id {
				break
			}
			t := ec[i+1]
			ec[i+1] = ec[i]
			ec[i] = t
		}
	}
	return &dfa_state{hash: hash, states: ec, accept: accept}
}

func in_states(states []*dfa_state, u *dfa_state) int {
	for i, st := range states {
		if u.hash != st.hash || len(u.states) != len(st.states) {
			continue
		}
		diff := false
		for j := 0; j < len(u.states); j++ {
			if u.states[j] != st.states[j] {
				diff = true
				break
			}
		}
		if !diff {
			return i
		}
	}
	return -1
}

func nfa_to_dfa(n *NFA) *DFA {
	dstates := []*dfa_state{nfa_dstate(e_closure([]*NFAState{n.Start}))}
	dtran := make([][128]int, 1)
	daccept := []bool{false}
	stack := []int{0}
	for len(stack) > 0 {
		t := dstates[stack[len(stack)-1]]
		stack = stack[:len(stack)-1]
		charset := [128]bool{}
		for _, st := range t.states {
			for _, e := range st.edges {
				if e.epsilion {
					continue
				}
				charset[e.ch] = true
			}
		}
		for a := range charset {
			if !charset[a] {
				continue
			}
			u := nfa_dstate(e_closure(nfa_move(t.states, byte(a))))
			i := in_states(dstates, u)
			if i == -1 {
				u.i = len(dstates)
				dstates = append(dstates, u)
				dtran = append(dtran, [128]int{})
				daccept = append(daccept, u.accept)
				stack = append(stack, u.i)
			} else {
				u = dstates[i]
			}
			dtran[t.i][a] = u.i
		}
	}
	return &DFA{tran: dtran, accept: daccept}
}
